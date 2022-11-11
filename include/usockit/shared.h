/*
 * Copyright (c) 2022 Michael Federczuk
 * SPDX-License-Identifier: MPL-2.0 AND Apache-2.0
 */

#ifndef USOCKIT_SHARED_H
#define USOCKIT_SHARED_H

#include <stddef.h>
#include <sys/un.h>
#include <usockit/utils.h>

#define USOCKIT_SOCKET_PATHNAME_MAX_LENGTH  (array_size(((struct sockaddr_un){ 0 }).sun_path) - 1)

#endif /* USOCKIT_SHARED_H */
