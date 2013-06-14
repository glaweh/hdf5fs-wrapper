#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include "process_info.h"
#define LOGMSG_MAX 512
int logger_init() {
    if (process_info_init() < 0) {
        fprintf(stderr,"%s | error initializing proc info\n",__func__);
        return(0);
    }
    log_msg_function(__func__,"prog: '%s'",my_cmdline_info.argv[0]);
    return(1);
}
void log_msg_function(const char *function_name, const char *fstring, ...) {
    char msg_buffer[LOGMSG_MAX];
    if (my_pid == 0) logger_init();
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
    fputs(msg_buffer,stderr);
}
