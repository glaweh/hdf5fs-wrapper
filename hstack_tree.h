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
    hid_t hdf_rw;
} hstack_tree_t;
hstack_tree_t * hstack_tree_new();
int hstack_tree_add(hstack_tree_t * tree, const char *hdf5name, int flags);
int hstack_tree_close(hstack_tree_t * tree);
#endif
