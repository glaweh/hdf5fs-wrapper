#include <stdio.h>
#include <limits.h>
extern char* sanitize_path(const char *, char *);
extern void init();
int main(int argc, char *argv[]) {
    int i;
    char processed_path[PATH_MAX];
    if (argc < 2) {
        fprintf(stderr,"usage: %s <path> [path] [path] ...\n",argv[0]);
        return(1);
    }
    init();
    for (i=1;i<argc;i++) {
        if (sanitize_path(argv[i],processed_path) == NULL) {
            fprintf(stderr,"error processing arg '%s'\n",argv[i]);
            continue;
        }
        printf("%s -> %s\n",argv[i],processed_path);
    }
    return(0);
}
