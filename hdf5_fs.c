#include <sys/types.h>
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <hdf5.h>
#include <hdf5_hl.h>
#include "hdf5_fs.h"
#include "string_set.h"
#include "wrapper_limits.h"
#include "logger.h"

#define RANK 1
hid_t   hdf_file;
hid_t   create_params;
char    fillvalue = 0;
hsize_t chunk_dims[1]={1024*64};
hsize_t maxdims[1] = {H5S_UNLIMITED};
struct stat64 hdf_file_stat;
string_set * closed_empty_files;
string_set * datasets;

typedef struct hdf5_dataset_info {
    hid_t   space;
    hid_t   set;
    hsize_t dims[RANK];
    int     length;
    int     refcount;
    struct  hdf5_dataset_info * prev;
    struct  hdf5_dataset_info * next;
} hdf5_dataset_info_t;

typedef struct {
    hdf5_dataset_info_t * dataset;
    int     append;
    hsize_t offset[RANK];
    char  name[PATH_MAX];
} hdf5_data_t;

int     last_handle=-1;
hdf5_data_t * hdf5_data[HANDLES_MAX];
hdf5_dataset_info_t dataset_info_list;

int _hdf5_path_exists(const char *pathname);

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
    create_params = H5Pcreate(H5P_DATASET_CREATE);
    status        = H5Pset_chunk(create_params, 1, chunk_dims);
    if (status < 0) {
        LOG_WARN("error setting um chunking");
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
    int set_exists = _hdf5_path_exists(pathname);

    hid_t  set=-1;
    if (set_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            LOG_DBG("dataset already exists %d -> '%s'",fd,pathname);
            errno=EEXIST;
            return(-1);
        }
        set = H5Dopen(hdf_file,pathname,H5P_DEFAULT);
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
    d->dataset  = malloc(sizeof(hdf5_dataset_info_t));
    if (hdf5_data[fd]->dataset == NULL) {
        LOG_WARN("error allocating hdf5_data[%d]->dataset",fd);
        errno = ENOMEM;
        return(-1);
    }
    strncpy(d->name,pathname,PATH_MAX);
    d->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    d->dataset->set      = set;
    d->dataset->space    = -1;
    if (d->dataset->set < 0) {
        if (flags & O_CREAT) {
            d->dataset->length  = 0;
            d->offset[0] = 0;
            d->dataset->dims[0] = 1; // hdf5 does not zero-length datasets. make dims[0]=length+1;
            return(fd);
        } else {
            LOG_WARN("error opening dataset '%s' %d",hdf5_data[fd]->name,hdf5_data[fd]->dataset->set);
            free(hdf5_data[fd]->dataset);
            free(hdf5_data[fd]);
            hdf5_data[fd]=NULL;
            errno=ENOENT;
            return(-1);
        }
    } else {
        d->dataset->space = H5Dget_space(set);
        H5Sget_simple_extent_dims(d->dataset->space, d->dataset->dims, NULL);
        if (flags & O_TRUNC) {
            d->dataset->length  = 0;
            d->offset[0] = 0;
        } else {
            H5LTget_attribute_int(hdf_file,pathname,"length",&d->dataset->length);
            d->offset[0] = 0;
        }
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
    if (d->dataset->set >= 0) {
        H5LTset_attribute_int(hdf_file,d->name,"length",&d->dataset->length,1);
        hsize_t newdim = d->dataset->length + 1;
        if (newdim != d->dataset->dims[0]) {
            d->dataset->dims[0]=newdim;
            H5Dset_extent(d->dataset->set, d->dataset->dims);
        }
        if ((status = H5Dclose(hdf5_data[fd]->dataset->set)) < 0) {
            LOG_WARN("error closing dataset '%s' %d",d->name,status);
            errno = EIO;
            return(-1);
        }
    } else {
        string_set_add(closed_empty_files, d->name, NULL);
    }
    if ((d->dataset->space >=0) && ((status = H5Sclose(d->dataset->space)) < 0)) {
        LOG_WARN("error closing dataspace '%s' %d",d->name,status);
        errno = EIO;
        return(-1);
    }
    free(d->dataset);
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
    if (d->append)
        d->offset[0]=d->dataset->length;
    int resize = 0;
    int needed_dim = d->offset[0]+count+1;
    if (needed_dim > d->dataset->dims[0]) {
        resize = 1;
        d->dataset->dims[0]=needed_dim;
    }
    LOG_DBG("(%d='%s', %d) %d (%d)\n",fd,d->name,(int)count,(int)d->offset[0],(int)d->dataset->length);
    if (d->dataset->set < 0) {
        d->dataset->space = H5Screate_simple(RANK, d->dataset->dims, maxdims);
        d->dataset->set = H5Dcreate2(hdf_file, d->name, H5T_NATIVE_CHAR, d->dataset->space, H5P_DEFAULT, create_params, H5P_DEFAULT);
    } else if (resize) {
        H5Dset_extent(d->dataset->set, d->dataset->dims);
    }
    hsize_t hs_count[RANK];
    hs_count[0]=count;
    hid_t filespace = H5Dget_space(d->dataset->set);
    hid_t dataspace = H5Screate_simple(RANK, hs_count, NULL);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, d->offset, NULL, hs_count, NULL);

    H5Dwrite(d->dataset->set,H5T_NATIVE_CHAR,dataspace,filespace,H5P_DEFAULT,buf);
    H5Sclose(filespace);
    H5Sclose(dataspace);
    if ((d->offset[0]+count) > d->dataset->length)
        d->dataset->length = d->offset[0]+count;
    d->offset[0]+=count;
    return(count);
}

