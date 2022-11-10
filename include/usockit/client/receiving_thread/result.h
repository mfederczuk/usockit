/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_H
#define USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_H

enum usockit_client_receiving_thread_result_type {
	/**
	 * Server sent "fuck off" - a client is already connected.
	 */
	USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_FUCK_OFF,
	USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_READ_FAILURE,
};
struct usockit_client_receiving_thread_result {
	enum usockit_client_receiving_thread_result_type type;

	/**
	 * Is only initialized if `type` is `USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_TYPE_READ_FAILURE`.
	 */
	int read_errno;
};

#endif /* USOCKIT_CLIENT_RECEIVING_THREAD_RESULT_H */
