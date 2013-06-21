#include <sys/stat.h>
#include <fcntl.h>
#include "hstack_tree.h"
#include "logger.h"
hstack_tree_hdf5file_t __hstack_tree_hdf5file_initializer = {
    .hdf_id = -1, .next = NULL, .name = { 0 }
};
hstack_tree_t __hstack_tree_initializer = {
    .hdf_rw = NULL, .hdf_ro = NULL, .root = NULL
};
hstack_tree_t * hstack_tree_new() {
    hstack_tree_t * tree = malloc(sizeof(hstack_tree_t));
    *tree=__hstack_tree_initializer;
    tree->root = hdir_new("/");
    return(tree);
}
int hstack_tree_add(hstack_tree_t * tree, const char *hdf5name, int flags) {
    if (tree->hdf_rw != NULL) {
        if ((flags & O_ACCMODE) == O_RDONLY) {
            LOG_ERR("Cannot add ro file '%s' after rw file '%s'",hdf5name,tree->hdf_rw->name);
        } else {
            LOG_ERR("Cannot add rw file '%s' after rw file '%s'",hdf5name,tree->hdf_rw->name);
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
    if ((flags & O_ACCMODE) == O_RDONLY) {
        hdf5file->next = tree->hdf_ro;
        tree->hdf_ro = hdf5file;
        hdir_add_hdf5(tree->root,this_hdf,1);
    } else {
        tree->hdf_rw = hdf5file;
        hdir_add_hdf5(tree->root,this_hdf,0);
    }
    return(1);
}
int hstack_tree_close(hstack_tree_t * tree) {
    hdir_free(tree->root);
    if (tree->hdf_rw != NULL) {
        H5Fclose(tree->hdf_rw->hdf_id);
        free(tree->hdf_rw);
        tree->hdf_rw = NULL;
    }
    while (tree->hdf_ro != NULL) {
        hstack_tree_hdf5file_t * walker = tree->hdf_ro;
        H5Fclose(walker->hdf_id);
        tree->hdf_ro = walker->next;
        free(walker);
    }
    free(tree);
    return(1);
}
