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
//autowrap: retval=__real_open64(name, flags, mode);
//autowrap: k=kh_put(WFD,wrapper_fds,retval,&ret);
//autowrap: kh_value(wrapper_fds,k)=retval;
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
FILE*
    fopen(
        const PATHNAME
            name,
        const char*
            mode
        );
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
int
    fseek(
        FILE*
            stream,
        long
            offset,
        int
            whence
        );
long
    ftell(
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
int
    __fxstat(
        int
            vers,
        FD
            fd,
        struct stat *
            buf
        );
int
    __xstat(
        int 
            vers,
        const PATHNAME
            name,
        struct stat *
            buf
        );
int
    __lxstat(
        int
            vers,
        const PATHNAME
            name,
        struct stat *
            buf
        );
int
    __fxstat64(
        int
            vers,
        FD
            fd,
        struct stat64 *
            buf
        );
int
    __xstat64(
        int
            vers,
        const PATHNAME
            name,
        struct stat64 *
            buf
        );
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
//autowrap: kh_del(WFD,wrapper_fds,k);
//autowrap: retval=__real_close(fd);
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
int
    mkdir(
        const PATHNAME
            name,
        mode_t
            mode
        );
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
char*
    ttyname(
        FD
            fd
        );
int
    unlink(
        const PATHNAME
            name
        );
ssize_t
    write(
        FD
            fd,
        const void*
            buf,
        size_t
            count
        );
off_t
    lseek(
        FD
            fd,
        off_t
            offset,
        int
            whence
        );

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
