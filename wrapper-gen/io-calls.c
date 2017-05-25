//begin_preamble
/*
 * Copyright (c) 2013 Henning Glawe <glaweh@debian.org>
 *
 * This file is part of hdf5fs-wrapper.
 *
 * hdf5fs-wrapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hdf5fs-wrapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with hdf5fs-wrapper.  If not, see <http://www.gnu.org/licenses/>.
 */

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
//autowrap:      int ret;
//autowrap:      h5fd_t *h5fd = h5fd_open(scr_name, flags, mode);
//autowrap:      if (h5fd == NULL) goto errlabel;
//dbgautowrap:   retval=__real_open(name, flags, mode);
//nodbgautowrap: retval=__real_open("/dev/null", O_RDONLY, mode);
//autowrap:      h5fd->fd = retval;
//autowrap:      LOG_DBG("fd %d, filename %s, flags 0%o",retval,scr_name,flags);
//autowrap:      k=kh_put(WFD,wrapper_fds,retval,&ret);
//autowrap:      kh_value(wrapper_fds,k)=h5fd;
//autoerr:       retval=-1;
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
//autowrap:      int ret;
//autowrap:      h5fd_t *h5fd = h5fd_open(scr_name, flags, mode);
//autowrap:      if (h5fd == NULL) goto errlabel;
//dbgautowrap:   retval=__real_open64(name, flags, mode);
//nodbgautowrap: retval=__real_open64("/dev/null", O_RDONLY, mode);
//autowrap:      h5fd->fd = retval;
//autowrap:      LOG_DBG("fd %d, filename %s, flags 0%o",retval,scr_name,flags);
//autowrap:      k=kh_put(WFD,wrapper_fds,retval,&ret);
//autowrap:      kh_value(wrapper_fds,k)=h5fd;
//autoerr:       retval=-1;
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
//autowrap:      int real_retval;
//autowrap:      retval = h5fd_close(scr_stream);
//autowrap:      LOG_DBG("fclose(%p)=%d",scr_stream,retval);
//autowrap:      if (retval < 0) goto errlabel;
//autowrap:      kh_del(WFILE,wrapper_files,k);
//autowrap:      real_retval = __real_fclose(stream);
//dbgautowrap:   if (real_retval!=retval) LOG_ERR("different retval h5/real: %d,%d",retval,real_retval);
//autoerr:       retval=EOF;
int
    fclose(
        FILE*
            stream
        );
//dbgautowrap:   int real_retval;
//autowrap:      retval = h5fd_feof(scr_stream);
//dbgautowrap:   real_retval= __real_feof(stream);
//dbgautowrap:   if (real_retval!=retval) LOG_ERR("different retval h5/real: %d,%d",retval,real_retval);
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
//autowrap:      int ret;
//autowrap:      int flags = fopen_mode2open_flags(mode);
//autowrap:      h5fd_t *h5fd = h5fd_open(scr_name, flags, 0666);
//autowrap:      if (h5fd == NULL) goto errlabel;
//dbgautowrap:   retval=__real_fopen(name, mode);
//nodbgautowrap: retval=__real_fopen("/dev/null", "r");
//autowrap:      h5fd->stream = retval;
//autowrap:      LOG_DBG("stream %p, filename %s, h5fd %p",retval,scr_name,h5fd);
//autowrap:      k=kh_put(WFILE,wrapper_files,(PTR2INT)retval,&ret);
//autowrap:      kh_value(wrapper_files,k)=h5fd;
//autoerr:       retval=NULL;
FILE*
    fopen(
        const PATHNAME
            name,
        const char*
            mode
        );
//need_khiter
//autowrap:      int ret;
//autowrap:      int flags = fopen_mode2open_flags(mode);
//autowrap:      h5fd_t *h5fd = h5fd_open(scr_name, flags, 0666);
//autowrap:      if (h5fd == NULL) goto errlabel;
//dbgautowrap:   retval=__real_fopen64(name, mode);
//nodbgautowrap: retval=__real_fopen64("/dev/null", "r");
//autowrap:      h5fd->stream = retval;
//autowrap:      LOG_DBG("stream %p, filename %s, h5fd %p",retval,scr_name,h5fd);
//autowrap:      k=kh_put(WFILE,wrapper_files,(PTR2INT)retval,&ret);
//autowrap:      kh_value(wrapper_files,k)=h5fd;
//autoerr:       retval=NULL;
FILE* 
    fopen64(
        const PATHNAME
            name,
        const char*
            mode
        );
