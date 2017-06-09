/*
 * Copyright (c) 2013 Henning Glawe <glaweh@debian.org>
 *
 * This file is part of hdf5fs-wrapper.
 *
 * hdf5fs-wrapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hdf5fs-wrapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with hdf5fs-wrapper.  If not, see <http://www.gnu.org/licenses/>.
 */
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <fcntl.h>
#include "hstack_tree.h"
#include "logger.h"
hstack_tree_hdf5file_t __hstack_tree_hdf5file_initializer = {
    .hdf_id = -1, .next = NULL, .rdonly = 1, .name = { 0 }
};
hstack_tree_t __hstack_tree_initializer = {
    .hdf = NULL, .root = NULL, .hdf_rw = -1
};
hstack_tree_t * hstack_tree_new() {
    hstack_tree_t * tree = malloc(sizeof(hstack_tree_t));
    *tree=__hstack_tree_initializer;
    tree->root = hdir_new(NULL,"/");
    return(tree);
}

typedef struct __hstack_tree_add_cb_data {
    hstack_tree_hdf5file_t * hdf;
    hdirent_t * parent;
} __hstack_tree_add_cb_data_t;

void hstack_tree_create_fake_dirs(hdirent_t * parent, const char *name) {
    char fake_path[PATH_MAX];
    int i = 0;
    while (name[i]!=0) {
        if (name[i] == '%') {
            fake_path[i] = 0;
            LOG_DBG2("p: %s", fake_path);
            hdir_new(parent, fake_path);
            fake_path[i] = '%';
        } else {
            fake_path[i] = name[i];
        }
        i++;
    }
}

static herr_t __hstack_tree_add_cb(hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data)
{
    __hstack_tree_add_cb_data_t * cb_data = operator_data;
    H5O_info_t      infobuf;
    herr_t          status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    if ((status < 0) || (infobuf.type != H5O_TYPE_DATASET)) return(0);
    hfile_ds_t * hfile_ds = hfile_ds_open(cb_data->hdf->hdf_id,name);
    if (hfile_ds != NULL) {
        hstack_tree_create_fake_dirs(cb_data->parent,name);
        hdir_add_dirent(cb_data->parent,name,hfile_ds);
        hfile_ds_close(hfile_ds);
        hfile_ds->rdonly=cb_data->hdf->rdonly;
    } else {
        LOG_ERR("error opening file '%s'",name);
        return(0);
    }
    return(0);
}

int hstack_tree_add(hstack_tree_t * tree, const char *hdf5name, int flags) {
    if ((tree->hdf != NULL) && (tree->hdf->rdonly == 0)) {
        if ((flags & O_ACCMODE) == O_RDONLY) {
            LOG_ERR("Cannot add ro file '%s' after rw file '%s'",hdf5name,tree->hdf->name);
        } else {
            LOG_ERR("Cannot add rw file '%s' after rw file '%s'",hdf5name,tree->hdf->name);
        }
        return(-1);
    }
    struct stat hdf_file_stat;
    hid_t this_hdf=-1;
    if (stat(hdf5name,&hdf_file_stat) == 0) {
        // file exists
        if ((flags & O_EXCL) != 0) {
            LOG_ERR("File '%s' exists and O_EXCL was set",hdf5name);
            return(-1);
        }
        if ((flags & O_ACCMODE) == O_RDONLY) {
            this_hdf = H5Fopen(hdf5name,H5F_ACC_RDONLY,H5P_DEFAULT);
        } else if (flags & O_TRUNC) {
            this_hdf = H5Fcreate(hdf5name,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        } else {
            this_hdf = H5Fopen(hdf5name,H5F_ACC_RDWR,H5P_DEFAULT);
        }
    } else {
        if (flags & O_CREAT) {
            this_hdf = H5Fcreate(hdf5name,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
        } else {
            LOG_ERR("file '%s' does not exist",hdf5name);
            return(-1);
        }
    }
    if ( this_hdf < 0) {
        LOG_ERR("error opening hdf file '%s', %d",hdf5name, this_hdf);
        return(-1);
    }
    hstack_tree_hdf5file_t * hdf5file = malloc(sizeof(hstack_tree_hdf5file_t) + strlen(hdf5name));
    if (hdf5file == NULL) {
        LOG_ERR("malloc error");
        H5Fclose(this_hdf);
        return(-1);
    }
    *hdf5file = __hstack_tree_hdf5file_initializer;
    strcpy(hdf5file->name,hdf5name);
    hdf5file->hdf_id = this_hdf;
    hdf5file->rdonly = ((flags & O_ACCMODE) == O_RDONLY);
    hdf5file->next   = tree->hdf;
    tree->hdf        = hdf5file;
    if ((flags & O_ACCMODE) != O_RDONLY) {
        tree->hdf_rw = this_hdf;
    }
    __hstack_tree_add_cb_data_t cb_data;
    cb_data.parent = tree->root;
    cb_data.hdf    = hdf5file;
    // scan the datasets
    herr_t status = H5Lvisit_by_name(this_hdf, "/", H5_INDEX_NAME, H5_ITER_INC, __hstack_tree_add_cb, &cb_data, H5P_DEFAULT );
    if (status >= 0) {
        return(1);
    }
    return(0);
}
int hstack_tree_close(hstack_tree_t * tree) {
    hdir_free_all(tree->root,tree->hdf_rw);
    while (tree->hdf != NULL) {
        hstack_tree_hdf5file_t * walker = tree->hdf;
        H5Fclose(walker->hdf_id);
        tree->hdf = walker->next;
        free(walker);
    }
    free(tree);
    return(1);
}
