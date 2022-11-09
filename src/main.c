/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <usockit/cli.h>
#include <usockit/client.h>
#include <usockit/cross_support.h>
#include <usockit/server.h>
#include <usockit/shared.h>
#include <usockit/utils.h>
#include <usockit/version.h>

#define USAGE_STRING_SERVER "<socket_path> -- <program> [<args>...]"
#define USAGE_STRING_CLIENT "<socket_path>"


static inline void print_usage(const_cstr_t argv0)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all;

cross_support_nodiscard
static inline int main_server(const_cstr_t argv0, struct usockit_cli* cli)
	cross_support_attr_always_inline
	cross_support_attr_warn_unused_result;

cross_support_nodiscard
static inline int main_client(const_cstr_t socket_pathname)
	cross_support_attr_always_inline
	cross_support_attr_warn_unused_result;


int main(const int argc, const cstr_t* const argv) {
	struct usockit_cli cli = usockit_cli_create();

	// `cross_support_if_unlikely` is used in cases of wrong usage and `cross_support_if_likely` is used in cases of
	// correct usage; we make sure that the happy path is faster

	for(int i = 1; i < argc; ++i) {
		const cstr_t arg = argv[i];

		if(cli.child_program) {
			cross_support_if_unlikely((cli.child_program_argv_size == 0) && str_empty(arg)) {
				usockit_cli_destroy_definitely_init_child_program_argv(&cli);

				fprintf(
					stderr,
					"%s: argument %i: must not be empty\n"
					"usage: %s " USAGE_STRING_SERVER "\n",
					argv[0],
					i,
					argv[0]
				);
				return 9;
			}

			const ret_status_t ret_status = usockit_cli_append_child_program_arg(&cli, arg);
			cross_support_if_unlikely(ret_status != RET_STATUS_SUCCESS) {
				usockit_cli_destroy_definitely_init_child_program_argv(&cli);

				fprintf(stderr, "%s: out of heap memory\n", argv[0]);
				return 101;
			}

			continue;
		}

		if(strequ(arg, "--")) {
			const ret_status_t ret_status = usockit_cli_init_child_program_argv(&cli);

			cross_support_if_unlikely(ret_status != RET_STATUS_SUCCESS) {
				usockit_cli_destroy_definitely_no_init_child_program_argv(&cli);

				fprintf(stderr, "%s: out of heap memory\n", argv[0]);
				return 101;
			}

			continue;
		}

		if(strequ(arg, "--version")) {
			fputs(USOCKIT_VERSION_NAME "\n", stderr);

			usockit_cli_destroy_definitely_no_init_child_program_argv(&cli);
			return 0;
		}

		cross_support_if_unlikely(cli.socket_pathname != cross_support_nullptr) {
			usockit_cli_destroy_definitely_no_init_child_program_argv(&cli);

			fprintf(stderr, "%s: %s: invalid argument: expected either '--' or nothing here\n", argv[0], arg);
			print_usage(argv[0]);
			return 7;
		}

		cross_support_if_likely(!(str_empty(arg))) {
			cli.socket_pathname = arg;
			continue;
		}

		usockit_cli_destroy_definitely_no_init_child_program_argv(&cli);

		if(argc == 2) {
			fprintf(stderr, "%s: argument must not be empty\n", argv[0]);
		} else {
			fprintf(stderr, "%s: argument %i: must not be empty\n", argv[0], i);
		}
		print_usage(argv[0]);
		return 9;
	}

	cross_support_if_unlikely(cli.socket_pathname == cross_support_nullptr) {
		usockit_cli_destroy(&cli);

		fprintf(stderr, "%s: missing argument: <socket_path>\n", argv[0]);
		print_usage(argv[0]);
		return 3;
	}

	cross_support_if_unlikely(strlen(cli.socket_pathname) > USOCKIT_SOCKET_PATHNAME_MAX_LENGTH) {
		usockit_cli_destroy(&cli);

		fprintf(
			stderr,
			"%s: %s: path too long: socket path length must not be more than %zu\n",
			argv[0],
			cli.socket_pathname,
			USOCKIT_SOCKET_PATHNAME_MAX_LENGTH
		);
		return 48;
	}

	if(cli.child_program) {
		const int exit_code = main_server(argv[0], &cli);
		usockit_cli_destroy_definitely_init_child_program_argv(&cli);
		return exit_code;
	} else {
		const int exit_code = main_client(cli.socket_pathname);
		usockit_cli_destroy_definitely_no_init_child_program_argv(&cli);
		return exit_code;
	}
}


static inline int main_client(const const_cstr_t socket_pathname) {
	const enum usockit_client_ret_status ret_status = usockit_client(socket_pathname);

	switch(ret_status) {
		case USOCKIT_CLIENT_RET_STATUS_SUCCESS: {
			return 0;
		}
		case USOCKIT_CLIENT_RET_STATUS_UNKNOWN: {
			return 125;
		}
		// TODO: client error handling
		default: {
			cross_support_unreachable();
		}
	}
}

static inline int main_server(const const_cstr_t argv0, struct usockit_cli* const cli) {
	cross_support_if_unlikely(cli->child_program_argv_size == 0) {
		fprintf(stderr, "%s: missing arguments: <program> [<args>...]\n", argv0);
		print_usage(argv0);
		return 3;
	}

	const ret_status_t ret_status = usockit_cli_shrink_to_fit_child_program_argv(cli);
	cross_support_if_unlikely(ret_status != RET_STATUS_SUCCESS) {
		fprintf(stderr, "%s: out of heap memory\n", argv0);
		return 101;
	}

	const enum usockit_server_ret_status server_ret_status =
		usockit_server(
			cli->socket_pathname,
			#ifndef NDEBUG
			cli->child_program_argv_size,
			#endif
			cli->child_program_argv
		);

	switch(server_ret_status) {
		case USOCKIT_SERVER_RET_STATUS_SUCCESS: {
			return 0;
		}
		case USOCKIT_SERVER_RET_STATUS_UNKNOWN: {
			return 125;
		}
		// TODO: server error handling
		default: {
			cross_support_unreachable();
		}
	}

	return 0;
}

static inline void print_usage(const const_cstr_t argv0) {
	fprintf(
		stderr,
		"usage: %s " USAGE_STRING_SERVER "\n"
		"   or: %s " USAGE_STRING_CLIENT "\n",
		argv0,
		argv0
	);
}
