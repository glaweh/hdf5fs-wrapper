#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
//#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>

#define __USE_LARGEFILE64
#include <sys/stat.h>

#define __USE_GNU
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

int (*_open64)(const char *, int, mode_t) = NULL;
int (*_mkdir)(const char *, mode_t) = NULL;
int (*_unlink)(const char *) = NULL;
int (*___xstat64)(int,const char *,struct stat64*) = NULL;
FILE *(*_fopen)(const char *,const char *) = NULL;

int initialized = 0;

void init() {
    _open64 = dlsym(RTLD_NEXT, "open64");
    _mkdir = dlsym(RTLD_NEXT, "mkdir");
    _unlink = dlsym(RTLD_NEXT, "unlink");
    ___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
    _fopen = dlsym(RTLD_NEXT, "fopen");
    initialized = 1;
}

int mkdir(const char *pathname, mode_t mode) {
    if (initialized == 0) init();
    fprintf(stderr,"mkdir_called: %s\n",pathname);
    return _mkdir(pathname,mode);
}
