#include <sys/types.h>
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <hdf5.h>
#include "hdf5_fs.h"
#include "string_set.h"
#include "wrapper_limits.h"
#include "logger.h"
#include "file_ds.h"

#define RANK 1
hid_t   hdf_file;
struct stat64 hdf_file_stat;
string_set * closed_empty_files;

typedef struct {
    file_ds_t * dataset;
    int     append;
    hsize_t offset[RANK];
    char  name[PATH_MAX];
} hdf5_data_t;

int     last_handle=-1;
hdf5_data_t * hdf5_data[HANDLES_MAX];

int hdf5_fs_init(const char * hdf_filename) {
    struct stat hdf_stat;
    herr_t status;
    if ((stat(hdf_filename,&hdf_stat)>=0) && (H5Fis_hdf5(hdf_filename) >= 0)) {
        hdf_file = H5Fopen(hdf_filename,H5F_ACC_RDWR,H5P_DEFAULT);
    } else {
        hdf_file = H5Fcreate(hdf_filename,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    }
    LOG_INFO("hdf_file opened: '%s', %d",hdf_filename,hdf_file);
    if (hdf_file < 0) {
        LOG_WARN("hdf_file error");
        return(-1);
    }
    int i;
    for (i=0;i<HANDLES_MAX;i++)
        hdf5_data[i]=NULL;
    last_handle = -1;
    if (stat64(hdf_filename,&hdf_file_stat) < 0) {
        LOG_WARN("error calling stat64 on '%s', %s",hdf_filename,strerror(errno));
    }
    closed_empty_files=string_set_new();
    return(1);
}

int hdf5_fs_fini() {
    int i;
    for (i=0;i<=last_handle;i++) {
        if (hdf5_data[i] != NULL)
            hdf5_close(i);
    }
    string_set_dump(closed_empty_files);
    string_set_free(closed_empty_files);
    H5Fclose(hdf_file);
    return(1);
}

int hdf5_open(int fd, const char *pathname, int flags) {
    if (hdf5_data[fd] != NULL) {
        LOG_WARN("fd already in use");
        return(-1);
    }
    LOG_DBG(" %d, '%s', %o", fd,pathname,flags);
    int set_exists = file_ds_exists(hdf_file,pathname);

    hid_t  set=-1;
    if (set_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            LOG_DBG("dataset already exists %d -> '%s'",fd,pathname);
            errno=EEXIST;
            return(-1);
        }
        LOG_DBG("set '%s' exists",pathname);
    } else if ((flags & O_CREAT) == 0) {
        errno=ENOENT;
        return(-1);
    }
    hdf5_data[fd] = malloc(sizeof(hdf5_data_t));
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("error allocating hdf5_data[%d]",fd);
        errno = ENOMEM;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    d->dataset = NULL;
    if (set_exists) {
        d->dataset = file_ds_open(hdf_file,pathname);
        if (d->dataset == NULL) {
            LOG_WARN("error opening file_ds '%s'",pathname);
            errno = EIO;
            free(d);
            hdf5_data[fd] = NULL;
            return(-1);
        } else {
            LOG_DBG("deferring creation of dataset '%s' to first write",pathname);
        }
    }

    strncpy(d->name,pathname,PATH_MAX);
    d->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    d->offset[0]= 0;
    if (d->dataset!=NULL) {
        if (flags & O_TRUNC) d->dataset->length  = 0;
        d->dataset->rdonly = ((flags & O_ACCMODE) == O_RDONLY);
    }
    if (fd > last_handle) last_handle = fd;
    return(fd);
}

int hdf5_close(int fd) {
    int status;
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    if (d->dataset != NULL) {
        if (! file_ds_close(d->dataset)) {
            LOG_WARN("error closing dataset '%s'",d->name);
            errno = EIO;
            return(-1);
        }
    } else {
        string_set_add(closed_empty_files, d->name, NULL);
    }
    free(d->dataset);
    LOG_DBG(" %d, '%s", fd,d->name);
    free(hdf5_data[fd]);
    hdf5_data[fd]=NULL;
    while ((last_handle >= 0) && (hdf5_data[last_handle] == NULL)) {
        last_handle--;
    }
    return(1);
}

int hdf5_write(int fd, const void *buf, size_t count) {
    if (count == 0)
        return(0);
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("write on unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    if (d->dataset == NULL) {
        d->dataset = file_ds_create(hdf_file, d->name, 0, count+1, 0, 0);
        if (d->dataset == NULL) {
            LOG_ERR("error creating dataset '%s'",d->name);
            errno = EIO;
            return(-1);
        }
    } else {
        if (d->append) d->offset[0]=d->dataset->length;
        hsize_t needed_dim = DIM_CHUNKED(d->offset[0]+count+1,d->dataset->chunk[0]);
        if (needed_dim > d->dataset->dims[0]) {
            d->dataset->dims[0]=needed_dim;
            if (H5Dset_extent(d->dataset->set, d->dataset->dims) < 0) {
                LOG_ERR("error resizing dataset '%s'",d->name);
                errno = EIO;
                return(-1);
            }
        }
    }
    LOG_DBG("(%d='%s', %d) %d (%d)\n",fd,d->name,(int)count,(int)d->offset[0],(int)d->dataset->length);
    hsize_t hs_count[RANK];
    hs_count[0]=count;
    hid_t filespace = H5Dget_space(d->dataset->set);
    hid_t dataspace = H5Screate_simple(RANK, hs_count, NULL);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, d->offset, NULL, hs_count, NULL);

    H5Dwrite(d->dataset->set,H5T_FILE_DS,dataspace,filespace,H5P_DEFAULT,buf);
    H5Sclose(filespace);
    H5Sclose(dataspace);
    H5Awrite(d->dataset->length_attrib, H5T_NATIVE_INT64, &d->dataset->length);
    if ((d->offset[0]+count) > d->dataset->length)
        d->dataset->length = d->offset[0]+count;
    d->offset[0]+=count;
    return(count);
}

