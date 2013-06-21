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

hdirent_t * root = NULL;

int unpack_set_stack(const char * parent, hdirent_t * node, void * op_data) {
    char export_name[PATH_MAX];
    strcpy(export_name,node->name);
    int i,name_len;
    name_len = strlen(export_name);
    for (i=0; i<name_len; i++) {
        if (export_name[i]=='%') export_name[i]='/';
    }
    hfile_ds_reopen(node->dataset);
    int status = hfile_ds_export(node->dataset,export_name);
    if (status < 0) {
        LOG_ERR("error exporting dataset '%s'",export_name);
        return(-1);
    }
    hfile_ds_close(node->dataset);
    return(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,"usage: %s <src1> [src2] [src3] [src4]...\n",argv[0]);
        return(1);
    }

    n_hdf_src=0;
    int i;
    struct stat hdf_file_stat;
    root = hdir_new("/");
    for (i=1; i<argc;i++) {
        if (stat(argv[i],&hdf_file_stat) == 0) {
            hid_t this_src = H5Fopen(argv[i],H5F_ACC_RDONLY,H5P_DEFAULT);
            if ( this_src >=0 ) {
                hdf_src[n_hdf_src]=this_src;
                n_hdf_src++;
                printf("==== %s ====\n",argv[i]);
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
        return(1);
    }
    hdir_foreach_file(root,HDIRENT_ITERATE_UNORDERED,unpack_set_stack,NULL);
    hdir_free(root);
    for (i=0; i < n_hdf_src; i++) H5Fclose(hdf_src[i]);
    return(0);
}
