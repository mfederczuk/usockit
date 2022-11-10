/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#define _POSIX_C_SOURCE  199506L

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <usockit/client/sending_thread/result.h>
#include <usockit/client/sending_thread/sending_thread.h>
#include <usockit/client/threads_result.h>
#include <usockit/cross_support.h>
#include <usockit/memtrace.h>
#include <usockit/support_types.h>
#include <usockit/utils.h>

struct usockit_client_sending_thread_routine_arg {
	int socket_fd;
	struct usockit_client_threads_result_dest* result_dest_ptr;
};
static void* usockit_client_sending_thread_routine(void* arg_ptr) cross_support_attr_nonnull_all;


ret_status_t usockit_client_sending_thread_create(
	pthread_t* const restrict thread,
	const int socket_fd,
	struct usockit_client_threads_result_dest* const result_dest_ptr
) {
	assert(thread != cross_support_nullptr);
	assert(result_dest_ptr != cross_support_nullptr);


	struct usockit_client_sending_thread_routine_arg* thread_routine_arg_ptr;

	errno = 0;
	thread_routine_arg_ptr = calloc(1, sizeof *thread_routine_arg_ptr);
	cross_support_if_unlikely(thread_routine_arg_ptr == cross_support_nullptr) {
		return RET_STATUS_FAILURE;
	}

	thread_routine_arg_ptr->socket_fd = socket_fd;
	thread_routine_arg_ptr->result_dest_ptr = result_dest_ptr;


	const int ret =
		pthread_create(
			thread,
			cross_support_nullptr,
			&usockit_client_sending_thread_routine,
			thread_routine_arg_ptr
		);
	if(ret != 0) {
		free(thread_routine_arg_ptr);

		errno = ret;
		return RET_STATUS_FAILURE;
	}

	// no need to free `thread_routine_arg_ptr` here, it will be free'd in usockit_client_sending_thread_routine()

	return RET_STATUS_SUCCESS;
}


void* usockit_client_sending_thread_routine(void* const arg_ptr) {
	assert(arg_ptr != cross_support_nullptr);

	const struct usockit_client_sending_thread_routine_arg arg =
		*(struct usockit_client_sending_thread_routine_arg*)arg_ptr;

	free(arg_ptr);

	// with SIGPIPE blocked, a call to write() on a closed socket will not anymore raise the signal and instead return
	// with `errno` set to `EPIPE` (this default behavior is pretty strange anyhow)
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	pthread_sigmask(SIG_BLOCK, &sigset, cross_support_nullptr);

	struct usockit_client_threads_result result;
	zeroset_lvalue(result);
	result.origin = USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_SENDING;

	do {
		unsigned char buffer[1024];
		const ssize_t readc = read(STDIN_FILENO, buffer, array_size(buffer));

		if(readc > 0) { // success
			const ret_status_t ret_status = write_all(arg.socket_fd, buffer, (size_t)readc);
			if(ret_status != RET_STATUS_SUCCESS) {
				result.thread_union.sending.status = errno;
				result.thread_union.sending.func = USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_WRITE;
				break;
			}

			continue;
		}

		if(readc == 0) { // EOF
			// no need to set `result.thread_union.sending.status` to 0, we memset'd the entire struct to 0 before
			break;
		}

		assert(readc < 0); // failure

		result.thread_union.sending.status = errno;
		result.thread_union.sending.func = USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_READ;
		break;
	} while(1);

	usockit_client_threads_dispatch_result(arg.result_dest_ptr, result);
	return cross_support_nullptr;
}
