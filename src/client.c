/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <usockit/client.h>
#include <usockit/client/receiving_thread/receiving_thread.h>
#include <usockit/client/sending_thread/sending_thread.h>
#include <usockit/client/threads_result.h>
#include <usockit/memtrace.h>
#ifndef NDEBUG
	#include <usockit/shared.h>
#endif
#include <usockit/support_types.h>
#include <usockit/utils.h>

#include <stdio.h>  // TODO: remove this. just required for perror(3)


cross_support_nodiscard
static inline enum usockit_client_ret_status usockit_client_connect(int socket_fd, const_cstr_t socket_pathname)
	cross_support_attr_always_inline
	cross_support_attr_warn_unused_result;


enum usockit_client_ret_status usockit_client(const const_cstr_t socket_pathname) {
	#ifndef NDEBUG
	// extra `#ifndef NDEBUG` here so that the strlen(3) call is not executed on release builds
	{
		assert(socket_pathname != NULL);
		const size_t socket_pathname_len = strlen(socket_pathname);
		assert((socket_pathname_len > 0) && (socket_pathname_len <= USOCKIT_SOCKET_PATHNAME_MAX_LENGTH));
	}
	#endif


	// TODO: check socket_pathname


	errno = 0;
	const int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		// TODO: socket(2) error handling
		perror("socket(2)");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}

	const enum usockit_client_ret_status ret_status = usockit_client_connect(socket_fd, socket_pathname);

	close(socket_fd);

	return ret_status;
}


static inline enum usockit_client_ret_status usockit_client_connect(
	const int socket_fd,
	const const_cstr_t socket_pathname
) {
	struct usockit_client_threads_result_dest* threads_result_dest_ptr;
	threads_result_dest_ptr = calloc(1, sizeof *threads_result_dest_ptr);
	cross_support_if_unlikely(threads_result_dest_ptr == cross_support_nullptr) {
		// TODO: calloc() error handling
		perror("calloc");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}

	int ret = pthread_mutex_init(&(threads_result_dest_ptr->mutex), cross_support_nullptr);
	if(ret != 0) {
		free(threads_result_dest_ptr);

		// TODO: pthread_mutex_init() error handling
		errno = 0;
		perror("pthread_mute_init");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}

	ret = pthread_cond_init(&(threads_result_dest_ptr->cond), cross_support_nullptr);
	if(ret != 0) {
		pthread_mutex_destroy(&(threads_result_dest_ptr->mutex));
		free(threads_result_dest_ptr);

		// TODO: pthread_cond_init() error handling
		errno = 0;
		perror("pthread_cond_init");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}

	threads_result_dest_ptr->result.origin = USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_NONE;


	struct sockaddr_un addr;
	zeroset_lvalue(addr);

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_pathname);

	errno = 0;
	ret = connect(socket_fd, (const struct sockaddr*)&addr, sizeof addr);
	if(ret != 0) {
		pthread_cond_destroy(&(threads_result_dest_ptr->cond));
		pthread_mutex_destroy(&(threads_result_dest_ptr->mutex));
		free(threads_result_dest_ptr);

		// TODO: connect(2) error handling
		perror("connect(2)");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}


	pthread_t receiving_thread;
	ret_status_t ret_status =
		usockit_client_receiving_thread_create(
			&receiving_thread,
			socket_fd,
			threads_result_dest_ptr
		);
	if(ret_status != RET_STATUS_SUCCESS) {
		pthread_cond_destroy(&(threads_result_dest_ptr->cond));
		pthread_mutex_destroy(&(threads_result_dest_ptr->mutex));
		free(threads_result_dest_ptr);

		// TODO: usockit_client_receiving_thread_create() error handling
		perror("usockit_client_receiving_thread_create");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}

	pthread_t sending_thread;
	ret_status =
		usockit_client_sending_thread_create(
			&sending_thread,
			socket_fd,
			threads_result_dest_ptr
		);
	if(ret_status != RET_STATUS_SUCCESS) {
		pthread_cancel(receiving_thread);
		pthread_join(receiving_thread, cross_support_nullptr);

		pthread_cond_destroy(&(threads_result_dest_ptr->cond));
		pthread_mutex_destroy(&(threads_result_dest_ptr->mutex));
		free(threads_result_dest_ptr);

		// TODO: usockit_client_sending_thread_create() error handling
		perror("usockit_client_sending_thread_create");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}


	struct usockit_client_threads_result threads_result;

	pthread_mutex_lock(&(threads_result_dest_ptr->mutex));
	while(threads_result_dest_ptr->result.origin == USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_NONE) {
		pthread_cond_wait(&(threads_result_dest_ptr->cond), &(threads_result_dest_ptr->mutex));
	}

	threads_result = threads_result_dest_ptr->result;

	// cancel threads while they're still (potentially) waiting on the mutex
	pthread_cancel(receiving_thread);
	pthread_cancel(sending_thread);

	pthread_mutex_unlock(&(threads_result_dest_ptr->mutex));

	// join after unlocking mutex so we don't get deadlocks
	pthread_join(receiving_thread, cross_support_nullptr);
	pthread_join(sending_thread, cross_support_nullptr);

	// destroy the threads result destination *after* joining with the threads
	pthread_cond_destroy(&(threads_result_dest_ptr->cond));
	pthread_mutex_destroy(&(threads_result_dest_ptr->mutex));
	free(threads_result_dest_ptr);

	switch(threads_result.origin) {
		case USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_SENDING: {
			const struct usockit_client_sending_thread_result sending_thread_result =
				threads_result.thread_union.sending;

			if(sending_thread_result.status == 0) {
				return USOCKIT_CLIENT_RET_STATUS_SUCCESS_EOF;
			}

			errno = sending_thread_result.status;

			switch(sending_thread_result.func) {
				case USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_READ: {
					// TODO: read() error handling
					perror("read");
					return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
				}
				case USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_WRITE: {
					// TODO: write() error handling
					perror("write");
					return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
				}
				default: {
					cross_support_unreachable();
				}
			}
		}
		case USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_RECEIVING: {
			const struct usockit_client_receiving_thread_result receiving_thread_result =
				threads_result.thread_union.receiving;

			switch(receiving_thread_result.type) {
				case USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_FUCK_OFF: {
					return USOCKIT_CLIENT_RET_STATUS_SUCCESS_FUCK_OFF;
				}
				case USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_READ_FAILURE: {
					// TODO: read() error handling
					perror("read");
					return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
				}
				default: {
					cross_support_unreachable();
				}
			}
		}
		default: {
			cross_support_unreachable();
		}
	}
}