//no_syminit:    __real_vfprintf=dlsym(RTLD_NEXT, "vfprintf");
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
//autowrap:      size_t count = size * nmemb;
//dbgautowrap:   size_t real_retval;
//dbgautowrap:   void * ptr2 = malloc(count);
//dbgautowrao:   if (ptr2 == NULL) { LOG_ERR("malloc error"); goto errlabel; }
//dbgautowrap:   real_retval=__real_fread(ptr2,size,nmemb,stream);
//dbgautowrap:   nmemb = real_retval;
//dbgautowrap:   count = size * nmemb;
//autowrap:      retval=h5fd_read(scr_stream,ptr,count) / size;
//dbgautowrap:   if (real_retval!=retval) LOG_ERR("different retval h5/real: %ld/%ld",(long int)retval,(long int)real_retval);
//dbgautowrap:   if (memcmp(ptr2,ptr,(retval < real_retval ? retval : real_retval)*size) != 0) LOG_ERR("different data h5/real");
//dbgautowrap:   free(ptr2);
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
//autowrap:      size_t count = size * nmemb;
//dbgautowrap:   size_t real_retval;
//dbgautowrap:   void * ptr2 = malloc(count);
//dbgautowrap:   real_retval=__real_fread_unlocked(ptr2,size,nmemb,stream);
//dbgautowrap:   nmemb = real_retval;
//dbgautowrap:   count = size * nmemb;
//autowrap:      retval=h5fd_read(scr_stream,ptr,count) / size;
//dbgautowrap:   if (real_retval!=retval) LOG_ERR("different retval h5/real: %ld/%ld",(long int)retval,(long int)real_retval);
//dbgautowrap:   if (memcmp(ptr2,ptr,(retval < real_retval ? retval : real_retval)*size) != 0) LOG_ERR("different data h5/real");
//dbgautowrap:   free(ptr2);
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
//autowrap:      retval=h5fd_seek(scr_stream,offset,whence);
//autowrap:      if (retval>=0) retval=0;
//dbgautowrap:   off_t real_retval=__real_fseek(stream,offset,whence);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval,(long int)real_retval);
int
    fseek(
        FILE*
            stream,
        long
            offset,
        int
            whence
        );
//autowrap:      retval=h5fd_seek(scr_stream,offset,whence);
//autowrap:      if (retval>=0) retval=0;
//dbgautowrap:   off_t real_retval=__real_fseeko(stream,offset,whence);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval,(long int)real_retval);
int
    fseeko(
        FILE*
            stream,
        off_t
            offset,
        int
            whence
        );
//autowrap:      retval=h5fd_seek(scr_stream,offset,whence);
//autowrap:      if (retval>=0) retval=0;
//dbgautowrap:   off_t real_retval=__real_fseeko64(stream,offset,whence);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval,(long int)real_retval);
int
    fseeko64(
        FILE*
            stream,
        off64_t
            offset,
        int
            whence
        );
//autowrap:      retval=scr_stream->offset;
//dbgautowrap:   long real_retval=__real_ftell(stream);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval,(long int)real_retval);
long
    ftell(
        FILE*
            stream
        );
//autowrap:      retval=scr_stream->offset;
//dbgautowrap:   off_t real_retval=__real_ftello(stream);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval,(long int)real_retval);
off_t
    ftello(
        FILE*
            stream
        );
//autowrap:      retval=scr_stream->offset;
//dbgautowrap:   off64_t real_retval=__real_ftello64(stream);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("stream: %p, h5: %ld, real: %ld FUCK",stream,(long int)retval,(long int)real_retval);
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
//autowrap:      retval=h5fd_fstat(scr_fd,buf);
int
    __fxstat(
        int
            vers,
        FD
            fd,
        struct stat *
            buf
        );
