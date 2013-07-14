//begin_preamble
#define _GNU_SOURCE
#define __USE_LARGEFILE64
#include <dirent.h>
#include <libio.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <fcntl.h>

#define PATHNAME char*
#define FD int
//end_preamble

// dirent.h
int
    closedir(
        DIR*
            dh
        );
DIR*
    opendir(
        const PATHNAME
            name
        );
DIR*
    fdopendir(
        FD
            fd
        );
struct dirent*
    readdir(
        DIR*
            dh
        );
struct dirent64*
    readdir64(
        DIR*
            dh
        );
int
    scandir(
        const PATHNAME
            dirp,
        struct dirent***
            namelist,
        int
            (*filter)(const struct dirent *),
        int
            (*compar)(const struct dirent **, const struct dirent **)
        ); 
// fcntl.h
//vac: (flags & O_CREAT ? 1 : 0)
//vat: mode_t
//van: mode
//need_khiter
//autowrap: int ret;
//autowrap: h5fd_t *h5fd = h5fd_open(scr_name, flags, mode);
//autowrap: if (h5fd == NULL) goto errlabel;
//dbgautowrap: retval=__real_open(name, flags, mode);
//autowrap: retval=__real_open("/dev/null", O_RDONLY, mode);
//autowrap: h5fd->fd = retval;
//autowrap: LOG_INFO("fd %d, filename %s",retval,scr_name);
//autowrap: k=kh_put(WFD,wrapper_fds,retval,&ret);
//autowrap: kh_value(wrapper_fds,k)=h5fd;
//autoerr: retval=-1;
FD
    open(
        const PATHNAME
            name,
        int
            flags,
        ...
        );
//vac: (flags & O_CREAT ? 1 : 0)
//vat: mode_t
//van: mode
//need_khiter
//autowrap: int ret;
//autowrap: h5fd_t *h5fd = h5fd_open(scr_name, flags, mode);
//autowrap: if (h5fd == NULL) goto errlabel;
//dbgautowrap: retval=__real_open64(name, flags, mode);
//autowrap: retval=__real_open64("/dev/null", O_RDONLY, mode);
//autowrap: h5fd->fd = retval;
//autowrap: LOG_INFO("fd %d, filename %s",retval,scr_name);
//autowrap: k=kh_put(WFD,wrapper_fds,retval,&ret);
//autowrap: kh_value(wrapper_fds,k)=h5fd;
//autoerr: retval=-1;
FD
    open64(
        const PATHNAME
            name,
        int
            flags,
        ...
        );

// libio.h
int
    _IO_getc(
        _IO_FILE*
            stream
        );
int 
    _IO_putc(
        int
            data,
        _IO_FILE*
            stream
        );

// stdio.h
//need_khiter
//autowrap: retval = h5fd_close(scr_stream);
//autowrap: LOG_INFO("fclose(%p)=%d",scr_stream,retval);
//autowrap: if (retval < 0) goto errlabel;
//autowrap: kh_del(WFILE,wrapper_files,k);
//dbgautowrap: retval = __real_close(fd);
//autowrap: __real_fclose(stream);
//autoerr: retval=EOF;
int
    fclose(
        FILE*
            stream
        );
int
    feof(
        FILE*
            stream
        );
int
    ferror(
        FILE*
            stream
        );
int
    fflush(
        FILE*
            stream
        );
int
    fgetc(
        FILE*
            stream
        );
char*
    fgets(
        char*
            s,
        int
            size,
        FILE*
            stream
        );
FD
    fileno(
        FILE*
            stream
        );
//need_khiter
//autowrap: int ret;
//autowrap: int flags = fopen_mode2open_flags(mode);
//autowrap: h5fd_t *h5fd = h5fd_open(scr_name, flags, 0666);
//autowrap: if (h5fd == NULL) goto errlabel;
//dbgautowrap: retval=__real_fopen(name, mode);
//autowrap: retval=__real_fopen("/dev/null", "r");
//autowrap: h5fd->stream = retval;
//autowrap: LOG_INFO("stream %p, filename %s, h5fd %p",retval,scr_name,h5fd);
//autowrap: k=kh_put(WFILE,wrapper_files,(PTR2INT)retval,&ret);
//autowrap: kh_value(wrapper_files,k)=h5fd;
//autoerr: retval=NULL;
FILE*
    fopen(
        const PATHNAME
            name,
        const char*
            mode
        );
