#include "logger.h"
#include "hdir.h"
hdirent_t __hdirent_initializer_file = {
    .type = HDIRENT_FILE, .n_sets = 0, .dataset = NULL, .refcount=1, .name = { 0 }
};
hdirent_t __hdirent_initializer_dir = {
    .type = HDIRENT_DIR, .dir_iterator = -1, .dirents = NULL, .refcount=1, .name = { 0 }
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
hdirent_t * hdir_add_hfile_ds(hdirent_t * parent, hfile_ds_t * hfile_ds) {
    khiter_t k = kh_get(HDIR, parent->dirents, hfile_ds->name);
    hdirent_t * hdirent;
    if (k == kh_end(parent->dirents)) {
        // no dirent, so add new entry
        hdirent = malloc(sizeof(hdirent_t)+strlen(hfile_ds->name));
        if (hdirent == NULL) {
            LOG_ERR("malloc error");
            return(NULL);
        }
        *hdirent = __hdirent_initializer_file;
        strcpy(hdirent->name,hfile_ds->name);
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
    hfile_ds->next = hdirent->dataset;
    hdirent->dataset=hfile_ds;
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
int hdir_free(hdirent_t * dirent) {
    int result = 1;
    if (dirent->type == HDIRENT_DIR) {
        if (dirent->dirents != NULL) {
            khiter_t k;
            for (k = kh_begin(dirent->dirents); k!=kh_end(dirent->dirents); ++k) {
                if (kh_exist(dirent->dirents,k)) {
                    result &= hdir_free(kh_value(dirent->dirents,k));
                }
            }
            kh_destroy(HDIR,dirent->dirents);
            dirent->dirents = NULL;
        }
    } else if (dirent->type == HDIRENT_FILE) {
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

typedef struct __hdir_add_hdf5_cb_data {
    int rdonly;
    hid_t file_id;
    hdirent_t * parent;
} __hdir_add_hdf5_cb_data_t;

static herr_t __hdir_add_hdf5_cb(hid_t loc_id, const char *name, const H5L_info_t *info, void *operator_data)
{
    __hdir_add_hdf5_cb_data_t * cb_data = operator_data;
    H5O_info_t      infobuf;
    herr_t          status = H5Oget_info_by_name (loc_id, name, &infobuf, H5P_DEFAULT);
    if ((status < 0) || (infobuf.type != H5O_TYPE_DATASET)) return(0);
    hfile_ds_t * hfile_ds = hfile_ds_open(cb_data->file_id,name);
    if (hfile_ds != NULL) {
        hdir_add_hfile_ds(cb_data->parent,hfile_ds);
        hfile_ds_close(hfile_ds);
        hfile_ds->rdonly=cb_data->rdonly;
    } else {
        LOG_ERR("error opening file '%s'",name);
        return(0);
    }
    return(0);
}

int hdir_add_hdf5(hdirent_t * parent, hid_t file_id, int rdonly) {
    herr_t status;
    __hdir_add_hdf5_cb_data_t cb_data;
    cb_data.parent = parent;
    cb_data.rdonly = rdonly;
    cb_data.file_id = file_id;
    status = H5Lvisit_by_name(file_id, "/", H5_INDEX_NAME, H5_ITER_INC, __hdir_add_hdf5_cb, &cb_data, H5P_DEFAULT );
    return(status);
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