int hdf5_read(int fd, void *buf, size_t count) {
    if (count == 0)
        return(0);
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("write on unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    // end of file
    if (d->offset[0] >= d->dataset->length)
        return(0);
    if (d->dataset->set < 0)
        return(0);
    size_t remaining_count = d->dataset->length - d->offset[0];
    if (remaining_count > count)
        remaining_count = count;
    LOG_DBG("(%d='%s', %d) %d (%d)",fd,d->name,(int)count,(int)d->offset[0],(int)d->dataset->length);

    hsize_t hs_count[RANK];
    hs_count[0]=remaining_count;
    hid_t filespace = H5Dget_space(d->dataset->set);
    hid_t dataspace = H5Screate_simple(RANK, hs_count, NULL);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, d->offset, NULL, hs_count, NULL);

    H5Dread(d->dataset->set,H5T_NATIVE_CHAR,dataspace,filespace,H5P_DEFAULT,buf);
    H5Sclose(filespace);
    H5Sclose(dataspace);
    d->offset[0]+=remaining_count;
    return(remaining_count);
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
            d->offset[0]=d->dataset->length+offset;
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
    if (_hdf5_path_exists(pathname) == 0) {
        if (string_set_find(closed_empty_files, pathname) <= 0) {
            errno=ENOENT;
            return(-1);
        }
        (*buf)=hdf_file_stat;
        buf->st_blksize=chunk_dims[0];
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
    buf->st_blksize=chunk_dims[0];
    buf->st_atime  =object_info.atime;
    buf->st_mtime  =object_info.mtime;
    buf->st_ctime  =object_info.ctime;
    if (object_info.num_attrs > 0) {
        int fsize;
        H5LTget_attribute_int(hdf_file,pathname,"length",&fsize);
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
    buf->st_blksize=chunk_dims[0];
    buf->st_size=d->dataset->length;
    buf->st_blocks=d->dataset->length / 512 + 1;
    if (d->dataset->set >= 0) {
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
    if (_hdf5_path_exists(pathname)==0) {
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

int _hdf5_path_exists(const char *pathname) {
    int pathlen=strnlen(pathname,PATH_MAX);
    if (pathlen == 0) return(1);
    char testpath[PATH_MAX];
    strncpy(testpath,pathname,PATH_MAX);
    int i;
    htri_t testres;
    for (i=0;i<pathlen;i++) {
        if (testpath[i] == '/') {
            testpath[i] = 0;
            testres=H5Lexists(hdf_file,testpath,H5P_DEFAULT);
            if (testres <= 0)
                return(0);
            testpath[i] = '/';
        }
    }
    testres=H5Lexists(hdf_file,testpath,H5P_DEFAULT);
    if (testres <= 0)
        return(0);
    return(1);
}

