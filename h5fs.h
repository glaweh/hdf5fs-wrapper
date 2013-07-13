#ifndef H5FS_H
#define H5FS_H
#include <fcntl.h>
typedef struct h5fd h5fd_t;
typedef struct h5dd h5dd_t;
h5fd_t * h5fd_open(const char * name, int flags, mode_t mode);
int      h5fd_close(h5fd_t * h5fd);
int      h5fs_unlink(const char * name);
int      h5fs_stat(const char * name, struct stat * sstat);
int      h5fs_stat64(const char * name, struct stat64 * sstat);
int      h5fd_fstat(h5fd_t * h5fd, struct stat * sstat);
int      h5fd_fstat64(h5fd_t * h5fd, struct stat64 * sstat);
off64_t  h5fd_seek(h5fd_t * h5fd, off64_t offset, int whence);
ssize_t  h5fd_write(h5fd_t * h5fd, const void * buf, size_t count);
#endif