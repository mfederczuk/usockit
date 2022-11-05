/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLIENT_H
#define USOCKIT_CLIENT_H

#include <usockit/cross_support.h>
#include <usockit/support_types.h>

enum usockit_client_ret_status {
	USOCKIT_CLIENT_RET_STATUS_SUCCESS,
	USOCKIT_CLIENT_RET_STATUS_UNKNOWN, // TODO: remove this
};

cross_support_nodiscard
extern enum usockit_client_ret_status usockit_client(const_cstr_t socket_pathname)
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;

#endif /* USOCKIT_CLIENT_H */
