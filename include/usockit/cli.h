/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_CLI_H
#define USOCKIT_CLI_H

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <usockit/cross_support.h>
#include <usockit/support_types.h>

enum {
	USOCKIT_CLI_CHILD_PROGRAM_ARGV_INIT_CAPACITY = 8,
};

#define USOCKIT_CLI_CHILD_PROGRAM_ARGV_GROWTH_FACTOR 1.5

struct usockit_cli {
	const_cstr_t socket_pathname;

	/**
	 * Whether or not the '--' argument was given.
	 */
	bool child_program;

	/**
	 * Capacity of the vector holding the arguments given after the '--' argument.
	 *
	 * This number includes the null pointer sentinel item.
	 *
	 * Is uninitialized if `child_program` is `false`.
	 */
	size_t child_program_argv_capacity;
	/**
	 * Size of the vector holding arguments given after the '--' argument.
	 *
	 * This number *does not* include the null pointer sentinel item.
	 *
	 * Is uninitialized if `child_program` is `false`.
	 */
	size_t child_program_argv_size;
	/**
	 * Vector holding the arguments given after the '--' argument.
	 * This vector has the same format as the second argument of the `main` function,
	 * i.e.: child_program_argv[child_program_argv_size] is a null pointer.
	 *
	 * Range of [child_program_argv, child_program_argv + child_program_argv_capacity) is allocated data.
	 * Range of [child_program_argv, child_program_argv + child_program_argv_size] is initialized data.
	 *
	 * Is uninitialized if `child_program` is `false`.
	 */
	cstr_t* child_program_argv;
};


// there are all marked as inline since most of them are only used once or are very small

cross_support_nodiscard
static inline struct usockit_cli usockit_cli_create()
	cross_support_attr_always_inline
	cross_support_attr_warn_unused_result;

static inline void usockit_cli_destroy(struct usockit_cli* cli)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all;

static inline void usockit_cli_destroy_definitely_no_init_child_program_argv(struct usockit_cli* cli)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all;

static inline void usockit_cli_destroy_definitely_init_child_program_argv(struct usockit_cli* cli)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all;

cross_support_nodiscard
static inline ret_status_t usockit_cli_init_child_program_argv(struct usockit_cli* cli)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;

cross_support_nodiscard
static inline ret_status_t usockit_cli_append_child_program_arg(struct usockit_cli* cli, cstr_t arg)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;

cross_support_nodiscard
static inline ret_status_t usockit_cli_shrink_to_fit_child_program_argv(struct usockit_cli* cli)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;


static inline struct usockit_cli usockit_cli_create() {
	return (struct usockit_cli){
		.socket_pathname = cross_support_nullptr,

		.child_program = false,
	};
}

static inline void usockit_cli_destroy(struct usockit_cli* const cli) {
	assert(cli != cross_support_nullptr);

	if(cli->child_program) {
		usockit_cli_destroy_definitely_init_child_program_argv(cli);
	} else {
		usockit_cli_destroy_definitely_no_init_child_program_argv(cli);
	}
}

static inline void usockit_cli_destroy_definitely_no_init_child_program_argv(struct usockit_cli* const cli) {
	#ifndef NDEBUG
		assert(cli != cross_support_nullptr);
	#else
		(void)cli;
	#endif

	// empty
}

static inline void usockit_cli_destroy_definitely_init_child_program_argv(struct usockit_cli* const cli) {
	assert(cli != cross_support_nullptr);
	free(cli->child_program_argv);
}

static inline ret_status_t usockit_cli_init_child_program_argv(struct usockit_cli* const cli) {
	assert(cli != cross_support_nullptr);

	errno = 0;
	cstr_t* const tmp = malloc((sizeof *(cli->child_program_argv)) * USOCKIT_CLI_CHILD_PROGRAM_ARGV_INIT_CAPACITY);

	cross_support_if_unlikely(tmp == cross_support_nullptr) {
		return RET_STATUS_FAILURE;
	}

	cli->child_program = true;

	cli->child_program_argv_capacity = USOCKIT_CLI_CHILD_PROGRAM_ARGV_INIT_CAPACITY;
	cli->child_program_argv_size = 0;
	cli->child_program_argv = tmp;
	cli->child_program_argv[0] = cross_support_nullptr;

	return RET_STATUS_SUCCESS;
}

static inline ret_status_t usockit_cli_append_child_program_arg(
	struct usockit_cli* const cli,
	const cstr_t arg
) {
	assert(cli != cross_support_nullptr);
	assert(arg != cross_support_nullptr);

	// +2 here because of
	//     1. the new argument being added
	//     2. the null pointer sentinel item (which is not included in the size, but is in the capacity)
	if((cli->child_program_argv_size + 2) > cli->child_program_argv_capacity) {
		const size_t new_capacity =
			(size_t)((double)(cli->child_program_argv_capacity) * USOCKIT_CLI_CHILD_PROGRAM_ARGV_GROWTH_FACTOR);

		errno = 0;
		cstr_t* const tmp =
			realloc(
				cli->child_program_argv,
				(sizeof *(cli->child_program_argv)) * new_capacity
			);

		cross_support_if_unlikely(tmp == cross_support_nullptr) {
			return RET_STATUS_FAILURE;
		}

		cli->child_program_argv_capacity = new_capacity;
		cli->child_program_argv = tmp;
	}

	cli->child_program_argv[cli->child_program_argv_size] = arg;
	++(cli->child_program_argv_size);
	cli->child_program_argv[cli->child_program_argv_size] = cross_support_nullptr;

	return RET_STATUS_SUCCESS;
}

static inline ret_status_t usockit_cli_shrink_to_fit_child_program_argv(struct usockit_cli* const cli) {
	assert(cli != cross_support_nullptr);

	// +1 here because of the null pointer sentinel item (which is not included in the size, but is in the capacity)
	const size_t fitted_capacity = (cli->child_program_argv_size + 1);

	if(fitted_capacity == cli->child_program_argv_capacity) {
		return RET_STATUS_SUCCESS;
	}

	errno = 0;
	cstr_t* const tmp =
		realloc(
			cli->child_program_argv,
			(sizeof *(cli->child_program_argv)) * fitted_capacity
		);

	cross_support_if_unlikely(tmp == cross_support_nullptr) {
		return RET_STATUS_FAILURE;
	}

	cli->child_program_argv_capacity = fitted_capacity;
	cli->child_program_argv = tmp;

	return RET_STATUS_SUCCESS;
}

#endif /* USOCKIT_CLI_H */
