#ifndef H5FS_H
#define H5FS_H
#include <fcntl.h>
typedef struct h5fd h5fd_t;
typedef struct h5dd h5dd_t;
h5fd_t * h5fd_open(const char * name, int flags, mode_t mode);
int      h5fd_close(h5fd_t * h5fd);
#endif
