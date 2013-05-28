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
#include <fcntl.h>
#include <stdarg.h>

#include "path_util.h"

int (*_open64)(const char *, int, ...) = NULL;
int (*_mkdir)(const char *, mode_t) = NULL;
int (*_unlink)(const char *) = NULL;
int (*___xstat64)(int,const char *,struct stat64*) = NULL;
FILE *(*_fopen)(const char *,const char *) = NULL;
int (*_close)(int)=NULL;
int (*_fclose)(FILE *)=NULL;

int initialized = 0;

const char *scratch_base = "./SCRATCH/*.save/*";
char scratch_abs[PATH_MAX];

#define HANDLES_MAX 1024

int   nfiles   = 0;
int   handle_table[HANDLES_MAX];
FILE* file_table[HANDLES_MAX];
char  filename_table[HANDLES_MAX*PATH_MAX];
int   basename_idx[HANDLES_MAX];

void init() {
    _open64    = dlsym(RTLD_NEXT, "open64");
    _mkdir     = dlsym(RTLD_NEXT, "mkdir");
    _unlink    = dlsym(RTLD_NEXT, "unlink");
    ___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
    _fopen     = dlsym(RTLD_NEXT, "fopen");
    _close     = dlsym(RTLD_NEXT, "close");
    _fclose    = dlsym(RTLD_NEXT, "fclose");
    rel2abs(scratch_base,scratch_abs);
    fprintf(stderr,"scratch_abs: '%s'\n",scratch_abs);
    int i;
    for (i=0;i<HANDLES_MAX;i++) handle_table[i]=0;
    for (i=0;i<HANDLES_MAX;i++) file_table[i]=NULL;
    for (i=0;i<HANDLES_MAX*PATH_MAX;i++) filename_table[i]=0;
    for (i=0;i<HANDLES_MAX;i++) basename_idx[i]=0;
    initialized = 1;
}

int map_filename(const char *filename, char *mapped) {
    rel2abs(filename,mapped);
    int match_index = pathcmp(scratch_abs,mapped);
    if (match_index < 0) return(-1);
    char *rewrite=mapped+match_index-1;
    while (*rewrite != 0) {
        if (*rewrite == '/') *rewrite = '%';
        rewrite++;
    }
    fprintf(stderr,"map_filename: %d == pathcmp('%s','%s')\n",match_index,scratch_abs,mapped);
    return(match_index);
}

int mkdir(const char *pathname, mode_t mode) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"mkdir_called: '%s'\n",pathname);
    if (map_filename(pathname,mapped)>=0) {
        fprintf(stderr,"mkdir_mapped '%s' to: '%s'\n",pathname,mapped);
        return(0);
    }
    return _mkdir(pathname,mode);
}

char rdwr_str[5] ="RDWR";
char rdon_str[7]="RDONLY";
char wron_str[7]="WRONLY";
char WTF_str[4]="WTF";

int open64(const char *pathname, int flags, ...) {
    va_list argp;
    mode_t mode=-1;
    char mapped[PATH_MAX];
    int match_idx;
    if (initialized == 0) init();
    if (flags & O_CREAT) {
        va_start(argp,flags);
        mode = va_arg(argp, mode_t);
        va_end(argp);
    }

    fprintf(stderr,"open64_called: '%s' '%s'\n",pathname,scratch_abs);
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
        char *modestr=WTF_str;
        if ((flags & O_ACCMODE) == O_RDWR) {
            modestr=rdwr_str;
        } else if ((flags & O_ACCMODE) == O_RDONLY) {
            modestr=rdon_str;
        } else if ((flags & O_ACCMODE) == O_WRONLY) {
            modestr=wron_str;
        }
        fprintf(stderr,"open64_mapped: %s, 0x%x(%s), nfiles: %d\n",mapped,flags,modestr,nfiles);
        int fd=_open64(mapped,flags,mode);
        if (fd < 0) return(fd);
        if (fd > HANDLES_MAX) {
            fprintf(stderr,"filehandle out of range\n");
            exit(-1);
        }
        nfiles++;
        handle_table[fd]=1;
        strncpy(filename_table+PATH_MAX*fd,mapped,PATH_MAX);
        basename_idx[fd]=match_idx;
        return(fd);
    }
    return(_open64(pathname,flags,mode));
}

int __xstat64(int version, const char *pathname, struct stat64 *buf) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"__xstat64_called: '%s'\n",pathname);
    if (map_filename(pathname,mapped) >= 0) {
        fprintf(stderr,"__xstat64_mapped: %s\n",mapped);
        return(___xstat64(version,mapped,buf));
    }
    return(___xstat64(version,pathname,buf));
}

int unlink(const char *pathname) {
    char mapped[PATH_MAX];
    if (initialized == 0) init();
    fprintf(stderr,"unlink_called: '%s'\n",pathname);
    if (map_filename(pathname,mapped) >= 0) {
        fprintf(stderr,"unlink_mapped to: '%s'\n",mapped);
        return(_unlink(mapped));
    }
    return(_unlink(pathname));
}
FILE *fopen(const char *pathname,const char *mode) {
    char mapped[PATH_MAX];
    int match_idx;
    if (initialized == 0) init();
    fprintf(stderr,"fopen_called: %s\n",pathname);
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
        fprintf(stderr,"fopen_mapped to: %s, mode: %s, nfiles: %d\n",mapped,mode,nfiles);
        FILE *file=_fopen(mapped,mode);
        if (file == NULL)
            return(NULL);
        nfiles++;
        int fd = fileno(file);
        if (fd > HANDLES_MAX) {
            fprintf(stderr,"filehandle out of range\n");
            exit(-1);
        }
        handle_table[fd]=1;
        strncpy(filename_table+PATH_MAX*fd,mapped,PATH_MAX);
        file_table[fd]=file;
        basename_idx[fd]=match_idx;
        return(file);
    }
    return(_fopen(pathname,mode));
}
int close(int fd) {
    if (! handle_table[fd])
        return(_close(fd));
    nfiles--;
    handle_table[fd]=0;
    file_table[fd]=NULL;
    filename_table[fd*PATH_MAX]=0;
    basename_idx[fd]=0;
    return(_close(fd));
}
int fclose(FILE * file) {
    int fd=fileno(file);
    if (! handle_table[fd])
        return(_fclose(file));
    handle_table[fd]=0;
    file_table[fd]=NULL;
    basename_idx[fd]=0;
    nfiles--;
    filename_table[fd*PATH_MAX]=0;
    return(_fclose(file));
}
