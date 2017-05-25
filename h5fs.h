/*
 * Copyright (c) 2013 Henning Glawe <glaweh@debian.org>
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
#ifndef H5FS_H
#define H5FS_H
#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif
#include <sys/types.h>
#include <fcntl.h>
#include "hdir.h"
typedef struct h5fd {
    hdirent_t * hdirent;
    int         append;
    int         rdonly;
    off64_t     offset;
    int         fd;
    FILE*       stream;
} h5fd_t;
typedef struct h5dd h5dd_t;
h5fd_t * h5fd_open(const char * name, int flags, mode_t mode);
int      h5fd_close(h5fd_t * h5fd);
int      h5fs_unlink(const char * name);
int      h5fs_stat(const char * name, struct stat * sstat);
int      h5fs_stat64(const char * name, struct stat64 * sstat);
int      h5fd_fstat(h5fd_t * h5fd, struct stat * sstat);
int      h5fd_fstat64(h5fd_t * h5fd, struct stat64 * sstat);
off64_t  h5fd_seek(h5fd_t * h5fd, off64_t offset, int whence);
ssize_t  h5fd_write(h5fd_t * h5fd, const void * buf, size_t count);
ssize_t  h5fd_read(h5fd_t * h5fd, void *buf, size_t count);
int      h5fd_feof(h5fd_t * h5fd);
ssize_t  h5fd_ftruncate(h5fd_t * h5fd, size_t length);
#endif
