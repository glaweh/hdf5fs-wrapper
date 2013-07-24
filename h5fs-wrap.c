#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

char *wrapper = PREFIX"/lib/h5fs-wrapper.so";

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,"usage: h5fs-wrap <command> <args> ...\n");
        return(1);
    }
    setenv("LD_PRELOAD",wrapper,1);
    execvp(argv[1],argv+1);
    return(1);
}
