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
    tree = hstack_tree_new();
    n_hdf_src=0;
    int i;
    for (i=1; i<argc;i++) {
        if (hstack_tree_add(tree,argv[i],O_RDONLY) == 1) n_hdf_src++;
    }
    if (n_hdf_src == 0) {
        LOG_FATAL("no src files could be opened");
        hstack_tree_close(tree);
        return(1);
    }
    hdir_foreach_file(tree->root,HDIRENT_ITERATE_UNORDERED,
            unpack_set_stack,NULL);
    hstack_tree_close(tree);
    return(0);
}
