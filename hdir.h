#ifndef HDIR_H
#define HDIR_H
#include "hfile_ds.h"
#include "khash.h"
#define HDIRENT_FILE 1
#define HDIRENT_DIR  2
typedef struct hdirent hdirent_t;
KHASH_MAP_INIT_STR(HDIR,struct hdirent *);
struct hdirent {
    int type;
    khiter_t dir_iterator;
    int n_sets;
    hfile_ds_t    * dataset;
    khash_t(HDIR) * dirents;
    time_t atime;
    time_t mtime;
    time_t ctime;
    hsize_t chunk_size;
    int ref_name;
    int ref_open;
    int deleted;
    struct hdirent * parent;
    char name[1];
};

#define HDIRENT_ITERATE_UNORDERED 0
#define HDIRENT_ITERATE_ALPHA 1
#define HDIRENT_ITERATE_ALPHA_DESC 2
typedef int (*hdirent_iterate_t)(const char *parent, hdirent_t * dirent, void * op_data);

typedef struct stat stat_t;
typedef struct stat64 stat64_t;
hdirent_t * hdir_new(hdirent_t * parent, const char * name);
hdirent_t * hdir_add_dirent(hdirent_t * parent, const char * name, hfile_ds_t * hfile_ds);
hdirent_t * hdirent_open(hdirent_t * parent, const char * name);
int         hdirent_close(hdirent_t * dirent, hid_t hdf_rw);
int         hdir_unlink(hdirent_t * parent, const char *name, hid_t hdf_rw);
int         hdir_free_all(hdirent_t * root,hid_t hdf_rw);
int         hdir_foreach_file(hdirent_t * root,int order,hdirent_iterate_t op, void * op_data);
int         hdir_fstat_helper(hdirent_t * node, struct stat * sstat);
int         hdir_fstat64_helper(hdirent_t * node, struct stat64 * sstat);
#endif
