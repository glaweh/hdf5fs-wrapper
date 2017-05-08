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
#include <limits.h>
#include <string.h>
#include "path_util.h"
int main(int argc, char *argv[]) {
    int i;
    char pattern_path[PATH_MAX];
    char test_path[PATH_MAX];
    if (argc < 3) {
        fprintf(stderr,"usage: %s <patternpath> <testpath> [testpath] ...\n",argv[0]);
        return(1);
    }
    if (rel2abs(argv[1],pattern_path) == NULL) {
        fprintf(stderr,"error processing arg '%s'\n",argv[1]);
        return(1);
    }
    printf("pattern_path: %s\n",pattern_path);
    for (i=2;i<argc;i++) {
        int match_index;
        if (rel2abs(argv[i],test_path) == NULL) {
            printf("  error: rel2abs processing arg '%s'\n",argv[i]);
            continue;
        }
        match_index=pathcmp(pattern_path,test_path);
        if (match_index < 0) {
            printf("  nomatch: '%s' -> '%s'\n",argv[i],test_path);
        } else {
            if (match_index < strnlen(test_path,PATH_MAX)) {
                printf("  submatch: '%s' -> '%s', remains: '%s'\n",argv[i],test_path,test_path+match_index);
            } else {
                printf("  match: '%s' -> '%s'\n",argv[i],test_path);
            }
        }
    }
    return(0);
}