int hdf5_read(int fd, void *buf, size_t count) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("write on unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    // end of file
    if (d->dataset == NULL) {
        LOG_DBG("unitialized dataset '%s'",d->name);
        return(0);
    }
    hsize_t bytes_read = file_ds_read(d->dataset,d->offset[0],buf,count);
    if (bytes_read >= 0) {
        d->offset[0]+=bytes_read;
        return((int)bytes_read);
    } else if (bytes_read == -1) {
        errno=EBADF;
        return(-1);
    } else if (bytes_read == -2) {
        errno=EIO;
        return(-1);
    }
    errno = EINVAL;
    return(-1);
}

int hdf5_lseek(int fd, off_t offset, int whence) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    switch (whence) {
        case SEEK_SET:
            d->offset[0]=offset;
            break;
        case SEEK_END:
            d->offset[0]=(d->dataset == NULL ? offset : d->dataset->length+offset);
            break;
        case SEEK_CUR:
            d->offset[0]+=offset;
            break;
        default:
            errno=EINVAL;
            LOG_WARN("illegal whence: %d",whence);
            return(-1);
            break;
    }
    LOG_DBG("(%d='%s',%d,%d) = %d",fd,d->name,(int)offset,whence,(int)d->offset[0]);
    return(d->offset[0]);
}

int hdf5_stat64(const char *pathname, struct stat64 *buf) {
    H5O_info_t object_info;
    if (file_ds_exists(hdf_file,pathname) == 0) {
        if (string_set_find(closed_empty_files, pathname) <= 0) {
            errno=ENOENT;
            return(-1);
        }
        (*buf)=hdf_file_stat;
        buf->st_blksize=4096;
        buf->st_size=0;
        buf->st_blocks=0;
        return(0);
    }
    herr_t status = H5Oget_info_by_name(hdf_file,pathname,&object_info,H5P_DEFAULT);
    if (status < 0) {
        LOG_WARN("error getting status for '%s'",pathname);
        errno=ENOENT;
        return(-1);
    }
    (*buf)=hdf_file_stat;
    buf->st_blksize=4096;
    buf->st_atime  =object_info.atime;
    buf->st_mtime  =object_info.mtime;
    buf->st_ctime  =object_info.ctime;
    if (object_info.num_attrs > 0) {
        hid_t lena = H5Aopen_by_name(hdf_file,pathname,"Filesize",H5P_DEFAULT,H5P_DEFAULT);
        int64_t fsize;
        H5Aread(lena,H5T_NATIVE_INT64,&fsize);
        H5Aclose(lena);
        buf->st_size=fsize;
        buf->st_blocks=fsize / 512 + 1;
    } else {
        buf->st_size=0;
        buf->st_blocks=0;
    }
    LOG_DBG("size of '%s': %d",pathname,(int)buf->st_size);
    return(0);
}

int hdf5_fstat64(int fd, struct stat64 *buf) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    (*buf)=hdf_file_stat;
    if (d->dataset == NULL) {
        buf->st_blksize = 4096;
        buf->st_size = 0;
    } else {
        buf->st_blksize = d->dataset->chunk[0];
        buf->st_size    = d->dataset->length;
    }
    buf->st_blocks=buf->st_size / 512 + 1;
    if ((d->dataset != NULL) && (d->dataset->set >= 0)) {
        H5O_info_t object_info;
        herr_t status = H5Oget_info(d->dataset->set,&object_info);
        if (status < 0) {
            LOG_WARN("error getting status for '%s'",d->name);
            errno=EIO;
            return(-1);
        }
        buf->st_atime  =object_info.atime;
        buf->st_mtime  =object_info.mtime;
        buf->st_ctime  =object_info.ctime;
    }
    LOG_DBG("size of '%s': %d",d->name,(int)buf->st_size);
    return(0);
}

int hdf5_unlink(const char *pathname) {
    if (file_ds_exists(hdf_file,pathname)==0) {
        if (string_set_remove(closed_empty_files,pathname))
            return(0);
        errno=ENOENT;
        return(-1);
    }
    herr_t res = H5Ldelete(hdf_file,pathname,H5P_DEFAULT);
    if (res < 0) {
        errno=EIO;
        return(-1);
    }
    return(0);
}
