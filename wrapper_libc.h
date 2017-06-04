/*
 * This header defines/includes all symbols necessary to wrap libc symbols.
 * Besides the normal symbols, we also define the macros which are
 * referenced in wrapper-gen/io-calls.c
 */

/*
 * Copyright (c) 2017 Henning Glawe <glaweh@debian.org>
 *
 * This file is part of hdf5fs-wrapper.
 *
 * hdf5fs-wrapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hdf5fs-wrapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with hdf5fs-wrapper.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WRAPPER_LIBC_H
#define WRAPPER_LIBC_H

#define _GNU_SOURCE
#define __USE_LARGEFILE64
#include <dirent.h>
#include <libio.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>

#define PATHNAME char*
#define FD int

#endif
