/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#define _POSIX_C_SOURCE 200809L // for strdup(3) and O_CLOEXEC

#include <usockit/cross_support_core.h>

#if CROSS_SUPPORT_LINUX
	// for pipe2(2)
	#define _GNU_SOURCE
#endif

#include <usockit/cross_support_misc.h>

#define USOCKIT_SERVER_PIPE2_SUPPORT  (CROSS_SUPPORT_LINUX_LEAST(2,6,67) && CROSS_SUPPORT_GLIBC_LEAST(2,9))

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <usockit/server.h>
#ifndef NDEBUG
	#include <usockit/shared.h>
#endif
#include <usockit/support_types.h>
#include <usockit/utils.h>

#include <stdio.h> // TODO: remove this. just required for perror(3)

enum {
	PIPE_READ_INDEX  = 0,
	PIPE_WRITE_INDEX = 1,
};

enum usockit_server_child_error_func {
	USOCKIT_CHILD_ERROR_FUNC_DUP2,
	USOCKIT_CHILD_ERROR_FUNC_EXECVE,
};
struct usockit_server_child_error {
	enum usockit_server_child_error_func func;
	int func_errno;
};

struct usockit_server_child_ready_info {
	pthread_mutex_t mutex;
	bool condition;
	pthread_cond_t cond;
};
struct usockit_server_thread_routine_child_wait_arg {
	struct usockit_server_child_ready_info* child_ready_info;
	pid_t* child_pid_ptr;
};
struct usockit_server_thread_routine_accept_arg {
	struct usockit_server_child_ready_info* child_ready_info;
	int socket_fd;
	int* child_stdin_fd_ptr;
};


// usockit_server
// `--- usockit_server_check_socket_pathname
// `--- usockit_server_setup_socket
//      `--- usockit_server_setup_threads
//          `--- usockit_server_thread_routine_child_wait
//          `--- usockit_server_thread_routine_accept
//          |    `--- usockit_server_thread_routine_accept_cleanup_routine
//          `--- usockit_server_setup_child
//               `--- usockit_server_child
//               `--- usockit_server_parent

cross_support_nodiscard
static inline enum usockit_server_ret_status usockit_server_check_socket_pathname(const_cstr_t socket_pathname)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all
	cross_support_attr_warn_unused_result;

cross_support_noreturn
static inline void usockit_server_child(const cstr_t* child_program_argv,
                                        int main_pipe_read_fd,
                                        int reporting_pipe_write_fd)
	                                        cross_support_attr_always_inline
	                                        cross_support_attr_nonnull(1)
	                                        cross_support_attr_noreturn;

cross_support_nodiscard
static inline enum usockit_server_ret_status usockit_server_parent(
	int reporting_pipe_read_fd,
	pid_t child_pid,
	struct usockit_server_child_ready_info* child_ready_info,
	pthread_t child_wait_thread,
	pthread_t accept_thread
) cross_support_attr_always_inline
	  cross_support_attr_nonnull(3)
	  cross_support_attr_warn_unused_result;

static inline void usockit_server_wait_for_child_ready(struct usockit_server_child_ready_info* child_ready_info)
	cross_support_attr_always_inline
	cross_support_attr_nonnull_all;

static void* usockit_server_thread_routine_child_wait(void* arg) cross_support_attr_nonnull_all;

static void  usockit_server_thread_routine_accept_cleanup_routine(void* arg) cross_support_attr_nonnull_all;
static void* usockit_server_thread_routine_accept(void* arg) cross_support_attr_nonnull_all;

cross_support_nodiscard
static inline enum usockit_server_ret_status usockit_server_setup_child(
	const cstr_t* child_program_argv,
	int socket_fd,
	struct usockit_server_child_ready_info* child_read_info,
	pid_t* child_wait_thread_routine_arg_child_pid_ptr,
	int* accept_thread_routine_arg_child_stdin_fd_ptr,
	pthread_t child_wait_thread,
	pthread_t accept_thread
) cross_support_attr_always_inline
	  cross_support_attr_nonnull(1, 3, 4, 5)
	  cross_support_attr_warn_unused_result;

