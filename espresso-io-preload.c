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

const char *scratch_base = "./SCRATCH////";
char scratch_abs[PATH_MAX];

char *sanitize_path(const char *orig_path, char *new_path) {
    char tmp_path[PATH_MAX];
    if (orig_path[0] == '/') {
        strncpy(tmp_path,orig_path,PATH_MAX);
    } else {
        if (getcwd(tmp_path,PATH_MAX) == NULL) {
            fprintf(stderr,"error calling getwd\n");
            exit(-1);
        }
        int len=strnlen(tmp_path,PATH_MAX);
        tmp_path[len++]='/';
        tmp_path[len]=0;
        strncpy(tmp_path+len,orig_path, PATH_MAX-len-1);
    }
    int len=strnlen(tmp_path,PATH_MAX);
    int i,j;
    int had_slash=0;
    int dot_count=0;
    for (i=0,j=-1;i<=len;i++) {
        if ((tmp_path[i] == '/') || (i==len)) {
            if (had_slash) {
                if (dot_count == 1) {
                    j--;
                } else if (dot_count == 2) {
                    j-=3;
                    while (new_path[j]!='/') {
                        j--;
                        if (j<0) return(NULL);
                    }
                } else if (dot_count > 2) {
                    return(NULL);
                }
                dot_count=0;
                continue;
            }
            had_slash = 1;
        } else if (tmp_path[i] == '.') {
            dot_count++;
        } else {
            had_slash = 0;
        }
        new_path[++j]=tmp_path[i];
    }
    if ((new_path[j]=='/') && (j > 0)) j--;
    new_path[j+1]=0;
    return new_path;
}

void init() {
    _open64 = dlsym(RTLD_NEXT, "open64");
    _mkdir = dlsym(RTLD_NEXT, "mkdir");
    _unlink = dlsym(RTLD_NEXT, "unlink");
    ___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
    _fopen = dlsym(RTLD_NEXT, "fopen");
    sanitize_path(scratch_base,scratch_abs);
    fprintf(stderr,"scratch_abs: '%s'\n",scratch_abs);
    initialized = 1;
}

int mkdir(const char *pathname, mode_t mode) {
    if (initialized == 0) init();
    fprintf(stderr,"mkdir_called: %s\n",pathname);
    return _mkdir(pathname,mode);
}
