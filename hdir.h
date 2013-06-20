#ifndef HDIR_H
#define HDIR_H
#include "hfile_ds.h"
#include "khash.h"
#define HDIRENT_FILE 1
#define HDIRENT_DIR  2
typedef struct hdirent hdirent_t;
KHASH_MAP_INIT_STR(HDIR,struct hdirent *);
typedef struct hdirent {
    int type;
    union {
        khiter_t dir_iterator;
        int n_sets;
    };
    union {
        hfile_ds_t    * dataset;
        khash_t(HDIR) * dirents;
    };
    int refcount;
    char name[1];
} hdirent_t;
hdirent_t * hdir_new(const char * name);
hdirent_t * hdir_add_hfile_ds(hdirent_t * parent, hfile_ds_t * hfile_ds);
hdirent_t * hdir_get_dirent(hdirent_t * parent, const char * name);
int         hdir_free(hdirent_t * root);
int         hdir_add_hdf5(hdirent_t * parent, hid_t hdf5_file, int rdonly);
#endif