//no_syminit:    // openmpi's malloc()-wrapper calls stat on infiniband/myrinet device nodes
//no_syminit:    // dlsym() calls malloc()
//no_syminit:    // break the deadlock by pretending the device exists
//no_syminit:    LOG_EARLY_DBG("called before wrapper initialization (\"%s\")",name);
//no_syminit:    int i; const char * dev_str = "/dev/"; const char * sys_str="/sys/";
//no_syminit:    for (i=0;i<5;i++) if (*(dev_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    for (i=0;i<5;i++) if (*(sys_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    __real___xstat=dlsym(RTLD_NEXT, "__xstat");
//autowrap:      retval=h5fs_stat(scr_name,buf);
int
    __xstat(
        int 
            vers,
        const PATHNAME
            name,
        struct stat *
            buf
        );
//no_syminit:    // openmpi's malloc()-wrapper calls stat on infiniband/myrinet device nodes
//no_syminit:    // dlsym() calls malloc()
//no_syminit:    // break the deadlock by pretending the device exists
//no_syminit:    LOG_EARLY_DBG("called before wrapper initialization (\"%s\")",name);
//no_syminit:    int i; const char * dev_str = "/dev/"; const char * sys_str="/sys/";
//no_syminit:    for (i=0;i<5;i++) if (*(dev_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    for (i=0;i<5;i++) if (*(sys_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    __real___lxstat=dlsym(RTLD_NEXT, "__lxstat");
//autowrap:      retval=h5fs_stat(scr_name,buf);
int
    __lxstat(
        int
            vers,
        const PATHNAME
            name,
        struct stat *
            buf
        );
//autowrap:      retval=h5fd_fstat64(scr_fd,buf);
int
    __fxstat64(
        int
            vers,
        FD
            fd,
        struct stat64 *
            buf
        );
//no_syminit:    // openmpi's malloc()-wrapper calls stat on infiniband/myrinet device nodes
//no_syminit:    // dlsym() calls malloc()
//no_syminit:    // break the deadlock by pretending the device exists
//no_syminit:    LOG_EARLY_DBG("called before wrapper initialization (\"%s\")",name);
//no_syminit:    int i; const char * dev_str = "/dev/"; const char * sys_str="/sys/";
//no_syminit:    for (i=0;i<5;i++) if (*(dev_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    for (i=0;i<5;i++) if (*(sys_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    __real___xstat64=dlsym(RTLD_NEXT, "__xstat64");
//autowrap:      retval=h5fs_stat64(scr_name,buf);
int
    __xstat64(
        int
            vers,
        const PATHNAME
            name,
        struct stat64 *
            buf
        );
//no_syminit:    // openmpi's malloc()-wrapper calls stat on infiniband/myrinet device nodes
//no_syminit:    // dlsym() calls malloc()
//no_syminit:    // break the deadlock by pretending the device exists
//no_syminit:    LOG_EARLY_DBG("called before wrapper initialization (\"%s\")",name);
//no_syminit:    int i; const char * dev_str = "/dev/"; const char * sys_str="/sys/";
//no_syminit:    for (i=0;i<5;i++) if (*(dev_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    for (i=0;i<5;i++) if (*(sys_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    __real___lxstat64=dlsym(RTLD_NEXT, "__lxstat64");
//autowrap:      retval=h5fs_stat64(scr_name,buf);
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
//no_syminit:    // openmpi's malloc()-wrapper calls access on infiniband/myrinet device nodes
//no_syminit:    // dlsym() calls malloc()
//no_syminit:    // break the deadlock by pretending the device exists
//no_syminit:    LOG_EARLY_DBG("called before wrapper initialization (\"%s\")",name);
//no_syminit:    int i; const char * dev_str = "/dev/"; const char * sys_str="/sys/";
//no_syminit:    for (i=0;i<5;i++) if (*(dev_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    for (i=0;i<5;i++) if (*(sys_str+i)!=*(name+i)) break;
//no_syminit:    if (i==5) return(0);
//no_syminit:    __real_access=dlsym(RTLD_NEXT, "access");
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
//autowrap:      retval = h5fd_close(scr_fd);
//autowrap:      LOG_DBG("close(%d)=%d",fd,retval);
//autowrap:      if (retval < 0) goto errlabel;
//autowrap:      kh_del(WFD,wrapper_fds,k);
//autowrap:      int real_retval = __real_close(fd);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("fd: %d, h5: %d, real: %d FUCK",fd,retval,real_retval);
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
//autowrap:      retval=h5fd_ftruncate(scr_fd,length);
//autowrap:      LOG_DBG("called ftruncate (%d, %ld)", fd, (long int)length);
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
//autowrap:      retval = 0;
//autowrap:      errno  = ENOTTY;
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
//dbgautowrap:   retval=__real_mkdir(name,mode);
//nodbgautowrap: retval=0; //disable mkdir
int
    mkdir(
        const PATHNAME
            name,
        mode_t
            mode
        );
