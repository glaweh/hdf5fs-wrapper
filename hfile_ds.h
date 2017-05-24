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
#ifndef HFILE_DS_H
#define HFILE_DS_H
#include <hdf5.h>

#define H5T_HFILE_DS H5T_NATIVE_CHAR
typedef struct hfile_ds {
    hid_t   space;
    hid_t   set;
    hid_t   length_space;
    hid_t   length_attrib;
    hsize_t dims[1];
    hsize_t chunk[1];
    int64_t length;
    int64_t length_original;
    int     rdonly;
    struct  hfile_ds * next;
    time_t  atime;
    time_t  mtime;
    time_t  ctime;
    hid_t   loc_id;
    char    name[1];
} hfile_ds_t;

extern const hfile_ds_t __hfile_ds_initializer;
extern const hsize_t   __hfile_ds_maxdims[1];

#define DIM_CHUNKED(length,chunk) ((length) + ((chunk) - ((length) % (chunk))))

herr_t hfile_ds_close(hfile_ds_t * info);
hfile_ds_t * hfile_ds_create(hid_t loc_id, const char *name, hsize_t chunk_size, hssize_t initial_length, hsize_t expected_length, int deflate);
hfile_ds_t * hfile_ds_reopen(hfile_ds_t * info);
hfile_ds_t * hfile_ds_open(hid_t loc_id, const char *name);
hfile_ds_t * hfile_ds_copy(hid_t dst_loc_id, hfile_ds_t * src, hssize_t copy_length, hsize_t chunk_size, int deflate);
herr_t       hfile_ds_copy_contents(hfile_ds_t * dst, hfile_ds_t *src, hssize_t copy_length);
int          hfile_ds_exists(hid_t loc_id, const char *pathname);
hssize_t     hfile_ds_read(hfile_ds_t * hfile_ds, hsize_t offset, void * buf, hsize_t count);
hssize_t     hfile_ds_write(hfile_ds_t * hfile_ds, hsize_t offset, const void *buf, hsize_t count);
hssize_t     hfile_ds_export(hfile_ds_t * src, const char * filename);
void         hfile_ds_update_timestamps(hfile_ds_t * ds);
#endif
