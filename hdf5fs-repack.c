#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <hdf5.h>
#include <errno.h>
#include "hfile_ds.h"
#include "logger.h"
#include "hdir.h"

#define MAX_HDF5SRC 1024

int   n_hdf_src;
hid_t hdf_src[MAX_HDF5SRC];
hid_t hdf_dst;

hdirent_t * root = NULL;

typedef struct copy_set_stack_data {
    hid_t target_file;
    int compress;
} copy_set_stack_data_t;

int copy_set_stack(const char * parent, hdirent_t * node, void * op_data) {
    copy_set_stack_data_t * css_data=op_data;
    hfile_ds_reopen(node->dataset);
    hfile_ds_t * target_set = hfile_ds_copy(css_data->target_file, node->dataset, 0, css_data->compress);
    if (target_set == NULL) {
        LOG_ERR("error copying dataset '%s'",node->name);
        return(-1);
    }
    hfile_ds_close(node->dataset);
    hfile_ds_close(target_set);
    free(target_set);
    return(0);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"usage: %s <target> <src1> [src2] [src3] [src4]...\n",argv[0]);
        return(1);
    }

    n_hdf_src=0;
    int i;
    struct stat hdf_file_stat;
    if (stat(argv[1],&hdf_file_stat) == 0) {
        LOG_FATAL("file '%s' does already exist",argv[1]);
        return(1);
    }
    hdf_dst = H5Fcreate(argv[1],H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (hdf_dst < 0) {
        LOG_FATAL("error creating target file '%s'",argv[1]);
        return(1);
    }
    root = hdir_new("/");
    for (i=2; i<argc;i++) {
        if (stat(argv[i],&hdf_file_stat) == 0) {
            hid_t this_src = H5Fopen(argv[i],H5F_ACC_RDONLY,H5P_DEFAULT);
            if ( this_src >=0 ) {
                hdf_src[n_hdf_src]=this_src;
                n_hdf_src++;
                fprintf(stderr,"==== %s ====\n",argv[i]);
                hdir_add_hdf5(root,this_src,1);
            } else {
                LOG_WARN("error opening src file '%s'",argv[i]);
            }
        } else {
            LOG_WARN("missing src file '%s'",argv[i]);
        }
    }
    if (n_hdf_src == 0) {
        LOG_FATAL("no src files could be opened");
        H5Fclose(hdf_dst);
        return(1);
    }
    copy_set_stack_data_t css_data;
    css_data.target_file = hdf_dst;
    css_data.compress    = 3;
    hdir_foreach_file(root,HDIRENT_ITERATE_UNORDERED,copy_set_stack,&css_data);
    hdir_free(root);
    for (i=0; i < n_hdf_src; i++) H5Fclose(hdf_src[i]);
    H5Fclose(hdf_dst);
    return(0);
}
