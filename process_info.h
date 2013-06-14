#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H
#include <sys/types.h>
typedef struct {
    int  argc;
    char data[4096];
    char *argv[512];
} cmdline_info_t;
extern cmdline_info_t my_cmdline_info;
extern pid_t my_pid;
int process_info_init();
#endif
