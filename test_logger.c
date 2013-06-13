#include <stdio.h>
#include <limits.h>
#include "logger.h"
int main(int argc, char *argv[]) {
    int i;
    LOG_FATAL("bullshit %s","blahblahblah");
    return(0);
    char processed_string[PATH_MAX];
    if (argc < 2) {
        fprintf(stderr,"usage: %s <string> [string] [string] ...\n",argv[0]);
        return(1);
    }
    return(0);
}
