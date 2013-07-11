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
    time_t atime;
    time_t mtime;
    time_t ctime;
    hsize_t chunk_size;
    int refcount;
    char name[1];
} hdirent_t;

#define HDIRENT_ITERATE_UNORDERED 0
#define HDIRENT_ITERATE_ALPHA 1
#define HDIRENT_ITERATE_ALPHA_DESC 2
typedef int (*hdirent_iterate_t)(const char *parent, hdirent_t * dirent, void * op_data);

typedef struct stat stat_t;
typedef struct stat64 stat64_t;
hdirent_t * hdir_new(const char * name);
hdirent_t * hdir_add_dirent(hdirent_t * parent, const char * name, hfile_ds_t * hfile_ds);
hdirent_t * hdir_get_dirent(hdirent_t * parent, const char * name);
int         hdir_free(hdirent_t * root,hid_t file_empty_create);
int         hdir_foreach_file(hdirent_t * root,int order,hdirent_iterate_t op, void * op_data);
int         hdir_stat_helper(hdirent_t * node, struct stat * sstat);
int         hdir_stat64_helper(hdirent_t * node, struct stat64 * sstat);
#endif
