#include <sys/stat.h>
#include <hdf5.h>
#include "hdf5_fs.h"
#include "wrapper_limits.h"

hid_t hdf_file;

int hdf5_fs_init(const char * hdf_filename) {
    struct stat hdf_stat;
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
        return(0);
    }
    return(1);
}

int hdf5_fs_fini() {
    H5Fclose(hdf_file);
    return(0);
}


