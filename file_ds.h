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
    int     refcount;
    int     rdonly;
    struct  hfile_ds * next;
    hid_t   loc_id;
    char    name[1];
} hfile_ds_t;

extern const hfile_ds_t __hfile_ds_initializer;
extern const hsize_t   __hfile_ds_maxdims[1];

#define DIM_CHUNKED(length,chunk) ((length) + ((chunk) - ((length) % (chunk))))

herr_t hfile_ds_close(hfile_ds_t * info);
hfile_ds_t * hfile_ds_create(hid_t loc_id, const char *name, hsize_t chunk_size, hsize_t initial_dim, hsize_t expected_length, int deflate);
hfile_ds_t * hfile_ds_reopen(hfile_ds_t * info);
hfile_ds_t * hfile_ds_open(hid_t loc_id, const char *name);
hfile_ds_t * hfile_ds_copy(hid_t dst_loc_id, hfile_ds_t * src, hsize_t chunk_size, int deflate);
herr_t       hfile_ds_copy_contents(hfile_ds_t * dst, hfile_ds_t *src);
int          hfile_ds_exists(hid_t loc_id, const char *pathname);
hssize_t     hfile_ds_read(hfile_ds_t * hfile_ds, hsize_t offset, void * buf, hsize_t count);
hssize_t     hfile_ds_write(hfile_ds_t * hfile_ds, hsize_t offset, const void *buf, hsize_t count);
hssize_t     hfile_ds_export(hfile_ds_t * src, const char * filename);
#endif
