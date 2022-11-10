/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLIENT_THREADS_RESULT_H
#define USOCKIT_CLIENT_THREADS_RESULT_H

#include <pthread.h>
#include <usockit/client/receiving_thread/result.h>
#include <usockit/client/sending_thread/result.h>
#include <usockit/cross_support.h>

enum usockit_client_threads_result_origin {
	USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_NONE,

	USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_SENDING,
	USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_RECEIVING,
};
struct usockit_client_threads_result {
	enum usockit_client_threads_result_origin origin;

	/**
	 * Is uninitialized if `origin` is `USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_NONE`.
	 */
	union {
		struct usockit_client_sending_thread_result   sending;
		struct usockit_client_receiving_thread_result receiving;
	} thread_union;
};

struct usockit_client_threads_result_dest {
	pthread_mutex_t mutex;
	struct usockit_client_threads_result result;
	pthread_cond_t cond;
};

extern void usockit_client_threads_dispatch_result(struct usockit_client_threads_result_dest* result_dest_ptr,
                                                   struct usockit_client_threads_result result)
	                                                   cross_support_attr_nonnull(1);

#endif /* USOCKIT_CLIENT_THREADS_RESULT_H */
