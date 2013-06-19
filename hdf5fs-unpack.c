#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <hdf5.h>
#include <errno.h>
#include "file_ds.h"
#include "logger.h"
#include "khash.h"

#define MAX_HDF5SRC 1024

int   n_hdf_src;
hid_t hdf_src[MAX_HDF5SRC];

typedef struct {
    int n_sets;
    hfile_ds_t * dataset;
    char name[0];
} file_node_t;

KHASH_MAP_INIT_STR(42,file_node_t *)

khash_t(42) * filelist = NULL;

hfile_ds_t * hfile_ds(hid_t loc_id, const char *name) {
    H5O_info_t      infobuf;
    herr_t          status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    if ((status < 0) || (infobuf.type != H5O_TYPE_DATASET)) return(NULL);
    hfile_ds_t * info = hfile_ds_open(loc_id,name);
    if (info != NULL) {
        LOG_DBG("dsinfo %s %"PRIi64"",name,info->length);
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
    } else {
        LOG_ERR("error opening file '%s'",name);
        free(info);
        return(NULL);
    }
    return(info);
}

static herr_t op_func_L (hid_t loc_id, const char *name, const H5L_info_t *info,
			 void *operator_data)
{
    hfile_ds(loc_id,name);
    return(0);
}

int hdf5_ls(hid_t file_id, const char * root_name) {
    herr_t status;
    status = H5Lvisit_by_name(file_id, root_name, H5_INDEX_NAME, H5_ITER_INC, op_func_L, NULL, H5P_DEFAULT );
    return(status);
}

file_node_t * close_node(file_node_t * node) {
    while (node->dataset != NULL) {
        hfile_ds_t * walker = node->dataset;
        node->dataset=node->dataset->next;
        hfile_ds_close(walker);
        free(walker);
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

herr_t unpack_set_stack(file_node_t * node) {
    char export_name[PATH_MAX];
    strcpy(export_name,node->name);
    int i,name_len;
    struct stat dirstat;
    name_len = strlen(export_name);
    for (i=0; i<name_len; i++) {
        if (export_name[i]=='%') {
            export_name[i]=0;
            if (stat(export_name,&dirstat) >= 0) {
                if (! S_ISDIR(dirstat.st_mode)) {
                    LOG_ERR("'%s' exists and is not a directory",export_name);
                    return(-1);
                }
            } else {
                if (mkdir(export_name,0770) < 0) {
                    LOG_ERR("error creating directory '%s'",export_name);
                }
            }
            export_name[i]='/';
        }
    }
    int status = hfile_ds_export(node->dataset,export_name);
    if (status < 0) {
        LOG_ERR("error exporting dataset '%s'",export_name);
        return(-1);
    }
    return(0);
}

herr_t unpack_all_files() {
    khiter_t k;
    for (k = kh_begin(filelist); k != kh_end(filelist); ++k) {
        if (kh_exist(filelist, k)) {
            unpack_set_stack(kh_value(filelist,k));
        }
    }
    return(0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,"usage: %s <src1> [src2] [src3] [src4]...\n",argv[0]);
        return(1);
    }

    filelist = kh_init(42);
    n_hdf_src=0;
    int i;
    struct stat hdf_file_stat;
    for (i=1; i<argc;i++) {
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
    unpack_all_files();
    filelist_clean();
    for (i=0; i < n_hdf_src; i++) H5Fclose(hdf_src[i]);
    return(0);
}
