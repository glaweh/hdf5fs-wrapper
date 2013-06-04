#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <hdf5.h>
#include "hdf5_fs.h"
#include "wrapper_limits.h"

#define RANK 1
hid_t   hdf_file;
hid_t   create_params;
char    fillvalue = 0;
hsize_t chunk_dims[1]={4096};
hsize_t maxdims[1] = {H5S_UNLIMITED};

typedef struct {
    hid_t   space;
    hid_t   set;
    int     append;
    int     truncate;
    hsize_t dims[RANK];
    hsize_t offset[RANK];
    hsize_t length;
    char  name[PATH_MAX];
} hdf5_file_member_t;

int     last_handle=-1;
hdf5_file_member_t hdf5_data[HANDLES_MAX];

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
    for (i=0;i<HANDLES_MAX;i++) {
        hdf5_data[i].set=-1;
        hdf5_data[i].name[0]=0;
    }
    last_handle = -1;
    return(1);
}

int hdf5_fs_fini() {
    int i;
    for (i=0;i<=last_handle;i++) {
        if (hdf5_data[i].set > 0) {
            H5Dclose(hdf5_data[i].set);
            H5Sclose(hdf5_data[i].space);
        }
    }
    H5Fclose(hdf_file);
    return(1);
}

int hdf5_open(int fd, const char *pathname, int flags) {
    if (hdf5_data[fd].set >= 0) {
        fprintf(stderr,"fd already in use\n");
        return(-1);
    }
    strncpy(hdf5_data[fd].name,pathname,PATH_MAX);
    hdf5_data[fd].truncate = ((flags & O_TRUNC)  != 0 ? 1 : 0);
    hdf5_data[fd].append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    hdf5_data[fd].set = H5Dopen(hdf_file,hdf5_data[fd].name,H5P_DEFAULT);
    if (hdf5_data[fd].set < 0) {
        if (flags & O_CREAT) {
            return(fd);
        } else {
            fprintf(stderr,"error opening dataset '%s' %d\n",hdf5_data[fd].name,hdf5_data[fd].set);
            hdf5_data[fd].name[0]=0;
            return(-1);
        }
    }
    if (fd > last_handle) last_handle = fd;
    return(fd);
}

int hdf5_close(int fd) {
    int status;
    if ((hdf5_data[fd].set >= 0) && ((status = H5Dclose(hdf5_data[fd].set)) < 0)) {
        fprintf(stderr,"error closing dataset '%s' %d\n",hdf5_data[fd].name,status);
    }
    if ((hdf5_data[fd].space >=0) && ((status = H5Sclose(hdf5_data[fd].space)) < 0)) {
        fprintf(stderr,"error closing dataspace '%s' %d\n",hdf5_data[fd].name,status);
    }
    hdf5_data[fd].set=-1;
    hdf5_data[fd].name[0]=0;
    while ((last_handle >= 0) && (hdf5_data[last_handle].name[0] == 0)) {
        last_handle--;
    }
    return(1);
}

int hdf5_write(int fd, const void *buf, size_t count) {
    if (count == 0)
        return(0);
    hdf5_data[fd].dims[0]+=count;
    if (hdf5_data[fd].set < 0) {
        hdf5_data[fd].space = H5Screate_simple(RANK, hdf5_data[fd].dims, maxdims);
    }
    return(1);
}