cross_support_nodiscard
static inline enum usockit_server_ret_status usockit_server_setup_threads(const cstr_t* child_program_argv,
                                                                          int socket_fd)
	                                                                          cross_support_attr_always_inline
	                                                                          cross_support_attr_nonnull(1)
	                                                                          cross_support_attr_warn_unused_result;

cross_support_nodiscard
static inline enum usockit_server_ret_status usockit_server_setup_socket(const_cstr_t socket_pathname,
                                                                         const cstr_t* child_program_argv)
	                                                                         cross_support_attr_always_inline
	                                                                         cross_support_attr_nonnull_all
	                                                                         cross_support_attr_warn_unused_result;


enum usockit_server_ret_status usockit_server(
	const const_cstr_t socket_pathname,
	#ifndef NDEBUG
	const size_t child_program_argc,
	#endif
	const cstr_t* const child_program_argv
) {
	#ifndef NDEBUG
	// extra `#ifndef NDEBUG` here so that the strlen(3) call is not executed on release builds
	{
		assert(socket_pathname != cross_support_nullptr);
		const size_t socket_pathname_len = strlen(socket_pathname);
		assert((socket_pathname_len > 0) && (socket_pathname_len <= USOCKIT_SOCKET_PATHNAME_MAX_LENGTH));

		assert(child_program_argc >= 1);

		assert(child_program_argv != cross_support_nullptr);
		assert(!(str_empty(child_program_argv[0])));
		assert(child_program_argv[child_program_argc] == cross_support_nullptr);
	}
	#endif

	const enum usockit_server_ret_status ret_status = usockit_server_check_socket_pathname(socket_pathname);
	if(ret_status != USOCKIT_SERVER_RET_STATUS_SUCCESS) {
		return ret_status;
	}

	return usockit_server_setup_socket(socket_pathname, child_program_argv);
}