//need_khiter
//autowrap: int ret;
//autowrap: int flags = fopen_mode2open_flags(mode);
//autowrap: h5fd_t *h5fd = h5fd_open(scr_name, flags, 0666);
//autowrap: if (h5fd == NULL) goto errlabel;
//dbgautowrap: retval=__real_fopen64(name, mode, 0666);
//autowrap: retval=__real_fopen64("/dev/null", "r");
//autowrap: h5fd->stream = retval;
//autowrap: LOG_INFO("stream %p, filename %s, h5fd %p",retval,scr_name,h5fd);
//autowrap: k=kh_put(WFILE,wrapper_files,(PTR2INT)retval,&ret);
//autowrap: kh_value(wrapper_files,k)=h5fd;
//autoerr: retval=NULL;
FILE* 
    fopen64(
        const PATHNAME
            name,
        const char*
            mode
        );
//vaforward
int
    fprintf(
        FILE*
            stream,
        const char*
            format,
        ...
        );
int
    fputc(
        int
            c,
        FILE*
            stream
        );
int
    fputs(
        const char*
            s,
        FILE*
            stream
        );
size_t
    fread(
        void*
            ptr,
        size_t
            size,
        size_t
            nmemb,
        FILE*
            stream
        );
size_t
    fread_unlocked(
        void*
            ptr,
        size_t
            size,
        size_t
            nmemb,
        FILE*
            stream
        );
size_t
    fwrite(
        const void*
            ptr,
        size_t
            size,
        size_t
            nmemb,
        FILE*
            stream
        );
size_t
    fwrite_unlocked(
        const void*
            ptr,
        size_t
            size,
        size_t
            nmemb,
        FILE*
            stream
        );
//autowrap: retval=h5fd_seek(scr_stream,offset,whence);
//dbgautowrap: off_t retval_h5=h5fd_seek(scr_stream,offset,whence);
//dbgautowrap: retval=__real_fseek(stream,offset,whence);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval_h5,(long int)retval);
int
    fseek(
        FILE*
            stream,
        long
            offset,
        int
            whence
        );
//autowrap: retval=h5fd_seek(scr_stream,offset,whence);
//dbgautowrap: off_t retval_h5=h5fd_seek(scr_stream,offset,whence);
//dbgautowrap: retval=__real_fseeko(stream,offset,whence);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval_h5,(long int)retval);
int
    fseeko(
        FILE*
            stream,
        off_t
            offset,
        int
            whence
        );
//autowrap: retval=h5fd_seek(scr_stream,offset,whence);
//dbgautowrap: off_t retval_h5=h5fd_seek(scr_stream,offset,whence);
//dbgautowrap: retval=__real_fseeko64(stream,offset,whence);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval_h5,(long int)retval);
int
    fseeko64(
        FILE*
            stream,
        off64_t
            offset,
        int
            whence
        );
//autowrap: retval=scr_stream->offset;
//dbgautowrap: off_t retval_h5=scr_stream->offset;
//dbgautowrap: retval=__real_ftell(stream,offset,whence);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval_h5,(long int)retval);
long
    ftell(
        FILE*
            stream
        );
//autowrap: retval=scr_stream->offset;
//dbgautowrap: off_t retval_h5=scr_stream->offset;
//dbgautowrap: retval=__real_ftello(stream);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval_h5,(long int)retval);
off_t
    ftello(
        FILE*
            stream
        );
//autowrap: retval=scr_stream->offset;
//dbgautowrap: off_t retval_h5=scr_stream->offset;
//dbgautowrap: retval=__real_ftello64(stream);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval_h5,(long int)retval);
off64_t
    ftello64(
        FILE*
            stream
        );
int
    remove(
        const PATHNAME
            pathname
        );
char*
    tempnam(
        const PATHNAME
            dir,
        const char*
            pfx
        );
int
    vfprintf(
        FILE*
            stream,
        const char*
            format,
        va_list
            ap
        );
int
    setvbuf(
        FILE*
            stream,
        char*
            buf,
        int
            mode,
        size_t
            size
        );
void
    setbuf(
        FILE*
            stream,
        char*
            buf
        );
void
    setbuffer(
        FILE*
            stream,
        char*
            buf,
        size_t
            size
        );
void
    setlinebuf(
        FILE*
            stream
        );
int
    rename(
        const PATHNAME
            oldpath,
        const PATHNAME
            newpath
        );

// sys/ioctl.h
//vac: 1
//vat: char*
//van: arg
int
    ioctl(
        FD
            fd,
        unsigned long int
            request,
        ...
        );

// sys/stat.h
//autowrap: retval=h5fd_fstat(scr_fd,buf);
int
    __fxstat(
        int
            vers,
        FD
            fd,
        struct stat *
            buf
        );
//autowrap: retval=h5fs_stat(scr_name,buf);
int
    __xstat(
        int 
            vers,
        const PATHNAME
            name,
        struct stat *
            buf
        );
