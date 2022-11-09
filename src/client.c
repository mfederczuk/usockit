/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <usockit/client.h>
#ifndef NDEBUG
	#include <usockit/shared.h>
#endif
#include <usockit/support_types.h>
#include <usockit/utils.h>

#include <stdio.h>  // TODO: remove this. just required for perror(3)


cross_support_nodiscard
static inline enum usockit_client_ret_status usockit_client_connect(int socket_fd, const_cstr_t socket_pathname)
	cross_support_attr_always_inline
	cross_support_attr_warn_unused_result;


enum usockit_client_ret_status usockit_client(const const_cstr_t socket_pathname) {
	#ifndef NDEBUG
	// extra `#ifndef NDEBUG` here so that the strlen(3) call is not executed on release builds
	{
		assert(socket_pathname != NULL);
		const size_t socket_pathname_len = strlen(socket_pathname);
		assert((socket_pathname_len > 0) && (socket_pathname_len <= USOCKIT_SOCKET_PATHNAME_MAX_LENGTH));
	}
	#endif


	// TODO: check socket_pathname


	errno = 0;
	const int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		// TODO: socket(2) error handling
		perror("socket(2)");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}

	const enum usockit_client_ret_status ret_status = usockit_client_connect(socket_fd, socket_pathname);

	close(socket_fd);

	return ret_status;
}


static inline enum usockit_client_ret_status usockit_client_connect(
	const int socket_fd,
	const const_cstr_t socket_pathname
) {
	struct sockaddr_un addr;
	zeroset_lvalue(addr);

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_pathname);

	errno = 0;
	const int ret = connect(socket_fd, (const struct sockaddr*)&addr, sizeof addr);
	if(ret != 0) {
		// TODO: connect(2) error handling
		perror("connect(2)");
		return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
	}


	do {
		unsigned char buffer[1024];
		const ssize_t readc = read(STDIN_FILENO, buffer, (sizeof(buffer) / sizeof(*buffer)));

		if(readc > 0) {
			const ret_status_t ret_status = write_all(socket_fd, buffer, (size_t)readc);
			if(ret_status != RET_STATUS_SUCCESS) {
				// TODO: write(2) error handling
				perror("write(2)");
				return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
			}

			continue;
		}

		if(readc == 0) {
			// EOF
			break;
		}

		if(readc < 0) {
			// TODO: read(2) error handling
			perror("read(2)");
			return USOCKIT_CLIENT_RET_STATUS_UNKNOWN;
		}
	} while(1);

	return USOCKIT_CLIENT_RET_STATUS_SUCCESS;
}
