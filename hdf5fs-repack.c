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
#define HDF5FS_T H5T_NATIVE_CHAR

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
    struct  hdf5_dataset_info * next;
} hdf5_dataset_info_t;

typedef struct {
    int n_sets;
    hdf5_dataset_info_t * dataset;
    char name[0];
} file_node_t;

KHASH_MAP_INIT_STR(42,file_node_t *)

khash_t(42) * filelist = NULL;

#define DIM_CHUNKED(length,chunk) ((length) + ((chunk) - ((length) % (chunk))))

hid_t hdf5_dataset_close(const char *name, hdf5_dataset_info_t * info) {
    if (! info->rdonly) {
        if ((info->length != info->length_original) && 
                (H5Awrite(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0)) {
            LOG_ERR("error writing updated filesize attrib for '%s'",name);
        }
        hsize_t old_dim = info->dims[0];
        info->dims[0] = DIM_CHUNKED(info->length+1,info->chunk[0]);
        LOG_DBG("resizing %40s, length %6d, chunksize %6d, old_dim %6d, new_dim %6d",
                name,info->length,info->chunk[0],old_dim,info->dims[0]);
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

hsize_t maxdims[1] = {H5S_UNLIMITED};

hid_t hdf5_dataset_create(hid_t loc_id, const char *name, hdf5_dataset_info_t * info) {
    info->rdonly = 1;
    info->length = info->length_original = 0;
    /*
     * input:
     *   info->chunk
     *   info->dims
     * */
    if (info->chunk[0] <= 0) {
        LOG_ERR("no chunk size set for '%s'",name);
        goto errlabel;
    }
    hid_t   create_params = H5Pcreate(H5P_DATASET_CREATE);
    herr_t         status = H5Pset_chunk(create_params, 1, info->chunk);
    if (status < 0) {
        LOG_WARN("error setting up chunking");
        goto errlabel;
    }
    info->dims[0] = DIM_CHUNKED(info->dims[0],info->chunk[0]);
    if (info->dims[0] == 0) info->dims[0] = info->chunk[0];
    LOG_DBG("create %40s, chunksize %d, dim %d",name,info->chunk[0],info->dims[0]);
    if ((info->space = H5Screate_simple(RANK, info->dims, maxdims)) < 0) {
        LOG_ERR("error creating dataspace for '%s'",name);
        goto errlabel;
    }
    if ((info->set = H5Dcreate2(loc_id,name,HDF5FS_T, info->space, H5P_DEFAULT, create_params, H5P_DEFAULT)) < 0) {
        LOG_ERR("error creating dataset '%s'",name);
        goto errlabel;
    }
    H5Pclose(create_params);
    if ((info->length_space = H5Screate(H5S_SCALAR)) < 0) {
        LOG_ERR("error creating filesize attribute dataspace of '%s'",name);
        goto errlabel;
    }
    if ((info->length_attrib = H5Acreate2(info->set,"Filesize",H5T_NATIVE_INT64,
                    info->length_space, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        LOG_ERR("error creating filesize attribute of '%s'",name);
        goto errlabel;
    }
    if (H5Awrite(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0) {
        LOG_ERR("Error resetting filesize attrib for '%s'",name);
        goto errlabel;
    }
    info->rdonly=0;
    return(info->set);
    errlabel:
        hdf5_dataset_close(name,info);
    return(-1);
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
        LOG_DBG("dsinfo %s %d",name,info->length);
        khiter_t k;
        int ret, is_missing;
        k=kh_get(42, filelist, name);
        is_missing = (k == kh_end(filelist));
        file_node_t * node;
        if (is_missing) {
            node = malloc(sizeof(file_node_t)+strlen(name)+1);
            if (node == NULL) {
                LOG_ERR("malloc error in node creation '%s'",name);
                free(info);
                return(NULL);
            }
            node->n_sets = 1;
            node->dataset = info;
            strcpy(node->name,name);
            k = kh_put(42,filelist,node->name,&ret);
            if (!ret) {
                LOG_ERR("error adding hash key '%s'",name);
                kh_del(42,filelist,k);
                free(node);
                free(info);
                return(NULL);
            }
            kh_value(filelist,k)=node;
            info->next = NULL;
            LOG_DBG("file '%s' missing from list",name);
        } else {
            node=kh_value(filelist,k);
            info->next=node->dataset;
            node->n_sets++;
            node->dataset=info;
            LOG_DBG("file '%s' already in list",name);
        }
//        if (hdf5_dataset_close(name,info) < 0) {
//            LOG_ERR("error closing file '%s'");
//        }
//        free(info);
    } else {
        LOG_ERR("error opening file '%s'");
        free(info);
        return(NULL);
    }
    return(info);
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

file_node_t * close_node(file_node_t * node) {
    hdf5_dataset_info_t * setwalker = node->dataset;
    while (setwalker != NULL) {
        hdf5_dataset_info_t * walker_prev = setwalker;
        setwalker=walker_prev->next;
        hdf5_dataset_close(node->name,walker_prev);
        free(walker_prev);
    }
    free(node);
    return(NULL);
}

void filelist_clean() {
    khiter_t k;
    for (k = kh_begin(filelist); k != kh_end(filelist); ++k) {
        if (kh_exist(filelist, k)) {
            close_node(kh_value(filelist,k));
        }
    }
    kh_destroy(42,filelist);
}

hsize_t suggest_chunk_size_from_length(hsize_t old_length) {
    if (old_length < 16*1024) {
        return(2*1024);
    } else if (old_length < 32*1024) {
        return(4*1024);
    } else if (old_length < 128*1024) {
        return(64*1024);
    } else if (old_length < 10*1024*1024) {
        return(256*1024);
    } else {
        return(1024*1024);
    }
}

herr_t copy_set_stack(hid_t loc_id, file_node_t * node) {
    hdf5_dataset_info_t target_set;
    target_set.dims[0]  = node->dataset->length;
    target_set.chunk[0] = suggest_chunk_size_from_length(node->dataset->length);
    hdf5_dataset_create(loc_id, node->name, &target_set);
    target_set.length = node->dataset->length;
    hdf5_dataset_close(node->name,&target_set);
    return(0);
}

herr_t copy_all_files() {
    khiter_t k;
    for (k = kh_begin(filelist); k != kh_end(filelist); ++k) {
        if (kh_exist(filelist, k)) {
            copy_set_stack(hdf_dst,kh_value(filelist,k));
        }
    }
    return(0);
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
    copy_all_files();
    filelist_clean();
    for (i=0; i < n_hdf_src; i++) H5Fclose(hdf_src[i]);
    H5Fclose(hdf_dst);
    return(0);
}
