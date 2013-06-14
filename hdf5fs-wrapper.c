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
#include <errno.h>

#include "logger.h"
#include "path_util.h"
#include "hdf5_fs.h"
#include "wrapper_limits.h"
#include "env_util.h"

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

char scratch_base[PATH_MAX] = "./SCRATCH/*";
char hdf_file[PATH_MAX] = "./scratch${OMPI_COMM_WORLD_RANK:%04d:0}.h5";
char tmpdir[PATH_MAX];
char hdf_abs[PATH_MAX];
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

void __attribute__ ((constructor)) hdf5fs_wr_init() {
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
    char * env_ptr;
    env_ptr=getenv("HDF5FS_BASE");
    if (env_ptr != NULL) {
        strncpy(scratch_base,env_ptr,PATH_MAX);
    }
    env_ptr=getenv("HDF5FS_FILE");
    if (env_ptr != NULL) {
        strncpy(hdf_file,env_ptr,PATH_MAX);
    }
    rel2abs(scratch_base,scratch_abs);
    LOG_INFO("scratch_abs: '%s'",scratch_abs);
    int i;
    for (i=0;i<HANDLES_MAX;i++) handle_table[i]=0;
    for (i=0;i<HANDLES_MAX;i++) file_table[i]=NULL;
    for (i=0;i<HANDLES_MAX*PATH_MAX;i++) filename_table[i]=0;
    for (i=0;i<HANDLES_MAX;i++) basename_idx[i]=0;
    tmpdir[0]=0;
    unsetenv("LD_PRELOAD");
    char hdf_expanded[PATH_MAX];
    if (strn_env_expand(hdf_file,hdf_expanded,PATH_MAX) < 0) {
        LOG_FATAL("error expanding hdf filename '%s'",hdf_file);
        exit(1);
    }
    rel2abs(hdf_expanded,hdf_abs);
    if (! hdf5_fs_init(hdf_abs)) {
        LOG_FATAL("error initializing hdf5_fs");
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
        LOG_INFO("tmpdir set to '%s'",tmpdir);
    }
    strncpy(mapped,tmpdir,PATH_MAX);
    char *dstbase=mapped+strnlen(mapped,PATH_MAX);
    while (*baseptr != 0) {
        *dstbase=(*baseptr == '/' ? '%' : *baseptr);
        dstbase++;
        baseptr++;
    }
    LOG_DBG("%d == pathcmp('%s','%s'), base: '%s'",
            match_index,scratch_abs,mapped,
            mapped+strlen(tmpdir));
    return(strlen(tmpdir));
}

int mkdir(const char *pathname, mode_t mode) {
    char mapped[PATH_MAX];
    LOG_DBG("mkdir_called: '%s'",pathname);
    if (map_filename(pathname,mapped)>=0) {
        LOG_INFO("mapped: '%s' to '%s'",pathname,mapped);
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

    LOG_DBG("called: '%s'",pathname);
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
#if (LOG_LEVEL >= LOG_LEVEL_INFO)
        char *modestr=WTF_str;
        if ((flags & O_ACCMODE) == O_RDWR) {
            modestr=rdwr_str;
        } else if ((flags & O_ACCMODE) == O_RDONLY) {
            modestr=rdon_str;
        } else if ((flags & O_ACCMODE) == O_WRONLY) {
            modestr=wron_str;
        }
        LOG_INFO("mapped: '%s' to '%s', 0x%x(%s), nfiles: %d",pathname,mapped,flags,modestr,nfiles);
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
            LOG_FATAL("filehandle out of range: %d",fd);
            exit(-1);
        }
        int res = hdf5_open(fd,mapped+match_idx,flags);
        if (res < 0) {
            int hdf5errno = errno;
            close(fd);
            errno=hdf5errno;
            return(res);
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
    int match_idx;
    LOG_DBG("called: '%s'",pathname);
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
        LOG_INFO("mapped: '%s' to '%s'",pathname,mapped);
        return(hdf5_stat64(mapped+match_idx,buf));
//        return(___xstat64(version,mapped,buf));
    }
    return(___xstat64(version,pathname,buf));
}

