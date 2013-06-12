#include <stdio.h>
#include <limits.h>
#include "env_util.h"
int main(int argc, char *argv[]) {
    int i;
    char processed_string[PATH_MAX];
    if (argc < 2) {
        fprintf(stderr,"usage: %s <string> [string] [string] ...\n",argv[0]);
        return(1);
    }
    for (i=1;i<argc;i++) {
        if (strn_env_expand(argv[i],processed_string,PATH_MAX) == -1) {
            fprintf(stderr,"error processing arg '%s'\n",argv[i]);
            continue;
        }
        printf("%s -> %s\n",argv[i],processed_string);
    }
    return(0);
}
