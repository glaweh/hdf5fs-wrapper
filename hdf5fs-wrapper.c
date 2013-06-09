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
#include "hdf5_fs.h"
#include "wrapper_limits.h"

int (*_open64)(const char *, int, ...) = NULL;
int (*_mkdir)(const char *, mode_t) = NULL;
int (*_unlink)(const char *) = NULL;
int (*___xstat64)(int,const char *,struct stat64*) = NULL;
FILE *(*_fopen)(const char *,const char *) = NULL;
int (*_close)(int)=NULL;
int (*_fclose)(FILE *)=NULL;
size_t (*_fread)(void *ptr, size_t size, size_t nmemb, FILE *stream) = NULL;
size_t (*_fwrite)(const void *ptr, size_t size, size_t nmemb, FILE *stream) = NULL;
ssize_t (*_read)(int fd, void *buf, size_t count);
ssize_t (*_write)(int fd, const void *buf, size_t count);
off_t (*_lseek64)(int fd, off_t offset, int whence);
int (*_fseek)(FILE *stream, long offset, int whence);
long int (*_ftell)(FILE *stream);
int (*___fxstat64)(int __ver, int __fildes, struct stat64 *__stat_buf);

const char *scratch_base = "./SCRATCH/*";
const char *hdf_base = "./scratch.h5";
char tmpdir[PATH_MAX];
char hdf_filename[PATH_MAX];
char scratch_abs[PATH_MAX];

int   nfiles   = 0;
int   handle_table[HANDLES_MAX];
FILE* file_table[HANDLES_MAX];
char  filename_table[HANDLES_MAX*PATH_MAX];
int   basename_idx[HANDLES_MAX];

/* libc io calls found in serial espresso pw.x (scf), pp.x (localization) for everything under outdir
 *  libc call    implemented_here  found_in_espresso
    close        Y
    fopen        Y
    mkdir        Y
    open64       Y
    unlink       Y
    __xstat64    Y
    fread        Y
    fseek        Y
    ftell        Y
    fwrite       Y                 N
    __fxstat64   Y
    lseek64      Y
    read         Y
    write        Y
    isatty
    ttyname
*/

void __attribute__ ((constructor)) my_init() {
    _open64    = dlsym(RTLD_NEXT, "open64");
    _mkdir     = dlsym(RTLD_NEXT, "mkdir");
    _unlink    = dlsym(RTLD_NEXT, "unlink");
    ___xstat64 = dlsym(RTLD_NEXT, "__xstat64");
    _fopen     = dlsym(RTLD_NEXT, "fopen");
    _close     = dlsym(RTLD_NEXT, "close");
    _fclose    = dlsym(RTLD_NEXT, "fclose");
    _fread     = dlsym(RTLD_NEXT, "fread");
    _fwrite    = dlsym(RTLD_NEXT, "fwrite");
    _read      = dlsym(RTLD_NEXT, "read");
    _write     = dlsym(RTLD_NEXT, "write");
    _lseek64   = dlsym(RTLD_NEXT, "lseek64");
    _fseek     = dlsym(RTLD_NEXT, "fseek");
    _ftell     = dlsym(RTLD_NEXT, "ftell");
    ___fxstat64= dlsym(RTLD_NEXT, "__fxstat64");
    rel2abs(scratch_base,scratch_abs);
#ifdef DEBUG
    fprintf(stderr,"scratch_abs: '%s'\n",scratch_abs);
#endif
    int i;
    for (i=0;i<HANDLES_MAX;i++) handle_table[i]=0;
    for (i=0;i<HANDLES_MAX;i++) file_table[i]=NULL;
    for (i=0;i<HANDLES_MAX*PATH_MAX;i++) filename_table[i]=0;
    for (i=0;i<HANDLES_MAX;i++) basename_idx[i]=0;
    tmpdir[0]=0;
    rel2abs(hdf_base,hdf_filename);
    if (! hdf5_fs_init(hdf_filename)) {
        fprintf(stderr,"error initializing hdf5_fs\n");
        exit(1);
    }
}

int map_filename(const char *filename, char *mapped) {
    char mapped0[PATH_MAX];
    rel2abs(filename,mapped0);
    int match_index = pathcmp(scratch_abs,mapped0);
    if (match_index < 0) return(-1);
    // rewind back to future dirname
    char *baseptr=mapped0+match_index-2;
    while (*baseptr != '/')
        baseptr--;
    baseptr++;
    if (tmpdir[0]==0) {
        strncpy(tmpdir,mapped0,PATH_MAX);
        *(tmpdir+(baseptr-mapped0))=0;
#ifdef DEBUG
        fprintf(stderr,"tmpdir set to '%s'\n",tmpdir);
#endif
    }
    strncpy(mapped,tmpdir,PATH_MAX);
    char *dstbase=mapped+strnlen(mapped,PATH_MAX);
    while (*baseptr != 0) {
        *dstbase=(*baseptr == '/' ? '%' : *baseptr);
        dstbase++;
        baseptr++;
    }
#ifdef DEBUG_WRAPPER
    fprintf(stderr,"map_filename: %d == pathcmp('%s','%s'), base: '%s'\n",
            match_index,scratch_abs,mapped,
            mapped+strlen(tmpdir)
            );
#endif
    return(strlen(tmpdir));
}