int unlink(const char *pathname) {
    char mapped[PATH_MAX];
    LOG_DBG("called: '%s'",pathname);
    int match_index;
    if ((match_index=map_filename(pathname,mapped)) >= 0) {
        LOG_INFO("mapped: '%s' to '%s'",pathname,mapped);
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
    LOG_DBG("called: '%s'",pathname);
    if ((match_idx=map_filename(pathname,mapped)) >= 0) {
        LOG_INFO("mapped: '%s' to '%s', mode: %s, nfiles: %d",pathname,mapped,mode,nfiles);
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
            LOG_FATAL("filehandle out of range");
            exit(-1);
        }
        handle_table[fd]=1;
        strncpy(filename_table+PATH_MAX*fd,mapped,PATH_MAX);
        file_table[fd]=file;
        basename_idx[fd]=match_idx;
        int flags=O_RDONLY;
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
        int res;
        if ((res=hdf5_open(fd,mapped+match_idx,flags)) < 0) {
            int hdf5errno=errno;
            LOG_INFO("error hdf5_opening '%s'",mapped+match_idx);
            fclose(file);
            errno=hdf5errno;
            return(NULL);
        }
        return(file);
    }
    return(_fopen(pathname,mode));
}
int close(int fd) {
    if (! handle_table[fd])
        return(_close(fd));
    LOG_INFO("called: '%s', nfiles %d", filename_table+fd*PATH_MAX,nfiles);
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
    LOG_INFO("called: '%s', nfiles %d", filename_table+fd*PATH_MAX,nfiles);
    handle_table[fd]=0;
    file_table[fd]=NULL;
    basename_idx[fd]=0;
    nfiles--;
    filename_table[fd*PATH_MAX]=0;
    hdf5_close(fd);
    return(_fclose(file));
}
void __attribute__ ((destructor)) hdf5fs_wr_fini(void) {
    LOG_DBG("called");
    hdf5_fs_fini();
}
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    int fd=fileno(stream);
    if (! handle_table[fd])
        return(_fread(ptr,size,nmemb,stream));
    LOG_DBG("'%s' %d", filename_table+fd*PATH_MAX, (int)size*nmemb);
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
    LOG_DBG("'%s', %d", filename_table+fd*PATH_MAX,(int)size*nmemb);
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
    LOG_DBG("'%s', %d", filename_table+fd*PATH_MAX,count);
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
    LOG_DBG("'%s', %d", filename_table+fd*PATH_MAX, count);
    ssize_t fwritten = count;
#ifdef BOTH_HDF_AND_FILE
    fwritten=_write(fd,buf,count);
#endif
    return(hdf5_write(fd,buf,fwritten));
}
off_t lseek64(int fd, off_t offset, int whence) {
    if (! handle_table[fd])
        return(_lseek64(fd,offset,whence));
    LOG_DBG("'%s', %d, %d", filename_table+fd*PATH_MAX, (int)offset,(int)whence);
    int hoff = hdf5_lseek(fd,offset,whence);
#ifdef BOTH_HDF_AND_FILE
    int foff = _lseek64(fd,offset,whence);
    if (hoff != foff) {
        LOG_WARN("difference between hdf/file in '%s': %d/%d",
                filename_table+fd*PATH_MAX,hoff,foff);
    }
#endif
    return(hoff);
}
int fseek(FILE *stream, long offset, int whence) {
    int fd = fileno(stream);
    if (! handle_table[fd])
        return(_fseek(stream,offset,whence));
    LOG_DBG("'%s', %d, %d", filename_table+fd*PATH_MAX,(int)offset,(int)whence);
    int hoff=hdf5_lseek(fd,offset,whence);
#ifdef BOTH_HDF_AND_FILE
    int foff=_fseek(stream,offset,whence);
    if (hoff != foff) {
        LOG_WARN("difference between hdf/file in '%s': %d/%d",
                filename_table+fd*PATH_MAX,hoff,foff);
    }
#endif
    return(hoff);
}
long int ftell(FILE *stream) {
    int fd = fileno(stream);
    if (! handle_table[fd])
        return(_ftell(stream));
    LOG_DBG("'%s'", filename_table+fd*PATH_MAX);
    int hoff=hdf5_lseek(fd,0,SEEK_CUR);
#ifdef BOTH_HDF_AND_FILE
    int foff=_ftell(stream);
    if (hoff != foff) {
        LOG_WARN("difference between hdf/file in '%s': %d/%d",
                filename_table+fd*PATH_MAX,hoff,foff);
    }
#endif
    return(hoff);
}
int __fxstat64 (int __ver, int fd, struct stat64 *buf) {
    if (! handle_table[fd])
        return(___fxstat64(__ver,fd,buf));
    LOG_DBG("'%s'", filename_table+fd*PATH_MAX);
    return(hdf5_fstat64(fd,buf));
}
