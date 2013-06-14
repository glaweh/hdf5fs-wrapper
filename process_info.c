#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "process_info.h"

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

int process_info_init() {
    if (my_pid != 0) return(1);
    my_pid = getpid();
    return(cmdline_info_init());
}
