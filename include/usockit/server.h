/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_SERVER_H
#define USOCKIT_SERVER_H

#include <stddef.h>
#include <usockit/cross_support.h>
#include <usockit/support_types.h>

enum usockit_server_ret_status {
	USOCKIT_SERVER_RET_STATUS_SUCCESS,
	USOCKIT_SERVER_RET_STATUS_OUT_OF_MEMORY,
	USOCKIT_SERVER_RET_STATUS_UNKNOWN, // TODO: remove this
};

cross_support_nodiscard
extern enum usockit_server_ret_status usockit_server(const_cstr_t socket_pathname,
                                                     #ifndef NDEBUG
                                                     size_t child_program_argc,
                                                     #endif
                                                     const cstr_t* child_program_argv)
	                                                     #ifndef NDEBUG
	                                                     cross_support_attr_nonnull(1, 3)
	                                                     #else
	                                                     cross_support_attr_nonnull_all
	                                                     #endif
	                                                     cross_support_attr_warn_unused_result;

#endif /* USOCKIT_SERVER_H */
