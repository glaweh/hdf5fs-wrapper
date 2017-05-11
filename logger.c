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
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_info.h"
#define LOGMSG_MAX 512
extern void process_info_init();

FILE * log_stream = NULL;

void log_msg_function(const char *function_name, const char *fstring, ...) {
    char msg_buffer[LOGMSG_MAX];
    if (my_pid == 0) {
        process_info_init();
    }
    if (log_stream == NULL) {
        log_stream = stderr;
        log_msg_function(__func__,"prog: '%s'",my_cmdline_info.argv[0]);
    }

    int prefix_len=snprintf(msg_buffer,LOGMSG_MAX,"%6d %15.15s | ",my_pid,function_name);
    va_list vargs;
    va_start(vargs,fstring);
    int user_len=vsnprintf(msg_buffer+prefix_len,LOGMSG_MAX-prefix_len,fstring,vargs);
    va_end(vargs);
    int msglen = prefix_len+user_len;
    if (msglen > (LOGMSG_MAX-3)) {
        msglen = LOGMSG_MAX-3;
    }
    msg_buffer[msglen]='\n';
    msg_buffer[msglen+1]=0;
    fputs(msg_buffer,log_stream);
}
