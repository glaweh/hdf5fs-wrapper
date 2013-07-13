#include <time.h>
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <errno.h>
#include "logger.h"
#include "hdir.h"
#include "chunksize.h"
hdirent_t __hdirent_initializer_file = {
    .type = HDIRENT_FILE, .n_sets = 0, .dataset = NULL, .ref_name=1, .ref_open=1, .name = { 0 }, .mtime = 0, .ctime = 0, .atime = 0, .chunk_size = 512, .deleted = 0, .parent = NULL
};
hdirent_t __hdirent_initializer_dir = {
    .type = HDIRENT_DIR, .dir_iterator = -1, .dirents = NULL, .ref_name=1, .ref_open=1, .name = { 0 }, .mtime = 0, .ctime = 0, .atime = 0, .chunk_size = 512, .deleted = 0, .parent = NULL
};
hdirent_t * hdir_new(hdirent_t * parent, const char * name) {
    hdirent_t * hdir = malloc(sizeof(hdirent_t)+strlen(name));
    if (hdir == NULL) {
        LOG_ERR("malloc error");
        return(NULL);
    }
    *hdir = __hdirent_initializer_dir;
    strcpy(hdir->name,name);
    hdir->dirents=kh_init(HDIR);
    hdir->parent=parent;
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
        hdirent->parent = parent;
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
        hdirent->deleted = (hfile_ds->length < 0);
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
void __hdirent_unlink_helper(hdirent_t * dirent,hid_t hdf_rw) {
    if ((dirent->ref_open > 0) || (dirent->ref_name > 0)) {
        LOG_INFO("refcount for '%s' (%d/%d) (skip unlink)",dirent->name,dirent->ref_name,dirent->ref_open);
        return;
    }
    if (dirent->deleted == 0) {
        LOG_DBG("not deleted: '%s'",dirent->name);
        return;
    }
    LOG_DBG("deleting dataset '%s'",dirent->name);
    hfile_ds_t * to_kill = dirent->dataset;
    if ((to_kill != NULL) && (to_kill->rdonly == 0)) {
        H5Ldelete(to_kill->loc_id,to_kill->name,H5P_DEFAULT);
        dirent->dataset=to_kill->next;
        free(to_kill);
        to_kill = dirent->dataset;
    }
    if (to_kill != NULL) {
        if (hdf_rw < 0) {
            LOG_WARN("cannot create kill-overlay: no hdf_rw");
        } else {
            LOG_DBG("creating kill-overlay %d:%s",hdf_rw,dirent->name);
            hfile_ds_t * killer = hfile_ds_create(hdf_rw, dirent->name, 0, -1, 0, 0);
            killer->next=to_kill;
            dirent->dataset=killer;
        }
    }
    if (dirent->dataset == NULL) {
        if (dirent->parent!=NULL) {
            LOG_DBG("removing '%s' from parent",dirent->name);
            khiter_t k = kh_get(HDIR, dirent->parent->dirents, dirent->name);
            if (k!=kh_end(dirent->parent->dirents)) {
                kh_del(HDIR, dirent->parent->dirents, k);
            }
        }
        LOG_DBG("freeing dirent memory for '%s'",dirent->name);
        free(dirent);
    }
}
hdirent_t * hdirent_open(hdirent_t * parent, const char * name) {
    hdirent_t * dirent = NULL;
    khiter_t k = kh_get(HDIR, parent->dirents, name);
    if (k == kh_end(parent->dirents)) {
        return(NULL);
    }
    dirent=kh_value(parent->dirents,k);
    dirent->ref_open++;
    if ((dirent->ref_open==1) && (dirent->dataset!=0))
        hfile_ds_reopen(dirent->dataset);
    return(dirent);
}
int hdirent_close(hdirent_t * dirent,hid_t hdf_rw) {
    dirent->ref_open--;
    if (dirent->ref_open>0) return(0);
    if ((dirent->dataset != NULL) && (hfile_ds_close(dirent->dataset) < 0))
        return(-1);
    __hdirent_unlink_helper(dirent,hdf_rw);
    return(0);
}

int hdir_free_all(hdirent_t * dirent,hid_t hdf_rw) {
    int result = 1;
    if (dirent->type == HDIRENT_DIR) {
        if (dirent->dirents != NULL) {
            khiter_t k;
            for (k = kh_begin(dirent->dirents); k!=kh_end(dirent->dirents); ++k) {
                if (kh_exist(dirent->dirents,k)) {
                    result &= hdir_free_all(kh_value(dirent->dirents,k),hdf_rw);
                }
            }
            kh_destroy(HDIR,dirent->dirents);
            dirent->dirents = NULL;
        }
    } else if (dirent->type == HDIRENT_FILE) {
        if (hdf_rw >= 0) {
            if (dirent->deleted) {
                if (dirent->dataset != NULL) {
                    if ((dirent->dataset->rdonly) || (dirent->dataset->next != NULL)) {
                        hfile_ds_t * killer = hfile_ds_create(hdf_rw, dirent->name, 0, -1, 0, 0);
                        killer->next = dirent->dataset;
                        dirent->dataset = killer;
                    } else {
                        hfile_ds_close(dirent->dataset);
                        H5Ldelete(hdf_rw,dirent->name,H5P_DEFAULT);
                    }
                }
            } else if (dirent->dataset == NULL) {
                dirent->dataset = hfile_ds_create(hdf_rw, dirent->name, 0, 1, 0, 0);
            }
        }
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

int hdir_unlink(hdirent_t * parent, const char *name, hid_t hdf_rw) {
    hdirent_t * dirent = NULL;
    khiter_t k = kh_get(HDIR, parent->dirents, name);
    if (k == kh_end(parent->dirents)) {
        errno=ENOENT;
        return(-1);
    }
    dirent=kh_value(parent->dirents,k);
    if (dirent->deleted) {
        errno=ENOENT;
        return(-1);
    }
    dirent->deleted=1;
    dirent->ref_name--;
    __hdirent_unlink_helper(dirent,hdf_rw);
    return(0);
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
#define FSTAT_HELPER(stattype) \
int hdir_f##stattype##_helper(hdirent_t * node, struct stattype * sstat) {\
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

FSTAT_HELPER(stat)
FSTAT_HELPER(stat64)
