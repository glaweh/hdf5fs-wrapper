#include <time.h>
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include "logger.h"
#include "hdir.h"
#include "chunksize.h"
hdirent_t __hdirent_initializer_file = {
    .type = HDIRENT_FILE, .n_sets = 0, .dataset = NULL, .refcount=1, .name = { 0 }, .mtime = 0, .ctime = 0, .atime = 0, .chunk_size = 512, .deleted = 0
};
hdirent_t __hdirent_initializer_dir = {
    .type = HDIRENT_DIR, .dir_iterator = -1, .dirents = NULL, .refcount=1, .name = { 0 }, .mtime = 0, .ctime = 0, .atime = 0, .chunk_size = 512, .deleted = 0
};
hdirent_t * hdir_new(const char * name) {
    hdirent_t * hdir = malloc(sizeof(hdirent_t)+strlen(name));
    if (hdir == NULL) {
        LOG_ERR("malloc error");
        return(NULL);
    }
    *hdir = __hdirent_initializer_dir;
    strcpy(hdir->name,name);
    hdir->dirents=kh_init(HDIR);
    return(hdir);
}
hdirent_t * hdir_add_dirent(hdirent_t * parent, const char *name, hfile_ds_t * hfile_ds) {
    khiter_t k = kh_get(HDIR, parent->dirents, name);
    hdirent_t * hdirent;
    if (k == kh_end(parent->dirents)) {
        // no dirent, so add new entry
        hdirent = malloc(sizeof(hdirent_t)+strlen(name));
        if (hdirent == NULL) {
            LOG_ERR("malloc error");
            return(NULL);
        }
        *hdirent = __hdirent_initializer_file;
        strcpy(hdirent->name,name);
        int ret;
        k = kh_put(HDIR,parent->dirents,hdirent->name,&ret);
        if (! ret) {
            LOG_ERR("error adding key to hash, %d",ret);
            free(hdirent);
            return(NULL);
        }
        kh_value(parent->dirents,k)=hdirent;
    } else {
        hdirent=kh_value(parent->dirents,k);
    }
    if (hfile_ds!=NULL) {
        hfile_ds->next   = hdirent->dataset;
        hdirent->dataset = hfile_ds;
        hdirent->atime   = hfile_ds->atime;
        hdirent->ctime   = hfile_ds->ctime;
        hdirent->mtime   = hfile_ds->mtime;
        hdirent->chunk_size = hfile_ds->chunk[0];
    } else {
        time_t now = time(NULL);
        hdirent->atime = now;
        hdirent->ctime = now;
        hdirent->mtime = now;
        hdirent->chunk_size = chunksize_suggest(name,0);
    }
    hdirent->n_sets++;
    return(hdirent);
}
hdirent_t * hdir_get_dirent(hdirent_t * parent, const char * name) {
    khiter_t k = kh_get(HDIR, parent->dirents, name);
    if (k == kh_end(parent->dirents)) {
        return(NULL);
    }
    return(kh_value(parent->dirents,k));
}
int hdir_free(hdirent_t * dirent,hid_t hdf_rw) {
    int result = 1;
    if (dirent->type == HDIRENT_DIR) {
        if (dirent->dirents != NULL) {
            khiter_t k;
            for (k = kh_begin(dirent->dirents); k!=kh_end(dirent->dirents); ++k) {
                if (kh_exist(dirent->dirents,k)) {
                    result &= hdir_free(kh_value(dirent->dirents,k),hdf_rw);
                }
            }
            kh_destroy(HDIR,dirent->dirents);
            dirent->dirents = NULL;
        }
    } else if (dirent->type == HDIRENT_FILE) {
        if ((dirent->dataset == NULL) && (hdf_rw >=0))
            dirent->dataset = hfile_ds_create(hdf_rw, dirent->name, 0, 1, 0, 0);
        while (dirent->dataset != NULL) {
            hfile_ds_t * walker = dirent->dataset;
            dirent->dataset=dirent->dataset->next;
            result &= hfile_ds_close(walker);
            free(walker);
        }
    } else {
        LOG_ERR("unknown dirent type %d in dirent '%s'",dirent->type,dirent->name);
        result=0;
    }
    free(dirent);
    return(result);
}

int hdir_foreach_file(hdirent_t * root, int order, hdirent_iterate_t op, void * op_data) {
    khiter_t k;
    if ((root->type != HDIRENT_DIR) || (root->dirents == NULL)) {
        LOG_ERR("hdir_foreach_file need dir dirent argument");
        return(0);
    }
    hdirent_t * dirent;
    if (order!=HDIRENT_ITERATE_UNORDERED) {
        LOG_WARN("falling back to unordered iteration");
    }
    int res = 0;
    for (k = kh_begin(root->dirents); k!=kh_end(root->dirents); ++k) {
        if (!kh_exist(root->dirents,k)) continue;
        dirent = kh_value(root->dirents,k);
        if (dirent->type == HDIRENT_FILE) {
            res = op(root->name,dirent,op_data);
        } else if (dirent->type == HDIRENT_DIR) {
            LOG_INFO("no recursion implemented");
            res = 0;
        }
        if (res != 0) break;
    }
    return(res);
}
#define STAT_HELPER(stattype) \
int hdir_##stattype##_helper(hdirent_t * node, struct stattype * sstat) {\
    if (node->type == HDIRENT_FILE) {\
        sstat->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;\
        if (node->dataset == NULL) {\
            sstat->st_size = 0;\
            sstat->st_blocks = 0;\
        } else {\
            hfile_ds_update_timestamps(node->dataset);\
            node->atime=node->dataset->atime;\
            node->ctime=node->dataset->ctime;\
            node->mtime=node->dataset->mtime;\
            sstat->st_size = node->dataset->length;\
            sstat->st_blocks = node->dataset->dims[0] / 512;\
        }\
    } else {\
        sstat->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;\
        sstat->st_size = 0;\
        sstat->st_blocks = 0;\
    }\
    sstat->st_atime=node->atime;\
    sstat->st_ctime=node->ctime;\
    sstat->st_mtime=node->mtime;\
    sstat->st_blksize=node->chunk_size;\
    return(0);\
}

STAT_HELPER(stat)
STAT_HELPER(stat64)
