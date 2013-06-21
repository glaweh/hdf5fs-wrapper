#ifndef HSTACK_TREE_H
#define HSTACK_TREE_H
#include "hdir.h"
typedef struct hstack_tree_hdf5file {
    hid_t hdf_id;
    int   rdonly;
    struct hstack_tree_hdf5file * next;
    char name[1];
} hstack_tree_hdf5file_t;
typedef struct hstack_tree {
    hstack_tree_hdf5file_t * hdf;
    hdirent_t * root;
} hstack_tree_t;
hstack_tree_t * hstack_tree_new();
int hstack_tree_add(hstack_tree_t * tree, const char *hdf5name, int flags);
int hstack_tree_close(hstack_tree_t * tree);
#endif
