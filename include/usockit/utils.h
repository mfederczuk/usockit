/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_UTILS_H
#define USOCKIT_UTILS_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <usockit/cross_support.h>
#include <usockit/support_types.h>


#define errno_push()  do { \
	                      int usockit_internal_pushed_errno_tmp; \
	                      ((void)(usockit_internal_pushed_errno_tmp = errno))
#define errno_pop()   \
	                      (void)(errno = usockit_internal_pushed_errno_tmp); \
                      } while(0)


cross_support_nodiscard
static inline bool strequ(const_cstr_t s1, const_cstr_t s2)
	cross_support_attr_always_inline
	cross_support_attr_pure
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;

static inline bool strequ(const const_cstr_t s1, const const_cstr_t s2) {
	assert(s1 != cross_support_nullptr);
	assert(s2 != cross_support_nullptr);

	return (strcmp(s1, s2) == 0);
}


cross_support_nodiscard
static inline bool str_empty(const_cstr_t s)
	cross_support_attr_always_inline
	cross_support_attr_pure
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;

static inline bool str_empty(const const_cstr_t s) {
	assert(s != cross_support_nullptr);

	return (*s == '\0');
}


cross_support_nodiscard
static inline ret_status_t write_all(int fd, const void* buf, size_t count)
	cross_support_attr_always_inline
	cross_support_attr_warn_unused_result;

static inline ret_status_t write_all(const int fd, const void* const buf, const size_t count) {
	size_t total_writec = 0;
	do {
		errno = 0;
		const ssize_t tmp_writec = write(fd, ((const unsigned char*)(buf) + total_writec), (count - total_writec));

		if(tmp_writec < 0) {
			return RET_STATUS_FAILURE;
		}

		total_writec += (size_t)tmp_writec;
	} while(total_writec < count);

	return RET_STATUS_SUCCESS;
}

#endif /* USOCKIT_UTILS_H */
