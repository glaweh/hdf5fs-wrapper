#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <hdf5.h>
#include <hdf5_hl.h>
#include "hdf5_fs.h"
#include "wrapper_limits.h"

#define RANK 1
hid_t   hdf_file;
hid_t   create_params;
char    fillvalue = 0;
hsize_t chunk_dims[1]={1024*64};
hsize_t maxdims[1] = {H5S_UNLIMITED};

typedef struct {
    hid_t   space;
    hid_t   set;
    int     append;
    int     truncate;
    hsize_t dims[RANK];
    int length;
    hsize_t offset;
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
#ifdef DEBUG
    fprintf(stderr,"hdf_file opened: '%s', %d\n",hdf_filename,hdf_file);
#endif
    if (hdf_file < 0) {
        fprintf(stderr,"hdf_file error\n");
        return(-1);
    }
    create_params = H5Pcreate(H5P_DATASET_CREATE);
    status        = H5Pset_chunk(create_params, 1, chunk_dims);
    if (status < 0) {
        fprintf(stderr,"error setting um chunking\n");
        return(-1);
    }
    int i;
    for (i=0;i<HANDLES_MAX;i++)
        hdf5_data[i]=NULL;
    last_handle = -1;
    return(1);
}

int hdf5_fs_fini() {
    int i;
    for (i=0;i<=last_handle;i++) {
        if (hdf5_data[i] != NULL)
            hdf5_close(i);
    }
    H5Fclose(hdf_file);
    return(1);
}

int hdf5_open(int fd, const char *pathname, int flags) {
    if (hdf5_data[fd] != NULL) {
        fprintf(stderr,"fd already in use\n");
        return(-1);
    }
    fprintf(stderr,"hdf5_open(%d,'%s',%o)\n",fd,pathname,flags);
    htri_t set_exists = H5Lexists(hdf_file,pathname,H5P_DEFAULT);

    hid_t  set=-1;
    if (set_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            fprintf(stderr,"dataset already exists %d -> '%s'\n",fd,pathname);
            errno=EEXIST;
            return(-1);
        }
        set = H5Dopen(hdf_file,pathname,H5P_DEFAULT);
    }
    hdf5_data[fd] = malloc(sizeof(hdf5_data_t));
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"error allocating hdf5_data[%d]\n",fd);
        errno = ENOMEM;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    strncpy(d->name,pathname,PATH_MAX);
    d->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    d->set      = set;
    d->space    = -1;
    if (d->set < 0) {
        if (flags & O_CREAT) {
            d->length  = 0;
            d->offset  = 0;
            d->dims[0] = 1;
            return(fd);
        } else {
            fprintf(stderr,"error opening dataset '%s' %d\n",hdf5_data[fd]->name,hdf5_data[fd]->set);
            free(hdf5_data[fd]);
            hdf5_data[fd]=NULL;
            errno=ENOENT;
            return(-1);
        }
    } else {
        d->space = H5Dget_space(set);
        H5Sget_simple_extent_dims(d->space, d->dims, NULL);
        if (flags & O_TRUNC) {
            d->length  = 0;
            d->offset  = 0;
        } else {
            H5LTget_attribute_int(hdf_file,pathname,"length",&d->length);
            d->offset = 0;
        }
    }
    if (fd > last_handle) last_handle = fd;
    return(fd);
}

int hdf5_close(int fd) {
    int status;
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"close on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    if (hdf5_data[fd]->set >= 0) {
        H5LTset_attribute_int(hdf_file,hdf5_data[fd]->name,"length",&hdf5_data[fd]->length,1);
        if ((status = H5Dclose(hdf5_data[fd]->set)) < 0) {
            fprintf(stderr,"error closing dataset '%s' %d\n",hdf5_data[fd]->name,status);
            errno = EIO;
            return(-1);
        }
    }
    if ((hdf5_data[fd]->space >=0) && ((status = H5Sclose(hdf5_data[fd]->space)) < 0)) {
        fprintf(stderr,"error closing dataspace '%s' %d\n",hdf5_data[fd]->name,status);
        errno = EIO;
        return(-1);
    }
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
        fprintf(stderr,"write on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    if (d->append)
        d->offset=d->length;
    int resize = 0;
    if ((d->offset+count) > d->dims[0]) {
        resize = 1;
        d->dims[0]=d->offset+count;
    }
    fprintf(stderr,"hdf5_write(%d='%s', %d) %d (%d)\n",fd,d->name,(int)count,(int)d->offset,(int)d->length);
    if (d->set < 0) {
        d->space = H5Screate_simple(RANK, d->dims, maxdims);
        d->set = H5Dcreate2(hdf_file, d->name, H5T_NATIVE_CHAR, d->space, H5P_DEFAULT, create_params, H5P_DEFAULT);
    } else if (resize) {
        H5Dset_extent(d->set, d->dims);
    }
    hsize_t hs_count[RANK];
    hs_count[0]=count;
    hid_t filespace = H5Dget_space(d->set);
    hid_t dataspace = H5Screate_simple(RANK, hs_count, NULL);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, &d->offset, NULL, hs_count, NULL);

    H5Dwrite(d->set,H5T_NATIVE_CHAR,dataspace,filespace,H5P_DEFAULT,buf);
    H5Sclose(filespace);
    H5Sclose(dataspace);
    if ((d->offset+count) > d->length)
        d->length = d->offset+count;
    d->offset+=count;
    return(count);
}

int hdf5_lseek(int fd, off_t offset, int whence) {
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"write on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    switch (whence) {
        case SEEK_SET:
            d->offset=offset;
            break;
        case SEEK_END:
            d->offset=d->length+offset;
            break;
        case SEEK_CUR:
            d->offset+=offset;
            break;
        default:
            errno=EINVAL;
            return(-1);
            break;
    }
    return(d->offset);
}
