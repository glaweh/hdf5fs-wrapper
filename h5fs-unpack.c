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
int   verbosity = 1;

hstack_tree_t * tree = NULL;

int unpack_set_stack(const char * parent, hdirent_t * node, void * op_data) {
    char export_name[PATH_MAX];
    if (node->dataset == NULL) return(0);
    if (node->deleted) return(0);
    strcpy(export_name,node->name);
    int i,name_len;
    name_len = strlen(export_name);
    for (i=0; i<name_len; i++) {
        if (export_name[i]=='%') export_name[i]='/';
    }
    if (verbosity > 0) {
        hstack_tree_hdf5file_t * hdf = tree->hdf;
        hid_t ds_loc = node->dataset->loc_id;
        while (hdf!=NULL) {
            if (hdf->hdf_id==ds_loc)
                break;
            hdf=hdf->next;
        }
        if (hdf==NULL) {
            printf("unpacking WTF:%s to %s\n",node->name,export_name);
        } else {
            printf("unpacking %s:%s to %s\n",hdf->name,node->name,export_name);
        }
        fflush(stdout);
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
        if (verbosity > 0) {
            printf("scanning contents of %s ... ",argv[i]);
            fflush(stdout);
        }
        if (hstack_tree_add(tree,argv[i],O_RDONLY) == 1) {
            if (verbosity > 0) {
                printf("ok.\n");
                fflush(stdout);
            }
            n_hdf_src++;
        } else {
            if (verbosity > 0) {
                printf("failed.\n");
                fflush(stdout);
            }
        }
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
