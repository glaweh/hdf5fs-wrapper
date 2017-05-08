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
#include <openssl/md5.h>

int   n_hdf_src;

#define BUFSIZE (1024*1024)
hstack_tree_t * tree = NULL;
char * buf = NULL;

int check_set_stack(const char * parent, hdirent_t * node, void * op_data) {
    char export_name[PATH_MAX];
    if (node->dataset == NULL) return(0);
    if (node->deleted) return(0);
    strcpy(export_name,node->name);
    int i,name_len;
    name_len = strlen(export_name);
    for (i=0; i<name_len; i++) {
        if (export_name[i]=='%') export_name[i]='/';
    }
    hfile_ds_reopen(node->dataset);
    ssize_t offset = 0;
    ssize_t bytes  = 0;
    MD5_CTX c;
    MD5_Init(&c);
    bytes = hfile_ds_read(node->dataset,offset,buf,BUFSIZE);
    while (bytes > 0) {
        MD5_Update(&c, buf, bytes);
        offset+=bytes;
        bytes = hfile_ds_read(node->dataset,offset,buf,BUFSIZE);
    }
    unsigned char out[MD5_DIGEST_LENGTH];
    MD5_Final(out, &c);
    char md5str[2*MD5_DIGEST_LENGTH];
    int n;
    for(n=0; n<MD5_DIGEST_LENGTH; n++)
        sprintf((md5str+2*n),"%02x", out[n]);
    printf("%-40s %32s %20"PRIi64"\n",export_name,md5str,node->dataset->length);
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
    buf = malloc(BUFSIZE);
    hdir_foreach_file(tree->root,HDIRENT_ITERATE_UNORDERED,
            check_set_stack,NULL);
    free(buf);
    hstack_tree_close(tree);
    return(0);
}