//autowrap: retval=h5fs_stat(scr_name,buf);
int
    __lxstat(
        int
            vers,
        const PATHNAME
            name,
        struct stat *
            buf
        );
//autowrap: retval=h5fd_fstat64(scr_fd,buf);
int
    __fxstat64(
        int
            vers,
        FD
            fd,
        struct stat64 *
            buf
        );
//autowrap: retval=h5fs_stat64(scr_name,buf);
int
    __xstat64(
        int
            vers,
        const PATHNAME
            name,
        struct stat64 *
            buf
        );
//autowrap: retval=h5fs_stat64(scr_name,buf);
int
    __lxstat64(
        int
            vers,
        const PATHNAME
            name,
        struct stat64 *
            buf
        );
int
    chmod(
        const PATHNAME
            name,
        mode_t
            mode
        );
int
    fchmod(
        FD
            fd,
        mode_t
            mode
        );
int
    mkfifo(
        const PATHNAME
            name,
        mode_t
            mode
        );

// unistd.h
int
    access(
        const PATHNAME
            name,
        int
            mode
        );
int
    chdir(
        const PATHNAME
            name
        );
int
    chown(
        const PATHNAME
            name,
        uid_t
            owner,
        uid_t
            group
        );
//need_khiter
//autowrap: retval = h5fd_close(scr_fd);
//autowrap: LOG_INFO("close(%d)=%d",fd,retval);
//autowrap: if (retval < 0) goto errlabel;
//autowrap: kh_del(WFD,wrapper_fds,k);
//dbgautowrap: retval = __real_close(fd);
//autowrap: __real_close(fd);
int
    close(
        FD
            fd
        );
int
    dup2(
        FD
            oldfd,
        FD
            newfd
        );
int
    fchdir(
        FD
            fd
        );
int
    ftruncate(
        FD
            fd,
        off_t
            length
        );
char*
    getcwd(
        char*
            buf,
        size_t
            size
        );
//autowrap: retval = 0;
//autowrap: errno  = ENOTTY;
int
    isatty(
        FD
            fd
        );
int
    link(
        const PATHNAME
            oldpath,
        const PATHNAME
            newpath
        );
//dbgautowrap: retval=__real_mkdir(name,mode);
//autowrap: //disable mkdir
int
    mkdir(
        const PATHNAME
            name,
        mode_t
            mode
        );
//autowrap: retval=h5fd_read(scr_fd,buf,count);
//dbgautowrap: retval=__real_read(fd,buf,count);
ssize_t
    read(
        FD
            fd,
        void*
            buf,
        size_t
            count
        );
int
    rmdir(
        const PATHNAME
            name
        );
int
    symlink(
        const PATHNAME
            oldpath,
        const PATHNAME
            newpath
        );
void
    sync(
        void
        );
//void
//    syncfs(
//        FD
//            fd
//        );
int
    truncate(
        const PATHNAME
            name,
        off_t
            length
        );
int
    truncate64(
        const PATHNAME
            name,
        off64_t
            length
        );
char*
    ttyname(
        FD
            fd
        );
//autowrap: retval=h5fs_unlink(scr_name);
//dbgautowrap: retval=__real_unlink(name);
int
    unlink(
        const PATHNAME
            name
        );
//autowrap: retval=h5fd_write(scr_fd,buf,count);
//dbgautowrap: retval=__real_write(fd,buf,count);
ssize_t
    write(
        FD
            fd,
        const void*
            buf,
        size_t
            count
        );
//autowrap: retval=h5fd_seek(scr_fd,offset,whence);
//dbgautowrap: off_t retval_h5=h5fd_seek(scr_fd,offset,whence);
//dbgautowrap: retval=__real_lseek(fd,offset,whence);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("fd: %d, h5: %ld, real: %ld FUCK",fd,(long int)retval_h5,(long int)retval);
off_t
    lseek(
        FD
            fd,
        off_t
            offset,
        int
            whence
        );
//autowrap: retval=h5fd_seek(scr_fd,offset,whence);
//dbgautowrap: off_t retval_h5=h5fd_seek(scr_fd,offset,whence);
//dbgautowrap: retval=__real_lseek64(fd,offset,whence);
//dbgautowrap: if (retval_h5 != retval) LOG_ERR("fd: %d, h5: %ld, real: %ld FUCK",fd,(long int)retval_h5,(long int)retval);
off64_t
    lseek64(
        FD
            fd,
        off64_t
            offset,
        int
            whence
        );

// wchar.h
int
    fwide(
        FILE*
            stream,
        int
            mode
        );
