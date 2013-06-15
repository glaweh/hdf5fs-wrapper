#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <hdf5.h>
#include <errno.h>
#include "logger.h"
#include "string_set.h"
#include "khash.h"

#define MAX_HDF5SRC 1024
#define RANK 1

int   n_hdf_src;
hid_t hdf_src[MAX_HDF5SRC];
hid_t hdf_dst;

typedef struct hdf5_dataset_info {
    hid_t   space;
    hid_t   set;
    hid_t   length_space;
    hid_t   length_attrib;
    hid_t   file;
    hsize_t dims[RANK];
    hsize_t chunk[RANK];
    int64_t length;
    int64_t length_original;
    int     refcount;
    int     rdonly;
    struct  hdf5_dataset_info * prev;
    struct  hdf5_dataset_info * next;
} hdf5_dataset_info_t;

typedef struct {
    int n_sets;
    hdf5_dataset_info_t * dataset;
} file_node_t;

KHASH_MAP_INIT_STR(42,file_node_t *)

khash_t(42) * filelist = NULL;

#define DIM_CHUNKED(length,chunk) (length + (chunk - (length % chunk)))

hid_t hdf5_dataset_close(const char *name, hdf5_dataset_info_t * info) {
    if (! info->rdonly) {
        if ((info->length != info->length_original) && 
                (H5Awrite(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0)) {
            LOG_ERR("error writing updated filesize attrib for '%s'",name);
        }
        hsize_t old_dim = info->dims[0];
        info->dims[0] = DIM_CHUNKED(info->length+1,info->chunk[0]);
        if ((old_dim != info->dims[0]) && 
                (H5Dset_extent(info->set, info->dims)<0)) {
            LOG_ERR("error resizing datset for '%s' to %d",name,info->dims[0]);
        }
        // FIXME: use H5Dfill to fill the emtpy part
    }
    int status = 1;
    if ((info->length_space >= 0) && (H5Sclose(info->length_space) < 0)) {
        LOG_ERR("error closing length space for '%s'",name);
        status = 0;
    } else {
        info->length_space = -1;
    }
    if ((info->length_attrib >= 0) && (H5Aclose(info->length_attrib) < 0)) {
        LOG_ERR("error closing length attribute for '%s'",name);
        status = 0;
    } else {
        info->length_attrib=-1;
    }
    if ((info->space >= 0) && (H5Sclose(info->space) < 0)) {
        LOG_ERR("error closing data space for '%s'",name);
        status = 0;
    } else {
        info->space=-1;
    }
    if ((info->set >= 0) && (H5Dclose(info->set) < 0)) {
        LOG_ERR("error closing dataset '%s'",name);
        status = 0;
    }
    return(status);
}

hid_t hdf5_dataset_open(hid_t loc_id, const char *name, hdf5_dataset_info_t * info) {
    info->rdonly = 1;
    if ((info->set = H5Dopen(loc_id,name,H5P_DEFAULT)) < 0) {
        LOG_ERR("error opening dataset '%s'",name);
        goto errlabel;
    }
    if ((info->space = H5Dget_space(info->set)) < 0) {
        LOG_ERR("error getting dataspace for '%s'",name);
        goto errlabel;
    }
    if (H5Sget_simple_extent_dims(info->space, info->dims, NULL) < 0) {
        LOG_ERR("error getting dimensions of '%s'",name);
        goto errlabel;
    }
    info->length_attrib = H5Aopen(info->set,"Filesize",H5P_DEFAULT);
    if ((info->length_attrib = H5Aopen(info->set,"Filesize",H5P_DEFAULT)) < 0) {
        LOG_ERR("error opening filesize attribute of '%s'",name);
        goto errlabel;
    }
    if ((info->length_space = H5Aget_space(info->length_attrib)) < 0) {
        LOG_ERR("error opening filesize attribute dataspace of '%s'",name);
        goto errlabel;
    }
    if (H5Aread(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0) {
        LOG_ERR("Error reading filesize attrib for '%s'",name);
        goto errlabel;
    }
    hid_t dcpl;
    if ((dcpl = H5Dget_create_plist(info->set)) < 0) {
        LOG_ERR("error getting create plist for '%s'",name);
        goto errlabel;
    }
    if (H5D_CHUNKED != H5Pget_layout(dcpl)) {
        LOG_ERR("dataset '%s' is not chunked",name);
        goto err_dcpl;
    }
    if (H5Pget_chunk(dcpl,RANK,info->chunk) < 0) {
        LOG_ERR("error getting chunk properties for '%s'",name);
        goto err_dcpl;
    }
    H5Pclose(dcpl);
    info->length_original = info->length;
    return(info->set);
    err_dcpl:
        H5Pclose(dcpl);
    errlabel:
        hdf5_dataset_close(name,info);
    return(-1);
}

hdf5_dataset_info_t * hdf5_dataset_info(hid_t loc_id, const char *name) {
    H5O_info_t      infobuf;
    herr_t          status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    if ((status < 0) || (infobuf.type != H5O_TYPE_DATASET)) return(NULL);
    hdf5_dataset_info_t * info = malloc(sizeof(hdf5_dataset_info_t));
    if (hdf5_dataset_open(loc_id,name,info) > 0) {
        LOG_INFO("dsinfo %s %d",name,info->length);
        khiter_t k;
        int ret, is_missing;
        k=kh_get(42, filelist, name);
        is_missing = (k == kh_end(filelist));
        if (is_missing) {
            LOG_INFO("file '%s' missing from list",name);
        } else {
            LOG_INFO("file '%s' already in list",name);
        }

        if (hdf5_dataset_close(name,info) < 0) {
            LOG_ERR("error closing file '%s'");
        }
        free(info);
    } else {
        LOG_ERR("error opening file '%s'");
    }
    return(NULL);
}

static herr_t op_func_L (hid_t loc_id, const char *name, const H5L_info_t *info,
			 void *operator_data)
{
    herr_t          status;
    H5O_info_t      infobuf;

    /*
     * Get type of the object and display its name and type.
     * The name of the object is passed to this function by
     * the Library.
     */
    hdf5_dataset_info(loc_id,name);
    return(0);
    status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    if(status < 0) return -1;
    if (infobuf.type != H5O_TYPE_DATASET) return (0);
//    printf("oname: '%s'\n",name);
    return(0);

//    return op_func (loc_id, name, &infobuf, operator_data);
}

int hdf5_ls(hid_t file_id, const char * root_name) {
    herr_t status;
    status = H5Lvisit_by_name(file_id, root_name, H5_INDEX_NAME, H5_ITER_INC, op_func_L, NULL, H5P_DEFAULT );
    return(status);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr,"usage: %s <target> <src1> [src2] [src3] [src4]...\n",argv[0]);
        return(1);
    }
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

    filelist = kh_init(42);
    n_hdf_src=0;
    int i;
    for (i=2; i<argc;i++) {
        if (stat(argv[i],&hdf_file_stat) == 0) {
            hid_t this_src = H5Fopen(argv[i],H5F_ACC_RDONLY,H5P_DEFAULT);
            if ( this_src >=0 ) {
                hdf_src[n_hdf_src]=this_src;
                n_hdf_src++;
                printf("==== %s ====\n",argv[i]);
                hdf5_ls(this_src,"/");
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
    kh_destroy(42,filelist);
    for (i=0; i < n_hdf_src; i++) H5Fclose(hdf_src[i]);
    H5Fclose(hdf_dst);
    return(0);
}
