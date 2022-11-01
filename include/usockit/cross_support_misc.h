/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

// Version 1.1.0 (https://github.com/mfederczuk/cross-support)

#ifndef CROSS_SUPPORT_MISC_H
#define CROSS_SUPPORT_MISC_H 1

#if (!(defined(CROSS_SUPPORT_CORE_H)) && !(CROSS_SUPPORT_CORE_H + 0))
	#error Include 'cross_support_core.h' before 'cross_support_misc.h'
#endif

// === kernels / operating systems ================================================================================== //

#if CROSS_SUPPORT_LINUX
	#include <linux/version.h>

	#define CROSS_SUPPORT_LINUX_LEAST(major, patchlevel, sublevel) \
		(LINUX_VERSION_CODE >= KERNEL_VERSION(major, patchlevel, sublevel))
#else
	#define CROSS_SUPPORT_LINUX_LEAST(major, patchlevel, sublevel)  0
#endif

// === libraries ==================================================================================================== //

#if defined(__has_include)
	#if __has_include(<features.h>)
		#include <features.h>
	#endif
#endif

#if (!(defined(__GLIBC__))       && !(defined(__GLIBC_MINOR__)) && \
     !(defined(__GNU_LIBRARY__)) && !(defined(__GNU_LIBRARY_MINOR__)))

	#if CROSS_SUPPORT_CXX
		#include <climits>
	#else
		#include <limits.h>
	#endif
#endif

#define CROSS_SUPPORT_GLIBC_LEAST(major, minor) \
	(                                                           \
		(                                                       \
			((major) >= 6)                                      \
			&&                                                  \
			(                                                   \
				((__GLIBC__ + 0) > (major))                     \
				||                                              \
				(                                               \
					((__GLIBC__       + 0) == (major))          \
					&&                                          \
					((__GLIBC_MINOR__ + 0) >= (minor))          \
				)                                               \
			)                                                   \
		)                                                       \
		||                                                      \
		(                                                       \
			(((major) < 6))                                     \
			&&                                                  \
			(                                                   \
				((__GNU_LIBRARY__ + 0) > (major))               \
				||                                              \
				(                                               \
					((__GNU_LIBRARY__       + 0) == (major))    \
					&&                                          \
					((__GNU_LIBRARY_MINOR__ + 0) >= (minor))    \
				)                                               \
			)                                                   \
		)                                                       \
	)

// === C/C++ compatibility ========================================================================================== //

#if CROSS_SUPPORT_CXX11
	#define cross_support_nullptr  nullptr
#elif CROSS_SUPPORT_CXX
	#include <cstddef>
	#define cross_support_nullptr  NULL
#elif CROSS_SUPPORT_C23
	#define cross_support_nullptr  nullptr
#else
	#include <stddef.h>
	#define cross_support_nullptr  NULL
#endif

// === attributes =================================================================================================== //

#if (defined(cross_support_noreturn) && !CROSS_SUPPORT_CXX11 && !CROSS_SUPPORT_C23 && CROSS_SUPPORT_C11)
	#include <stdnoreturn.h>
	#undef  cross_support_noreturn
	#define cross_support_noreturn  noreturn
#endif

// === UB optimization ============================================================================================== //

#if CROSS_SUPPORT_CXX23
	#include <utility>
	#define cross_support_unreachable()  ::std::unreachable()
#elif CROSS_SUPPORT_C23
	#include <stddef.h>
	#define cross_support_unreachable()  unreachable()
#elif (CROSS_SUPPORT_GCC_LEAST(4,5) || CROSS_SUPPORT_CLANG)
	#define cross_support_unreachable()  __builtin_unreachable()
#elif CROSS_SUPPORT_MSVC
	#define cross_support_unreachable()  __assume(0)
#else
	#if CROSS_SUPPORT_CXX
		#include <cassert>
	#else
		#include <assert.h>
	#endif

	// using `assert` because some implementations have noreturn optimizations
	#define cross_support_unreachable()  assert(0)
#endif

// === branch optimization ========================================================================================== //

#if (defined(cross_support_if_likely) && !CROSS_SUPPORT_CXX20 && (CROSS_SUPPORT_GCC_LEAST(3,0) || CROSS_SUPPORT_CLANG))
	#include <stdbool.h>
	#undef  cross_support_if_likely
	#define cross_support_if_likely(condition)    if(__builtin_expect((long)(bool)(condition), (long)(true)))
#endif

#if (defined(cross_support_if_unlikely) && !CROSS_SUPPORT_CXX20 && (CROSS_SUPPORT_GCC_LEAST(3,0) || CROSS_SUPPORT_CLANG))
	#include <stdbool.h>
	#undef  cross_support_if_unlikely
	#define cross_support_if_unlikely(condition)  if(__builtin_expect((long)(bool)(condition), (long)(false)))
#endif

#endif /* CROSS_SUPPORT_MISC_H */
