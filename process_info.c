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
#include <string.h>
#include <unistd.h>
#include "process_info.h"
#include <stdlib.h>
#include "logger.h"

pid_t          my_pid = 0;
cmdline_info_t my_cmdline_info;

int cmdline_info_init() {
    FILE * cmdline_file = fopen("/proc/self/cmdline","r");
    if (cmdline_file == NULL) {
        fprintf(stderr,"error opening /proc/self/cmdline");
        return(-1);
    }
    char * buf_ptr=my_cmdline_info.data;
    int to_be_read=sizeof(my_cmdline_info.data);
    while ((to_be_read > 0) && (! feof(cmdline_file))) {
        off_t read_bytes=fread(buf_ptr,1,to_be_read,cmdline_file);
        to_be_read-=read_bytes;
        buf_ptr+=read_bytes;
    }
    fclose(cmdline_file);
    my_cmdline_info.argc=1;
    my_cmdline_info.argv[0]=my_cmdline_info.data;
    int cmdline_length = sizeof(my_cmdline_info.data)-to_be_read;
    int i;
    for (i=0; i<cmdline_length; i++) {
        if (my_cmdline_info.data[i]==0) {
            my_cmdline_info.argv[my_cmdline_info.argc]=my_cmdline_info.data+i+1;
            my_cmdline_info.argc++;
        }
    }
    return(1);
}

void __attribute__ ((constructor(220))) process_info_init() {
    if (my_pid != 0) return;
    my_pid = getpid();
    if (cmdline_info_init() < 0) abort();
    LOG_INFO("program name '%s'", my_cmdline_info.argv[0]);
}
