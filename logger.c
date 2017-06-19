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
#include <limits.h>
#include <stdlib.h>
#define LOGMSG_MAX 512

FILE * logger_stream = NULL;
char   logger_tag[PATH_MAX] = "UNKNOWN";
pid_t  logger_pid = 0;

const char * log_level_str[] = {
    "ACOPALYPSE", // Level 0: should never happen
    "FATAL",      // Level 1
    "ERROR",      // Level 2
    "WARNING",    // Level 3
    "INFO",       // Level 4
    "DEBUG",      // Level 5
    "DEBUG2",     // Level 6
    "DEBUG3",     // Level 7
    "WTF",        // Level 8: should never happen
};

void log_msg_function(const int log_level, const char *function_name, const char *fstring, ...) {
    char msg_buffer[LOGMSG_MAX];
    if (logger_pid == 0) {
        logger_pid = getpid();
    }
    if (logger_stream == NULL) {
        logger_stream = stderr;
    }

    int prefix_len=snprintf(msg_buffer,LOGMSG_MAX,"%-10s %6d %-7s %-20s | ", logger_tag, logger_pid, log_level_str[log_level], function_name);
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
    fputs(msg_buffer, logger_stream);
}

void log_early_msg_function(const int log_level, const char *function_name, const char *fstring, ...) {
    // early-callable log-msg function which cannot use 'wrapped-away' symbols due to dependency loops
    // note that we _have to_ write to stdout here, as fputs is wrapped-away
    char msg_buffer[LOGMSG_MAX];
    if (logger_pid == 0) {
        logger_pid = getpid();
    }

    int prefix_len=snprintf(msg_buffer,LOGMSG_MAX,"%-10s %6d %-7s %-20s | ", logger_tag, logger_pid, log_level_str[log_level], function_name);
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
    puts(msg_buffer);
}

void logger_init(const char * init_logger_tag) {
    strncpy(logger_tag, init_logger_tag, PATH_MAX);
    char program_name[PATH_MAX];
    ssize_t progname_length;
    if ((progname_length = readlink("/proc/self/exe", program_name, PATH_MAX)) >=0 ) {
        program_name[progname_length] = 0;
        LOG_INFO("program name '%s'", program_name);
    } else {
        LOG_ERR("error getting program name");
        abort();
    }
}
