#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <hdf5.h>
#include <errno.h>
#include <fcntl.h>
#include "hfile_ds.h"
#include "logger.h"
#include "hstack_tree.h"

int   n_hdf_src;

hstack_tree_t * tree = NULL;

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
    tree = hstack_tree_new();
    n_hdf_src=0;
    int i;
    for (i=2; i<argc;i++) {
        if (hstack_tree_add(tree,argv[i],O_RDONLY) == 1) n_hdf_src++;
    }
    if (n_hdf_src == 0) {
        LOG_FATAL("no src files could be opened");
        hstack_tree_close(tree);
        return(1);
    }
    if (hstack_tree_add(tree,argv[1],O_RDWR | O_CREAT | O_EXCL) < 0) {
        LOG_FATAL("file '%s' does already exist",argv[1]);
        hstack_tree_close(tree);
        return(1);
    }
    copy_set_stack_data_t css_data;
    css_data.target_file = tree->hdf_rw->hdf_id;
    css_data.compress    = 3;
    hdir_foreach_file(tree->root,HDIRENT_ITERATE_UNORDERED,
            copy_set_stack,&css_data);
    hstack_tree_close(tree);
    return(0);
}
