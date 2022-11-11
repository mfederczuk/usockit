/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLIENT_SENDING_THREAD_SENDING_THREAD_H
#define USOCKIT_CLIENT_SENDING_THREAD_SENDING_THREAD_H

#include <pthread.h>
#include <usockit/client/threads_result.h>
#include <usockit/cross_support.h>
#include <usockit/support_types.h>

cross_support_nodiscard
/**
 * The sending thread will read data from stdin and forward it to the socket.
 *
 * On success, do not obtain the return value of `*thread`, (i.e.: do no call pthread_join() with the second argument
 * not being a null pointer) as it will be undefined.
 * Cancelling the thread before joining is not reliable as the thread may not hit a cancellation point.
 */
extern ret_status_t usockit_client_sending_thread_create(pthread_t* restrict thread,
                                                         int socket_fd,
                                                         struct usockit_client_threads_result_dest* result_dest_ptr)
	                                                         cross_support_attr_nonnull(1, 3)
	                                                         cross_support_attr_warn_unused_result;

#endif /* USOCKIT_CLIENT_SENDING_THREAD_SENDING_THREAD_H */
