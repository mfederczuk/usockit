/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLIENT_RECEIVING_THREAD_RECEIVING_THREAD_H
#define USOCKIT_CLIENT_RECEIVING_THREAD_RECEIVING_THREAD_H

#include <pthread.h>
#include <usockit/client/threads_result.h>
#include <usockit/cross_support.h>
#include <usockit/support_types.h>

cross_support_nodiscard
/**
 * The receiving thread will read data from the socket and perform certain actions. (currently just exiting when the
 * string "fuck off" is received)
 */
extern ret_status_t usockit_client_receiving_thread_create(pthread_t* restrict thread,
                                                           int socket_fd,
                                                           struct usockit_client_threads_result_dest* result_dest_ptr)
	                                                           cross_support_attr_nonnull(1, 3)
	                                                           cross_support_attr_warn_unused_result;

#endif /* USOCKIT_CLIENT_RECEIVING_THREAD_RECEIVING_THREAD_H */
