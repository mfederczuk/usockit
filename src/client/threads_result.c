/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#include <assert.h>
#include <pthread.h>
#include <usockit/client/threads_result.h>
#include <usockit/cross_support.h>

void usockit_client_threads_dispatch_result(
	struct usockit_client_threads_result_dest* const result_dest_ptr,
	const struct usockit_client_threads_result result
) {
	assert(result_dest_ptr != cross_support_nullptr);

	pthread_mutex_lock(&(result_dest_ptr->mutex));

	if(result_dest_ptr->result.origin == USOCKIT_CLIENT_THREADS_RESULT_ORIGIN_NONE) {
		result_dest_ptr->result = result;
	}

	pthread_mutex_unlock(&(result_dest_ptr->mutex));

	pthread_cond_signal(&(result_dest_ptr->cond));
}
