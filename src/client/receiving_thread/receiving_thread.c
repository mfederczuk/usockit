/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <usockit/client/receiving_thread/receiving_thread.h>
#include <usockit/client/receiving_thread/result.h>
#include <usockit/client/threads_result.h>
#include <usockit/cross_support.h>
#include <usockit/memtrace.h>
#include <usockit/support_types.h>
#include <usockit/utils.h>

#define USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING  "fuck off"
#define USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING_SIZE \
	(array_size(USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING) - 1)

struct usockit_client_receiving_thread_routine_arg {
	int socket_fd;
	struct usockit_client_threads_result_dest* result_dest_ptr;
};
static void* usockit_client_receiving_thread_routine(void* arg_ptr) cross_support_attr_nonnull_all;


ret_status_t usockit_client_receiving_thread_create(
	pthread_t* const restrict thread,
	const int socket_fd,
	struct usockit_client_threads_result_dest* const result_dest_ptr
) {
	assert(thread != cross_support_nullptr);
	assert(result_dest_ptr != cross_support_nullptr);


	struct usockit_client_receiving_thread_routine_arg* thread_routine_arg_ptr;

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
			&usockit_client_receiving_thread_routine,
			thread_routine_arg_ptr
		);
	if(ret != 0) {
		free(thread_routine_arg_ptr);

		errno = ret;
		return RET_STATUS_FAILURE;
	}

	// no need to free `thread_routine_arg_ptr` here, it will be free'd in usockit_client_receiving_thread_routine()

	return RET_STATUS_SUCCESS;
}


void* usockit_client_receiving_thread_routine(void* const arg_ptr) {
	assert(arg_ptr != cross_support_nullptr);

	const struct usockit_client_receiving_thread_routine_arg arg =
		*(struct usockit_client_receiving_thread_routine_arg*)arg_ptr;

	free(arg_ptr);

	struct usockit_client_threads_result result;
	zeroset_lvalue(result);
	result.origin = USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_RECEIVING;

	do {
		unsigned char buffer[USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING_SIZE];
		const ssize_t readc = read(arg.socket_fd, buffer, array_size(buffer));

		if((readc == USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING_SIZE) &&
		   (memcmp(
			    buffer,
			    USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING,
			    USOCKIT_CLIENT_RECEIVING_THREAD_FUCK_OFF_STRING_SIZE
		    ) == 0)) {

			result.thread_union.receiving.type = USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_FUCK_OFF;
			break;
		}

		if(readc == 0) { // EOF
			// TODO: will this even happen?
			abort();
		}

		if(readc < 0) { // failure
			result.thread_union.receiving.type = USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_READ_FAILURE;
			result.thread_union.receiving.read_errno = errno;
			break;
		}
	} while(1);

	pthread_mutex_lock(&(arg.result_dest_ptr->mutex));

	// special case: if the sending thread already signalled write() EPIPE - then we override it with our "fuck off"
	// TODO: this is a hack, a better way to do this is via handshakes;
	//       once the protocol is set up, we'll have server & client send handshakes, and only once the server gives the
	//       all clear that the client may send data, we'll signal the sending thread, which will then, only after the
	//       handshake is done, start reading from stdin.
	//       while waiting we can show a message like "Connecting with server..." (only when stderr is tty)
	//       once all of this is implemented, we can use the usockit_client_threads_dispatch_result() function here as
	//       well
	if((arg.result_dest_ptr->result.origin == USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_NONE) ||
	   ((result.thread_union.receiving.type == USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_FUCK_OFF) &&
	    (arg.result_dest_ptr->result.origin == USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_SENDING) &&
	    (arg.result_dest_ptr->result.thread_union.sending.status == EPIPE) &&
	    (arg.result_dest_ptr->result.thread_union.sending.func == USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_WRITE))) {

		arg.result_dest_ptr->result = result;
	}

	pthread_mutex_unlock(&(arg.result_dest_ptr->mutex));

	pthread_cond_signal(&(arg.result_dest_ptr->cond));

	return cross_support_nullptr;
}
