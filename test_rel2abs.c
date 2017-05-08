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
#include "path_util.h"
int main(int argc, char *argv[]) {
    int i;
    char processed_path[PATH_MAX];
    if (argc < 2) {
        fprintf(stderr,"usage: %s <path> [path] [path] ...\n",argv[0]);
        return(1);
    }
    for (i=1;i<argc;i++) {
        if (rel2abs(argv[i],processed_path) == NULL) {
            fprintf(stderr,"error processing arg '%s'\n",argv[i]);
            continue;
        }
        printf("%s -> %s\n",argv[i],processed_path);
    }
    return(0);
}
