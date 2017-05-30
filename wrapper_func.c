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
#include <errno.h>
#include <sys/types.h>
#include "wrapper_func.h"
#include "logger.h"
#include "path_util.h"

khash_t(WFILE) * wrapper_files;
khash_t(WFD)   * wrapper_fds;
khash_t(WDIR)  * wrapper_dirs;
char scratch_base[PATH_MAX] = "./H5FS_SCRATCH";
char scratch_abs[PATH_MAX];

void __attribute__ ((constructor(300))) wrapper_func_init(void) {
    wrapper_files = kh_init(WFILE);
    wrapper_fds   = kh_init(WFD);
    wrapper_dirs  = kh_init(WDIR);
    char * env_ptr;
    env_ptr=getenv("H5FS_BASE");
    if (env_ptr != NULL) {
        strncpy(scratch_base,env_ptr,PATH_MAX);
    }
    rel2abs(scratch_base,scratch_abs);
    LOG_INFO("root path treated by wrapper: '%s/'",scratch_abs);
}

void __attribute__ ((destructor(300)))  wrapper_func_fini(void) {
    khiter_t k;
    for (k = kh_begin(wrapper_fds); k != kh_end(wrapper_fds); ++k) {
        if (kh_exist(wrapper_fds, k)) {
            LOG_DBG("fd still open: %d\n",kh_key(wrapper_fds,k));
            h5fd_close(kh_value(wrapper_fds,k));
        }
    }
    kh_destroy(WFILE,wrapper_files);
    kh_destroy(WFD,wrapper_fds);
    kh_destroy(WDIR,wrapper_dirs);
}

char* path_below_scratch(const char *filename) {
    char mapped0[PATH_MAX];
    rel2abs(filename,mapped0);
    int match_index = pathcmp(scratch_abs,mapped0);
    if (match_index < 0) return(NULL);
    char * mapped = strdup(mapped0+match_index);
    LOG_DBG("%d == pathcmp('%s','%s'), base: '%s'",
            match_index,scratch_abs,mapped,
            mapped);
    return(mapped);
}

int fopen_mode2open_flags(const char * mode) {
    int flags=O_RDONLY;
    if (mode[1] == '+') {
        switch (mode[0]) {
            case 'r' :
                flags = O_RDWR;
                break;
            case 'w' :
                flags = O_RDWR | O_CREAT | O_TRUNC;
                break;
            case 'a' :
                flags = O_APPEND | O_CREAT | O_RDWR;
                break;
        }
    } else {
        switch (mode[0]) {
            case 'r' :
                flags = O_RDONLY;
                break;
            case 'w' :
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case 'a' :
                flags = O_WRONLY | O_CREAT | O_APPEND;
                break;
        }
    }
    return(flags);
}
