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

#include "path_util.h"

int (*_open64)(const char *, int, mode_t) = NULL;
int (*_mkdir)(const char *, mode_t) = NULL;
int (*_unlink)(const char *) = NULL;
int (*___xstat64)(int,const char *,struct stat64*) = NULL;
FILE *(*_fopen)(const char *,const char *) = NULL;

int initialized = 0;

const char *scratch_base = "./SCRATCH/*.save/*";
char scratch_abs[PATH_MAX];

void init() {
    _open64 = dlsym(RTLD_NEXT, "open64");
    _mkdir = dlsym(RTLD_NEXT, "mkdir");
    _unlink = dlsym(RTLD_NEXT, "unlink");
    ___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
    _fopen = dlsym(RTLD_NEXT, "fopen");
    rel2abs(scratch_base,scratch_abs);
    fprintf(stderr,"scratch_abs: '%s'\n",scratch_abs);
    initialized = 1;
}

int map_filename(const char *filename, char *mapped) {
    rel2abs(filename,mapped);
    int match_index = pathcmp(scratch_abs,mapped);
    if (match_index < 0) return(-1);
    mapped+=match_index-1;
    while (*mapped != 0) {
        if (*mapped == '/') *mapped = '%';
        mapped++;
    }
    return(1);
}

int mkdir(const char *pathname, mode_t mode) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"mkdir_called: %s\n",pathname);
    if (map_filename(pathname,mapped)>=0) {
        fprintf(stderr,"mapped to: %s\n",mapped);
        return(0);
    }
    return _mkdir(pathname,mode);
}

int open64(const char *pathname, int flags, mode_t mode) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"open64_called: %s\n",pathname);
    if (map_filename(pathname,mapped) >= 0) {
        fprintf(stderr,"open64_mapped to: %s\n",mapped);
        return(_open64(mapped,flags,mode));
    }
    return(_open64(pathname,flags,mode));
}

int __xstat64(int version, const char *pathname, struct stat64 *buf) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"__xstat64_called: %s\n",pathname);
    if (map_filename(pathname,mapped) >= 0) {
        fprintf(stderr,"__xstat64_mapped to: %s\n",mapped);
        return(___xstat64(version,mapped,buf));
    }
    return(___xstat64(version,pathname,buf));
}

int unlink(const char *pathname) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"unlink_called: %s\n",pathname);
    if (map_filename(pathname,mapped) >= 0) {
        fprintf(stderr,"unlink_mapped to: %s\n",mapped);
        return(_unlink(mapped));
    }
    return(_unlink(pathname));
}
FILE *fopen(const char *pathname,const char *mode) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"fopen_called: %s\n",pathname);
    if (map_filename(pathname,mapped) >= 0) {
        fprintf(stderr,"fopen_mapped to: %s\n",mapped);
        return(_fopen(mapped,mode));
    }
    return(_fopen(pathname,mode));
}
