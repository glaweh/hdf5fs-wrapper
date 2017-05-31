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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/auxv.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include "logger.h"
#include "path_util.h"

#define WRAPPER_BASENAME "h5fs-wrapper.so"
int rel_wrapper_testdir_len = 2;
char * rel_wrapper_testdir[] = {
    "..",         // same dir as h5fs-wrap
    "../../lib",  // or ../lib
};

int main(int argc, char *argv[]) {
    char wrapper_path[PATH_MAX];
    char * exec_filename;
    log_tag="H5FS-WRAP ";
    wrapper_path[0] = 0x00;
    exec_filename = (char *)getauxval(AT_EXECFN);
    if (exec_filename == NULL) {
        LOG_FATAL("unable to get filename of executable\n");
        abort();
    }
    for (int testdir_i=0; testdir_i<rel_wrapper_testdir_len; testdir_i++) {
        char wrapper_path_rel_test[PATH_MAX];
        char wrapper_path_abs_test[PATH_MAX];
        snprintf(wrapper_path_rel_test, PATH_MAX, "%s/%s/"WRAPPER_BASENAME, argv[0], rel_wrapper_testdir[testdir_i]);
        if (rel2abs(wrapper_path_rel_test, wrapper_path_abs_test) != NULL) {
            LOG_DBG2("check wrapper_path: %s", wrapper_path_abs_test);
            if (access(wrapper_path_abs_test, R_OK | X_OK) == 0) {
                strncpy(wrapper_path, wrapper_path_abs_test, PATH_MAX);
                LOG_DBG("found wrapper_path: %s", wrapper_path_abs_test);
                break;
            }
        }
    }
    if (wrapper_path[0] == 0x00) {
        LOG_FATAL("unable to find \""WRAPPER_BASENAME"\"");
        abort();
    }
    if (argc < 2) {
        fprintf(stderr,"usage: h5fs-wrap <command> <args> ...\n");
        return(1);
    }
    setenv("LD_PRELOAD",wrapper_path,1);
    int retval = execvp(argv[1],argv+1);
    if (retval < 0) {
        LOG_ERR("Error calling '%s': %s", argv[1], strerror(errno));
        return(1);
    }
    LOG_FATAL("this should never happen");
    return(1);
}