int mkdir(const char *pathname, mode_t mode) {
    char mapped[PATH_MAX];
#ifdef DEBUG_WRAPPER
    fprintf(stderr,"mkdir_called: '%s'\n",pathname);
#endif
    if (map_filename(pathname,mapped)>=0) {
#ifdef DEBUG
        fprintf(stderr,"mkdir_mapped: '%s' to '%s'\n",pathname,mapped);
#endif
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
    if (flags & O_CREAT) {
        va_start(argp,flags);
        mode = va_arg(argp, mode_t);
        va_end(argp);
    }

#ifdef DEBUG_WRAPPER
    fprintf(stderr,"open64_called: '%s'\n",pathname);
#endif
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
#ifdef DEBUG
        char *modestr=WTF_str;
        if ((flags & O_ACCMODE) == O_RDWR) {
            modestr=rdwr_str;
        } else if ((flags & O_ACCMODE) == O_RDONLY) {
            modestr=rdon_str;
        } else if ((flags & O_ACCMODE) == O_WRONLY) {
            modestr=wron_str;
        }
        fprintf(stderr,"open64_mapped: '%s' to '%s', 0x%x(%s), nfiles: %d\n",pathname,mapped,flags,modestr,nfiles);
#endif
        int fd;
#ifdef BOTH_HDF_AND_FILE
        fd=_open64(mapped,flags,mode);
#else
        // open dummy handle
        fd=_open64("/dev/null",O_RDONLY,mode);
#endif
        if (fd < 0) return(fd);
        if (fd > HANDLES_MAX) {
            fprintf(stderr,"filehandle out of range\n");
            exit(-1);
        }
        nfiles++;
        handle_table[fd]=1;
        strncpy(filename_table+PATH_MAX*fd,mapped,PATH_MAX);
        basename_idx[fd]=match_idx;
        hdf5_open(fd,mapped+match_idx,flags);
        return(fd);
    }
    return(_open64(pathname,flags,mode));
}

int __xstat64(int version, const char *pathname, struct stat64 *buf) {
    char mapped[PATH_MAX];
    int match_idx;
#ifdef DEBUG_WRAPPER
    fprintf(stderr,"__xstat64_called: '%s'\n",pathname);
#endif
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
#ifdef DEBUG
        fprintf(stderr,"__xstat64_mapped: '%s' to '%s'\n",pathname,mapped);
#endif
        return(hdf5_stat64(mapped+match_idx,buf));
//        return(___xstat64(version,mapped,buf));
    }
    return(___xstat64(version,pathname,buf));
}

int unlink(const char *pathname) {
    char mapped[PATH_MAX];
#ifdef DEBUG_WRAPPER
    fprintf(stderr,"unlink_called: '%s'\n",pathname);
#endif
    int match_index;
    if ((match_index=map_filename(pathname,mapped)) >= 0) {
#ifdef DEBUG
        fprintf(stderr,"unlink_mapped: '%s' to '%s'\n",pathname,mapped);
#endif
        int hres = hdf5_unlink(mapped+match_index);
#ifdef BOTH_HDF_AND_FILE
        int fres = _unlink(mapped);
#endif
        return(hres);
    }
    return(_unlink(pathname));
}
FILE *fopen(const char *pathname,const char *mode) {
    char mapped[PATH_MAX];
    int match_idx;
#ifdef DEBUG_WRAPPER
    fprintf(stderr,"fopen_called: %s\n",pathname);
#endif
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
#ifdef DEBUG
        fprintf(stderr,"fopen_mapped: '%s' to '%s', mode: %s, nfiles: %d\n",pathname,mapped,mode,nfiles);
#endif
        FILE *file;
#ifdef BOTH_HDF_AND_FILE
        file =_fopen(mapped,mode);
#else
        file =_fopen("/dev/null","r");
#endif
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
        int flags;
        if (mode[1] == '+') {
            switch (mode[0]) {
                case 'r' :
                    flags = O_RDWR;
                    break;
                case 'w' :
                    flags = O_RDWR | O_CREAT | O_TRUNC;
                    break;
                case 'a' :
                    flags = O_APPEND | O_CREAT | O_RDWR;
                    break;
            }
        } else {
            switch (mode[0]) {
                case 'r' :
                    flags = O_RDONLY;
                    break;
                case 'w' :
                    flags = O_WRONLY | O_CREAT | O_TRUNC;
                    break;
                case 'a' :
                    flags = O_WRONLY | O_CREAT | O_APPEND;
                    break;
            }
        }
        if (hdf5_open(fd,mapped+match_idx,flags) < 0) {
            fprintf(stderr,"error hdf5_opening '%s'\n",mapped+match_idx);
        }
        return(file);
    }
    return(_fopen(pathname,mode));
}
int close(int fd) {
    if (! handle_table[fd])
        return(_close(fd));
#ifdef DEBUG
    fprintf(stderr,"close: '%s', nfiles %d\n", filename_table+fd*PATH_MAX,nfiles);
#endif
    nfiles--;
    handle_table[fd]=0;
    file_table[fd]=NULL;
    filename_table[fd*PATH_MAX]=0;
    basename_idx[fd]=0;
    hdf5_close(fd);
    return(_close(fd));
}
int fclose(FILE * file) {
    int fd=fileno(file);
    if (! handle_table[fd])
        return(_fclose(file));
#ifdef DEBUG
    fprintf(stderr,"fclose: '%s', nfiles %d\n", filename_table+fd*PATH_MAX,nfiles);
#endif
    handle_table[fd]=0;
    file_table[fd]=NULL;
    basename_idx[fd]=0;
    nfiles--;
    filename_table[fd*PATH_MAX]=0;
    hdf5_close(fd);
    return(_fclose(file));
}
void __attribute__ ((destructor)) my_fini(void) {
#ifdef DEBUG_WRAPPER
    fprintf(stderr,"my_fini called\n");
#endif
    hdf5_fs_fini();
}
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    int fd=fileno(stream);
    if (! handle_table[fd])
        return(_fread(ptr,size,nmemb,stream));
