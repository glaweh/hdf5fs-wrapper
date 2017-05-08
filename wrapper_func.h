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
#ifndef WRAPPER_FUNC_H
#define WRAPPER_FUNC_H
#include "h5fs.h"
#include "khash.h"
#ifdef __LP64__
KHASH_MAP_INIT_INT64(WFILE,h5fd_t*)
KHASH_MAP_INIT_INT64(WDIR,h5dd_t*)
#define PTR2INT int64_t
#else
KHASH_MAP_INIT_INT(WFILE,h5fd_t*)
KHASH_MAP_INIT_INT(WDIR,h5dd_t*)
#define PTR2INT int
#endif
KHASH_MAP_INIT_INT(WFD,h5fd_t*)

extern khash_t(WFILE) * wrapper_files;
extern khash_t(WFD)   * wrapper_fds;
extern khash_t(WDIR)  * wrapper_dirs;

char* path_below_scratch(const char *filename);
int fopen_mode2open_flags(const char * mode);
#endif
