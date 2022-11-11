/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLIENT_THREADS_SENDING_THREAD_RESULT_H
#define USOCKIT_CLIENT_THREADS_SENDING_THREAD_RESULT_H

enum usockit_client_sending_thread_result_func {
	USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_READ,
	USOCKIT_CLIENT_SENDING_THREAD_RESULT_FUNC_WRITE,
};
struct usockit_client_sending_thread_result {
	/**
	 * If 0, the thread received an EOF.
	 * If nonzero, and error occurred and this field is the errno of `func`.
	 */
	int status;
	enum usockit_client_sending_thread_result_func func;
};

#endif /* USOCKIT_CLIENT_THREADS_SENDING_THREAD_RESULT_H */