#ifdef DEBUG
    fprintf(stderr,"fread: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    int count = hdf5_read(fd,ptr,size*nmemb);
#ifdef BOTH_HDF_AND_FILE
    _fseek(stream,count,SEEK_CUR);
#endif
    return(count);
//    return(_fread(ptr,size,nmemb,stream));
}
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    int fd=fileno(stream);
    if (! handle_table[fd])
        return(_fwrite(ptr,size,nmemb,stream));
#ifdef DEBUG
    fprintf(stderr,"fwrite: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    size_t fwritten = nmemb;
#ifdef BOTH_HDF_AND_FILE
    fwritten=_fwrite(ptr,size,nmemb,stream);
#endif
    size_t hwritten = hdf5_write(fd,ptr,size*fwritten);
    return(hwritten);
}
ssize_t read(int fd, void *buf, size_t count) {
    if (! handle_table[fd])
        return(_read(fd,buf,count));
#ifdef DEBUG
    fprintf(stderr,"read: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    int rcount=hdf5_read(fd,buf,count);
#ifdef BOTH_HDF_AND_FILE
    _lseek64(fd,rcount,SEEK_CUR);
#endif
    return(rcount);
//    return(_read(fd,buf,count));
}
ssize_t write(int fd, const void *buf, size_t count) {
    if (! handle_table[fd])
        return(_write(fd,buf,count));
#ifdef DEBUG
    fprintf(stderr,"write: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    ssize_t fwritten = count;
#ifdef BOTH_HDF_AND_FILE
    fwritten=_write(fd,buf,count);
#endif
    return(hdf5_write(fd,buf,fwritten));
}
off_t lseek64(int fd, off_t offset, int whence) {
    if (! handle_table[fd])
        return(_lseek64(fd,offset,whence));
#ifdef DEBUG
    fprintf(stderr,"lseek64: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    int hoff = hdf5_lseek(fd,offset,whence);
#ifdef BOTH_HDF_AND_FILE
    int foff = _lseek64(fd,offset,whence);
    if (hoff != foff) {
        fprintf(stderr,"lseek64: '%s', difference between hdf/file %d/%d\n",
                filename_table+fd*PATH_MAX,hoff,foff);
    }
#endif
    return(hoff);
}
int fseek(FILE *stream, long offset, int whence) {
    int fd = fileno(stream);
    if (! handle_table[fd])
        return(_fseek(stream,offset,whence));
#ifdef DEBUG
    fprintf(stderr,"fseek: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    int hoff=hdf5_lseek(fd,offset,whence);
#ifdef BOTH_HDF_AND_FILE
    int foff=_fseek(stream,offset,whence);
    if (hoff != foff) {
        fprintf(stderr,"fseek: '%s', difference between hdf/file %d/%d\n",
                filename_table+fd*PATH_MAX,hoff,foff);
    }
#endif
    return(hoff);
}
long int ftell(FILE *stream) {
    int fd = fileno(stream);
    if (! handle_table[fd])
        return(_ftell(stream));
#ifdef DEBUG
    fprintf(stderr,"ftell: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    int hoff=hdf5_lseek(fd,0,SEEK_CUR);
#ifdef BOTH_HDF_AND_FILE
    int foff=_ftell(stream);
    if (hoff != foff) {
        fprintf(stderr,"ftell: '%s', difference between hdf/file %d/%d\n",
                filename_table+fd*PATH_MAX,hoff,foff);
    }
#endif
    return(hoff);
}
int __fxstat64 (int __ver, int fd, struct stat64 *buf) {
    if (! handle_table[fd])
        return(___fxstat64(__ver,fd,buf));
#ifdef DEBUG
    fprintf(stderr,"__fxstat64: '%s'\n", filename_table+fd*PATH_MAX);
#endif
    return(hdf5_fstat64(fd,buf));
}
