#ifndef FILE_DS_H
#define FILE_DS_H
#include <hdf5.h>

#define H5T_FILE_DS H5T_NATIVE_CHAR
typedef struct file_ds {
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
    struct  file_ds * next;
    hid_t   loc_id;
    char    name[1];
} file_ds_t;

extern const file_ds_t __file_ds_initializer;
extern const hsize_t   __file_ds_maxdims[1];

#define DIM_CHUNKED(length,chunk) ((length) + ((chunk) - ((length) % (chunk))))

herr_t file_ds_close(file_ds_t * info);
file_ds_t * file_ds_create(hid_t loc_id, const char *name, hsize_t chunk_size, hsize_t initial_dim, int deflate);
file_ds_t * file_ds_reopen(file_ds_t * info);
file_ds_t * file_ds_open(hid_t loc_id, const char *name);
file_ds_t * file_ds_copy(hid_t dst_loc_id, file_ds_t * src, hsize_t chunk_size, int deflate);
herr_t file_ds_copy_contents(file_ds_t * dst, file_ds_t *src);
#endif
