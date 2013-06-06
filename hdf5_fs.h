#ifndef _HDF5_FS_H
#define _HDF5_FS_H
int hdf5_fs_init(const char * hdf_filename);
int hdf5_fs_fini();
int hdf5_close(int fd);
int hdf5_write(int fd, const void *buf, size_t count);
int hdf5_open(int fd, const char *pathname, int flags);
int hdf5_lseek(int fd, off_t offset, int whence);
int hdf5_read(int fd, void *buf, size_t count);
int hdf5_fstat64(int fd, struct stat64 *buf);
int hdf5_stat64(const char *pathname, struct stat64 *buf);
int hdf5_unlink(const char *pathname);
#endif