//dbgautowrap:   retval = 0;
//dbgautowrap:   ssize_t real_offset=__real_lseek64(fd,0,SEEK_CUR);
//dbgautowrap:   if (real_offset != scr_fd->offset) {
//dbgautowrap:       LOG_ERR("different offsets h5/real %ld/%ld FUCK",scr_fd->offset,real_offset);
//dbgautowrap:   }
//dbgautowrap:   void * buf2 = malloc(count);
//dbgautowrao:   if (buf2 == NULL) { LOG_ERR("malloc error"); goto errlabel; }
//dbgautowrap:   ssize_t real_retval = __real_read(fd,buf2,count);
//dbgautowrap:   if (real_retval < count) { LOG_DBG("read only %ld bytes instead of %ld",real_retval,count); }
//dbgautowrap:   if (real_retval < 0) { old_errno=errno; free(buf2); errno=old_errno; goto errlabel; }
//dbgautowrap:   count=real_retval;
//autowrap:      retval = h5fd_read(scr_fd,buf,count);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("fd: %d, h5: %ld, real: %ld FUCK",fd,(long int)retval,(long int)real_retval);
//dbgautowrap:   int pos;
//dbgautowrap:   int len=(retval < real_retval ? retval : real_retval);
//dbgautowrap:   LOG_DBG("rv: %ld/%ld",(long int)retval,(long int)real_retval);
//dbgautowrap:   char * b1 = buf;
//dbgautowrap:   char * b2 = buf2;
//dbgautowrap:   for (pos=0;pos<len;pos++) { if (*(b1+pos)!=*(b2+pos)) break; }
//dbgautowrap:   if (pos != len) {
//dbgautowrap:       real_offset=__real_lseek64(fd,0,SEEK_CUR);
//dbgautowrap:       LOG_ERR("different data h5/real '%s'/%ld/%ld %d 0x%02x 0x%02x FUCK",scr_fd->hdirent->name,(long int)scr_fd->offset,real_offset,pos,*(b1+pos),*(b2+pos));
//dbgautowrap:       abort();
//dbgautowrap:   }
//dbgautowrap:   free(buf2);
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
//autowrap:      retval=h5fs_unlink(scr_name);
//dbgautowrap:   int real_retval=__real_unlink(name);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("path: %s, h5: %d, real: %d FUCK",name,retval,real_retval);
int
    unlink(
        const PATHNAME
            name
        );
//dbgautowrap:   retval = -1;
//dbgautowrap:   ssize_t real_retval=__real_write(fd,buf,count);
//dbgautowrap:   if (real_retval < 0) goto errlabel;
//dbgautowrap:   count = real_retval;
//autowrap:      retval=h5fd_write(scr_fd,buf,count);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("fd: %d, h5: %ld, real: %ld FUCK",fd,(long int)retval,(long int)real_retval);
ssize_t
    write(
        FD
            fd,
        const void*
            buf,
        size_t
            count
        );
//autowrap:      retval=h5fd_seek(scr_fd,offset,whence);
//dbgautowrap:   off_t real_retval=__real_lseek(fd,offset,whence);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("fd: %d, h5: %ld, real: %ld FUCK",fd,(long int)retval,(long int)real_retval);
off_t
    lseek(
        FD
            fd,
        off_t
            offset,
        int
            whence
        );
//autowrap:      retval=h5fd_seek(scr_fd,offset,whence);
//dbgautowrap:   off64_t real_retval=__real_lseek64(fd,offset,whence);
//dbgautowrap:   if (real_retval != retval) LOG_ERR("fd: %d, h5: %ld, real: %ld FUCK",fd,(long int)retval,(long int)real_retval);
//dbgautowrap:   real_retval=__real_lseek64(fd,0,SEEK_CUR);
//dbgautowrap:   if (scr_fd->offset != real_retval) LOG_ERR("offset_diff, fd: %d, h5/real: %ld / %ld FUCK",fd,(long int)scr_fd->offset,(long int)real_retval);
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
