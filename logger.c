#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#define LOGMSG_MAX 512
pid_t mypid=0;
void logger_init() {
    mypid = getpid();
    FILE * cmdline_file = fopen("/proc/self/cmdline","r");
    if (cmdline_file == NULL) {
        log_msg_function(__func__,"error opening /proc/self/cmdline");
    } else {
        char cmdline_buffer[1024];
        char *buf_ptr = cmdline_buffer;
        int  to_be_read=1024;
        while ((to_be_read > 0) && (! feof(cmdline_file))) {
            off_t read_bytes=fread(buf_ptr,1,to_be_read,cmdline_file);
            to_be_read-=read_bytes;
            buf_ptr+=read_bytes;
        }
        char process_cmdline[LOGMSG_MAX];
        int i;
        for (i=0;i<1024-to_be_read;i++) {
            process_cmdline[i]=(cmdline_buffer[i] == 0 ? ' ' : cmdline_buffer[i]);
        }
        process_cmdline[1024-to_be_read]=0;
        log_msg_function(__func__,"cmdline: '%s'",process_cmdline);
    }
}
void log_msg_function(const char *function_name, const char *fstring, ...) {
    char msg_buffer[LOGMSG_MAX];
    if (mypid == 0) logger_init();
    int prefix_len=snprintf(msg_buffer,LOGMSG_MAX,"%6d %15.15s | ",mypid,function_name);
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