static inline enum usockit_server_ret_status usockit_server_setup_socket(
	const const_cstr_t socket_pathname,
	const cstr_t* const child_program_argv
) {
	assert(socket_pathname != cross_support_nullptr);
	assert(child_program_argv != cross_support_nullptr);

	errno = 0;
	const int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		// TODO: socket(2) error handling
		perror("socket(2)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_pathname);

	errno = 0;
	int ret = bind(socket_fd, (const struct sockaddr*)&addr, sizeof addr);
	if(ret != 0) {
		errno_push();
		close(socket_fd);
		errno_pop();

		// TODO: bind(2) error handling
		perror("bind(2)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	errno = 0;
	ret = listen(socket_fd, 1); // we only allow one client connection at a time, so this one extra connection will be
	                            // used to tell the extra client to fuck off
	if(ret != 0) {
		errno_push();
		unlink(socket_pathname);
		close(socket_fd);
		errno_pop();

		// TODO: listen(2) error handling
		perror("listen(2)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	const enum usockit_server_ret_status ret_status =
		usockit_server_setup_threads(
			child_program_argv,
			socket_fd
		);

	unlink(socket_pathname);
	close(socket_fd);

	return ret_status;
}

static inline enum usockit_server_ret_status usockit_server_setup_threads(
	const cstr_t* const child_program_argv,
	const int socket_fd
) {
	assert(child_program_argv != cross_support_nullptr);

	errno = 0;
	struct usockit_server_child_ready_info* const child_ready_info =
		malloc(sizeof (struct usockit_server_child_ready_info));
	cross_support_if_unlikely(child_ready_info == cross_support_nullptr) {
		// TODO: malloc(3) error handling
		perror("malloc(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}
	child_ready_info->condition = false;

	errno = pthread_mutex_init(&(child_ready_info->mutex), cross_support_nullptr);
	if(errno != 0) {
		errno_push();
		free(child_ready_info);
		errno_pop();

		// TODO: pthread_mutex_init(3p) error handling
		perror("pthread_mutex_init(3p)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}

	errno = pthread_cond_init(&(child_ready_info->cond), cross_support_nullptr);
	if(errno != 0) {
		errno_push();
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);
		errno_pop();

		// TODO: pthread_cond_init(3p) error handling
		perror("pthread_cond_init(3p)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	errno = 0;
	struct usockit_server_thread_routine_child_wait_arg* const child_wait_thread_routine_arg =
		calloc(1, sizeof (struct usockit_server_thread_routine_child_wait_arg));
	cross_support_if_unlikely(child_wait_thread_routine_arg == cross_support_nullptr) {
		errno_push();
		pthread_cond_destroy(&(child_ready_info->cond));
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);
		errno_pop();

		// TODO: malloc(3) error handling
		perror("malloc(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}

	errno = 0;
	child_wait_thread_routine_arg->child_pid_ptr = malloc(sizeof *(child_wait_thread_routine_arg->child_pid_ptr));
	cross_support_if_unlikely(child_wait_thread_routine_arg->child_pid_ptr == cross_support_nullptr) {
		errno_push();

		free(child_wait_thread_routine_arg);

		pthread_cond_destroy(&(child_ready_info->cond));
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);

		errno_pop();

		// TODO: malloc(3) error handling
		perror("malloc(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}

	child_wait_thread_routine_arg->child_ready_info = child_ready_info;


	errno = 0;
	struct usockit_server_thread_routine_accept_arg* const accept_thread_routine_arg =
		calloc(1, sizeof (struct usockit_server_thread_routine_accept_arg));
	cross_support_if_unlikely(accept_thread_routine_arg == cross_support_nullptr) {
		errno_push();

		free(child_wait_thread_routine_arg->child_pid_ptr);
		free(child_wait_thread_routine_arg);

		pthread_cond_destroy(&(child_ready_info->cond));
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);

		errno_pop();

		// TODO: malloc(3) error handling
		perror("malloc(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}

	errno = 0;
	accept_thread_routine_arg->child_stdin_fd_ptr = malloc(sizeof *(accept_thread_routine_arg->child_stdin_fd_ptr));
	cross_support_if_unlikely(accept_thread_routine_arg->child_stdin_fd_ptr == cross_support_nullptr) {
		errno_push();

		free(accept_thread_routine_arg);

		free(child_wait_thread_routine_arg->child_pid_ptr);
		free(child_wait_thread_routine_arg);

		pthread_cond_destroy(&(child_ready_info->cond));
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);

		errno_pop();

		// TODO: malloc(3) error handling
		perror("malloc(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}

	accept_thread_routine_arg->child_ready_info = child_ready_info;
	accept_thread_routine_arg->socket_fd = socket_fd;


	pthread_t child_wait_thread;
	errno =
		pthread_create(
			&child_wait_thread,
			cross_support_nullptr,
			&usockit_server_thread_routine_child_wait,
			child_wait_thread_routine_arg
		);
	if(errno != 0) {
		errno_push();

		free(accept_thread_routine_arg->child_stdin_fd_ptr);
		free(accept_thread_routine_arg);

		free(child_wait_thread_routine_arg->child_pid_ptr);
		free(child_wait_thread_routine_arg);

		pthread_cond_destroy(&(child_ready_info->cond));
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);

		errno_pop();

		// TODO: pthread_create(3) error handling
		perror("pthread_create(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	pthread_t accept_thread;
	errno =
		pthread_create(
			&accept_thread,
			cross_support_nullptr,
			&usockit_server_thread_routine_accept,
			accept_thread_routine_arg
		);
	if(errno != 0) {
		errno_push();

		pthread_cancel(child_wait_thread);
		pthread_join(child_wait_thread, cross_support_nullptr);

		free(accept_thread_routine_arg->child_stdin_fd_ptr);
		free(accept_thread_routine_arg);

		free(child_wait_thread_routine_arg->child_pid_ptr);
		free(child_wait_thread_routine_arg);

		pthread_cond_destroy(&(child_ready_info->cond));
		pthread_mutex_destroy(&(child_ready_info->mutex));
		free(child_ready_info);

		errno_pop();

		// TODO: pthread_create(3) error handling
		perror("pthread_create(3)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	const enum usockit_server_ret_status ret_status =
		usockit_server_setup_child(
			child_program_argv,
			socket_fd,
			child_ready_info,
			child_wait_thread_routine_arg->child_pid_ptr,
			accept_thread_routine_arg->child_stdin_fd_ptr,
			child_wait_thread,
			accept_thread
		);

	pthread_cancel(accept_thread);
	pthread_join(accept_thread, cross_support_nullptr);

	pthread_cancel(child_wait_thread);
	pthread_join(child_wait_thread, cross_support_nullptr);

	free(accept_thread_routine_arg->child_stdin_fd_ptr);
	free(accept_thread_routine_arg);

	free(child_wait_thread_routine_arg->child_pid_ptr);
	free(child_wait_thread_routine_arg);

	pthread_cond_destroy(&(child_ready_info->cond));
	pthread_mutex_destroy(&(child_ready_info->mutex));
	free(child_ready_info);

	return ret_status;
}

static inline enum usockit_server_ret_status usockit_server_setup_child(
	const cstr_t* const child_program_argv,
	const int socket_fd,
	struct usockit_server_child_ready_info* const child_read_info,
	pid_t* const child_wait_thread_routine_arg_child_pid_ptr,
	int* const accept_thread_routine_arg_child_stdin_fd_ptr,
	pthread_t child_wait_thread,
	pthread_t accept_thread
) {
	assert(child_program_argv != cross_support_nullptr);
	assert(child_read_info != cross_support_nullptr);
	assert(child_wait_thread_routine_arg_child_pid_ptr != cross_support_nullptr);
	assert(accept_thread_routine_arg_child_stdin_fd_ptr != cross_support_nullptr);

	// the main pipe is is used for writing to the child process' stdin
	int main_pipe[2];

	errno = 0;
	int ret = pipe(main_pipe);
	if(ret != 0) {
		// TODO: pipe(2) error handling
		perror("pipe(2)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}


	// the reporting pipe is for the child reporting either error or success back to the parent.
	// if the child encounters an error, it will send a struct `usockit_server_child_error` over the pipe, signaling to
	// the parent that an error occurred.
	// this pipe is also opened with the close-on-exec flag, which means that when the child successfully calls one of
	// the exec(3)-family functions, the parent will receive an EOF without any data, which signals to the parent that
	// everything went ok
	int reporting_pipe[2];

	#if USOCKIT_SERVER_PIPE2_SUPPORT
		errno = 0;
		ret = pipe2(reporting_pipe, O_CLOEXEC);
		if(ret != 0) {
			errno_push();
			close(main_pipe[PIPE_WRITE_INDEX]);
			close(main_pipe[PIPE_READ_INDEX]);
			errno_pop();

			// TODO: pipe2(2) error handling
			perror("pipe2(2)");
			return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
		}
	#else
		errno = 0;
		ret = pipe(reporting_pipe);
		if(ret != 0) {
			errno_push();
			close(main_pipe[PIPE_WRITE_INDEX]);
			close(main_pipe[PIPE_READ_INDEX]);
			errno_pop();

			// TODO: pipe(2) error handling
			perror("pipe(2)");
			return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
		}

		errno = 0;
		ret = fcntl(reporting_pipe[PIPE_WRITE_INDEX], F_SETFD, O_CLOEXEC);
		if(ret != 0) {
			errno_push();

			close(reporting_pipe[PIPE_WRITE_INDEX]);
			close(reporting_pipe[PIPE_READ_INDEX]);

			close(main_pipe[PIPE_WRITE_INDEX]);
			close(main_pipe[PIPE_READ_INDEX]);

			errno_pop();

			// TODO: fcntl(2) error handling
			perror("fcntl(2)");
			return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
		}
	#endif


	errno = 0;
	const pid_t child_pid = fork();
	if(child_pid == -1) {
		errno_push();

		close(reporting_pipe[PIPE_WRITE_INDEX]);
		close(reporting_pipe[PIPE_READ_INDEX]);

		close(main_pipe[PIPE_WRITE_INDEX]);
		close(main_pipe[PIPE_READ_INDEX]);

		errno_pop();

		// TODO: fork(2) error handling
		perror("fork(2)");
		return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
	}

	if(child_pid == 0) {
		// reporting pipe read end, main pipe write end and the socket is not needed by the child
		close(reporting_pipe[PIPE_READ_INDEX]);
		close(main_pipe[PIPE_WRITE_INDEX]);
		close(socket_fd);

		// noreturn
		usockit_server_child(
			child_program_argv,
			main_pipe[PIPE_READ_INDEX],
			reporting_pipe[PIPE_WRITE_INDEX]
		);
	} else {
		// reporting pipe write end and main pipe read end is not needed by the parent
		close(reporting_pipe[PIPE_WRITE_INDEX]);
		close(main_pipe[PIPE_READ_INDEX]);

		*child_wait_thread_routine_arg_child_pid_ptr = child_pid;
		*accept_thread_routine_arg_child_stdin_fd_ptr = main_pipe[PIPE_WRITE_INDEX];

		const enum usockit_server_ret_status ret_status =
			usockit_server_parent(
				reporting_pipe[PIPE_READ_INDEX],
				child_pid,
				child_read_info,
				child_wait_thread,
				accept_thread
			);

		close(reporting_pipe[PIPE_READ_INDEX]);
		close(main_pipe[PIPE_WRITE_INDEX]);

		return ret_status;
	}
}

static void usockit_server_thread_routine_accept_cleanup_routine(void* arg) {
	const int client_fd = *(const int*)arg;

	close(client_fd);
}

static void* usockit_server_thread_routine_accept(void* const arg_ptr) {
	assert(arg_ptr != cross_support_nullptr);

	const struct usockit_server_thread_routine_accept_arg arg =
		*(const struct usockit_server_thread_routine_accept_arg*)arg_ptr;

	usockit_server_wait_for_child_ready(arg.child_ready_info);

	const int child_stdin_fd = *(arg.child_stdin_fd_ptr);

	do {
		errno = 0;
		int client_fd = accept(arg.socket_fd, cross_support_nullptr, cross_support_nullptr);
		if(client_fd == -1) {
			// TODO: accept(2) error handling
			perror("accept(2)");
			continue;
		}

		// TODO: here we need to spawn yet another thread for the first accepted connection, any subsequent accepted
		//       connections are immediately closed. in the future we're gonna have a proper protocol where server and
		//       client send messages to another, and for these subsequent connections we're gonna send the "fuck off"
		//       message to say that a connection is currently already open.
		//       just for the joke i actually wan't to call the message type something like
		//       `USOCKIT_PROTOCOL_MESSAGE_TYPE_FUCK_OFF`

		pthread_cleanup_push(usockit_server_thread_routine_accept_cleanup_routine, &client_fd);

		do {
			unsigned char buffer[1024];
			const ssize_t readc = read(client_fd, buffer, array_size(buffer));

			if(readc > 0) {
				const ret_status_t ret_status = write_all(child_stdin_fd, buffer, (size_t)readc);
				if(ret_status != RET_STATUS_SUCCESS) {
					// TODO: write(2) error handling
					break;
				}
			}

			if(readc == 0) {
				break;
			}

			if(readc < 0) {
				// TODO: read(2) error handling
				break;
			}
		} while(true);

		pthread_cleanup_pop(1);
	} while(true);

	return cross_support_nullptr;
}

static void* usockit_server_thread_routine_child_wait(void* const arg_ptr) {
	assert(arg_ptr != cross_support_nullptr);

	const struct usockit_server_thread_routine_child_wait_arg arg =
		*(const struct usockit_server_thread_routine_child_wait_arg*)arg_ptr;

	usockit_server_wait_for_child_ready(arg.child_ready_info);

	waitpid(*(arg.child_pid_ptr), cross_support_nullptr, 0);

	return cross_support_nullptr;
}

static inline void usockit_server_wait_for_child_ready(struct usockit_server_child_ready_info* const child_ready_info) {
	pthread_mutex_lock(&(child_ready_info->mutex));
	while(!(child_ready_info->condition)) {
		pthread_cond_wait(&(child_ready_info->cond), &(child_ready_info->mutex));
	}
	pthread_mutex_unlock(&(child_ready_info->mutex));
}

static inline enum usockit_server_ret_status usockit_server_parent(
	const int reporting_pipe_read_fd,
	const pid_t child_pid,
	struct usockit_server_child_ready_info* const child_ready_info,
	pthread_t child_wait_thread,
	pthread_t accept_thread
) {
	assert(child_ready_info != cross_support_nullptr);

	struct usockit_server_child_error child_error;
	ssize_t readc = read(reporting_pipe_read_fd, &child_error, sizeof child_error);

	switch(readc) {
		case 0: {
			// EOF; the child successfully called one of the exec(3)-family functions
			break;
		}
		case sizeof child_error: {
			// we got data; the child encountered an error

			// *very* unlikely that the child is still alive at this point, but better safe than sorry
			waitpid(child_pid, cross_support_nullptr, 0);

			switch(child_error.func) {
				case USOCKIT_CHILD_ERROR_FUNC_DUP2: {
					// TODO: dup2(2) error handling
					errno = child_error.func_errno;
					perror("dup2(2)");
					return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
				}
				case USOCKIT_CHILD_ERROR_FUNC_EXECVE: {
					// TODO: execve(2) error handling
					errno = child_error.func_errno;
					perror("execve(2)");
					return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
				}
				default: {
					cross_support_unreachable();
				}
			}
		}
		default: {
			// either error when trying to read (readc == -1) or not enough data read (readc < (sizeof child_error))

			// TODO: kill the child

			// TODO: read(2) error handling
			perror("read(2)");
			return USOCKIT_SERVER_RET_STATUS_UNKNOWN;
		}
	}


	pthread_mutex_lock(&(child_ready_info->mutex));
	child_ready_info->condition = true;
	pthread_mutex_unlock(&(child_ready_info->mutex));
	pthread_cond_broadcast(&(child_ready_info->cond));


	// ============================================================================================================== //
	//                                                                                                                //
	//   Main program runs now.                                                                                       //
	//   The accept_thread is accepting connections from the socket and is forwarding it to the child's stdin.        //
	//   Meanwhile the child_wait_thread is waiting until the child dies. When it does, we (the main thread) join     //
	//   with it and cancel the accept_thread.                                                                        //
	//                                                                                                                //
	// ============================================================================================================== //


	pthread_join(child_wait_thread, cross_support_nullptr);

	pthread_cancel(accept_thread);
	pthread_join(accept_thread, cross_support_nullptr);

	return USOCKIT_SERVER_RET_STATUS_SUCCESS;
}

static inline void usockit_server_child(
	const cstr_t* const child_program_argv,
	const int main_pipe_read_fd,
	const int reporting_pipe_write_fd
) {
	assert(child_program_argv != cross_support_nullptr);

	errno = 0;
	const int new_fd = dup2(main_pipe_read_fd, STDIN_FILENO);
	if(new_fd != STDIN_FILENO) {
		errno_push();
		close(main_pipe_read_fd);
		errno_pop();

		struct usockit_server_child_error error = {
			.func = USOCKIT_CHILD_ERROR_FUNC_DUP2,
			.func_errno = errno,
		};
		write(reporting_pipe_write_fd, &error, sizeof error);
		close(reporting_pipe_write_fd);
		// no error handling here, we just hope that it works >.<

		// one of the only times we ever use exit(3)
		exit(EXIT_FAILURE);
	}

	// no need for this file descriptor anymore, we have stdin now
	close(main_pipe_read_fd);


	errno = 0;
	execvp(child_program_argv[0], child_program_argv);

	struct usockit_server_child_error error = {
		.func = USOCKIT_CHILD_ERROR_FUNC_EXECVE,
		.func_errno = errno,
	};
	write(reporting_pipe_write_fd, &error, sizeof error);
	close(reporting_pipe_write_fd);
	// no error handling here, we just hope that it works >.<

	// one of the only times we ever use exit(3)
	exit(EXIT_FAILURE);
}

static inline enum usockit_server_ret_status usockit_server_check_socket_pathname(const const_cstr_t socket_pathname) {
	assert(socket_pathname != cross_support_nullptr);

	// TODO: check if file with pathname `socket_pathname` already exists. fail if that is the case

	errno = 0;
	const cstr_t socket_pathname_dup = strdup(socket_pathname);

	cross_support_if_unlikely(socket_pathname_dup == cross_support_nullptr) {
		return USOCKIT_SERVER_RET_STATUS_OUT_OF_MEMORY;
	}

	const const_cstr_t socket_parent_dir_pathname = dirname(socket_pathname_dup);

	// TODO: check if we have write permissions to `socket_parent_dir_pathname` and if it is a directory in the first
	//       place
	(void)socket_parent_dir_pathname;

	free(socket_pathname_dup);

	return USOCKIT_SERVER_RET_STATUS_SUCCESS;
}
