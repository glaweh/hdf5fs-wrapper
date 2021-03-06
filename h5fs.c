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
#define _GNU_SOURCE
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <errno.h>
#include "hstack_tree.h"
#include "logger.h"
#include "path_util.h"
#include "env_util.h"
#include "h5fs.h"

struct h5dd {
    hdirent_t * hdirent;
    int         append;
    int         rdonly;
    hsize_t     offset;
};

h5fd_t __h5fd_t_initializer = {
    .hdirent = NULL, .append = 0, .rdonly = 1, .offset = 0, .fd = -1, .stream = NULL
};

h5dd_t __h5dd_t_initializer = {
    .hdirent = NULL, .append = 0, .rdonly = 1, .offset = 0
};

struct stat64 hdf_file_stat64;
struct stat   hdf_file_stat;
hstack_tree_t * tree;
char hdf_file[PATH_MAX] = "./scratch_${OMPI_COMM_WORLD_RANK:%04d:0}.h5";


void __h5fs_filename(const char * name, char * mapped_name) {
    int i = 0;
    while (name[i] != 0) {
        if (name[i] == '/') {
            mapped_name[i] = '%';
        } else {
            mapped_name[i] = name[i];
        }
        i++;
    }
    mapped_name[i] = 0;
}


int init_refcounts(const char * parent, hdirent_t * node, void * op_data) {
    LOG_DBG("'%s', deleted: %d, refcount: %d/%d, length: %"PRId64"",node->name,node->deleted,node->ref_name,node->ref_open,node->dataset->length);
    node->ref_open=0;
    return(0);
}


void __attribute__ ((constructor)) h5fs_init(void) {
    tree=hstack_tree_new();
    if (tree == NULL) {
        LOG_ERR("error creating hstack_tree");
        abort();
    }
    char * env_ptr;
    char hdf_expand1[PATH_MAX];
    char hdf_expand2[PATH_MAX];
    env_ptr=getenv("H5FS_RO");
    if (env_ptr!=NULL) {
        char * src_begin = env_ptr;
        while (src_begin != 0) {
            char * src_end = src_begin;
            while ((*src_end != ':') && (*src_end != 0)) {
                src_end++;
            }
            int we_are_at_end = (*src_end == 0);
            *src_end = 0;
            if (strn_env_expand(src_begin,hdf_expand1,PATH_MAX) < 0) {
                LOG_FATAL("error expanding hdf ro filename '%s'",src_begin);
                abort();
            }
            if (rel2abs(hdf_expand1,hdf_expand2) == NULL) {
                LOG_ERR("error calling rel2abs('%s')",hdf_expand1);
                abort();
            }
            LOG_DBG("expanded hdf ro '%s' to '%s'",src_begin,hdf_expand2);
            if (hstack_tree_add(tree,hdf_expand2,O_RDONLY) < 0) {
                LOG_ERR("error opening '%s'",hdf_expand2);
                abort();
            }
            LOG_INFO("hdf_ro opened: '%s', %d",hdf_expand2,tree->hdf->hdf_id);
            if (we_are_at_end) break;
            *src_end = ':';
            src_begin = src_end+1;
        }
    }
    env_ptr=getenv("H5FS_FILE");
    if (env_ptr != NULL) {
        strncpy(hdf_file,env_ptr,PATH_MAX);
    }
    if (strn_env_expand(hdf_file,hdf_expand1,PATH_MAX) < 0) {
        LOG_FATAL("error expanding hdf filename '%s'",hdf_file);
        abort();
    }
    if (rel2abs(hdf_expand1,hdf_expand2) == NULL) {
        LOG_ERR("error calling rel2abs('%s')",hdf_expand1);
        abort();
    }
    if (hstack_tree_add(tree,hdf_expand2,O_RDWR | O_CREAT) < 0) {
        LOG_ERR("error opening '%s'",hdf_expand2);
        abort();
    }
    LOG_INFO("hdf_file opened: '%s', %d",hdf_expand2,tree->hdf->hdf_id);
    if (stat64(hdf_expand2,&hdf_file_stat64) < 0) {
        LOG_WARN("error calling stat64 on '%s', %s",hdf_expand2,strerror(errno));
    }
    if (stat(hdf_expand2,&hdf_file_stat) < 0) {
        LOG_WARN("error calling stat on '%s', %s",hdf_expand2,strerror(errno));
    }
    LOG_DBG("fixing refcounters");
    hdir_foreach_file(tree->root,HDIRENT_ITERATE_UNORDERED,init_refcounts,NULL);
}


void __attribute__ ((destructor)) h5fs_fini(void) {
    if (tree != NULL) hstack_tree_close(tree);
}


h5fd_t * h5fd_open(const char * name, int flags, mode_t mode) {
    hdirent_t * existing_dirent;
    int set_exists = 0;
    int file_exists = 0;
    h5fd_t * h5fd = NULL;
    int old_errno;

    char mapped_name[PATH_MAX];
    __h5fs_filename(name, mapped_name);

    existing_dirent = hdirent_open(tree->root,mapped_name);
    set_exists = (existing_dirent != NULL);
    if (set_exists) {
        LOG_DBG("set '%s', deleted: %d, length: %ld",existing_dirent->name,existing_dirent->deleted,existing_dirent->dataset->length);
    }
    if ((set_exists) && (existing_dirent->deleted==0))
        file_exists = 1;
    if (file_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            LOG_DBG("file already exists '%s'",mapped_name);
            errno=EEXIST;
            goto errlabel;
        }
    } else if ((flags & O_CREAT) == 0) {
        errno=ENOENT;
        goto errlabel;
    }
    if (set_exists) {
        LOG_DBG("set '%s' exists",mapped_name);
    }
    h5fd = malloc(sizeof(h5fd_t));
    *h5fd = __h5fd_t_initializer;
    h5fd->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    h5fd->rdonly   = ((flags & O_ACCMODE) == O_RDONLY);
    h5fd->offset = 0;
    if (set_exists) {
        h5fd->hdirent = existing_dirent;
        if (existing_dirent->dataset!=NULL) {
            if ((flags & O_TRUNC) || existing_dirent->deleted)
                existing_dirent->dataset->length  = 0;
        }
        existing_dirent->deleted=0;
    } else {
        h5fd->hdirent = hdir_add_dirent(tree->root,mapped_name,NULL);
    }
    return(h5fd);
errlabel:
    old_errno=errno;
    free(h5fd);
    errno=old_errno;
    return(NULL);
}


int h5fd_close(h5fd_t * h5fd) {
    if (h5fd == NULL) {
        LOG_WARN("unknown fh");
        errno = EBADF;
        return(-1);
    }
    if (hdirent_close(h5fd->hdirent,tree->hdf_rw) < 0) {
        LOG_WARN("error closing dataset '%s'",h5fd->hdirent->name);
        errno = EIO;
        return(-1);
    }
    LOG_DBG(" '%s'", h5fd->hdirent->name);
    free(h5fd);
    return(0);
}


int h5fs_unlink(const char * name) {
    int old_errno;
    if (hdir_unlink(tree->root,name,tree->hdf_rw) < 0)
        goto errlabel;
    return(0);
errlabel:
    old_errno=errno;
    errno=old_errno;
    return(-1);
}


#define H5FS_STAT(stattype) \
int h5fs_##stattype(const char * name, struct stattype * sstat) {\
    hdirent_t * dirent = NULL; \
    if (name[0] == 0) { \
        dirent = tree->root; \
    } else { \
        char mapped_name[PATH_MAX]; \
        __h5fs_filename(name, mapped_name); \
        khiter_t k = kh_get(HDIR, tree->root->dirents, mapped_name); \
        if (k != kh_end(tree->root->dirents)) { \
            dirent = kh_value(tree->root->dirents,k); \
            if (dirent->deleted) \
                dirent = NULL; \
        } \
    } \
    if (dirent == NULL) { \
        memset(sstat, 0, sizeof(struct stattype)); \
        return(-1); \
    } \
    *sstat=hdf_file_##stattype;\
    return(hdir_f##stattype##_helper(dirent,sstat));\
}

H5FS_STAT(stat)
H5FS_STAT(stat64)


#define H5FD_FSTAT(stattype) \
int h5fd_f##stattype(h5fd_t * fd, struct stattype * sstat) {\
    *sstat=hdf_file_##stattype;\
    return(hdir_f##stattype##_helper(fd->hdirent,sstat));\
}

H5FD_FSTAT(stat)
H5FD_FSTAT(stat64)


off64_t    h5fd_seek(h5fd_t * h5fd, off64_t offset, int whence) {
    off64_t new_offset;
    switch (whence) {
        case SEEK_SET:
            new_offset=offset;
            break;
        case SEEK_CUR:
            new_offset=h5fd->offset+offset;
            break;
        case SEEK_END:
            new_offset=(h5fd->hdirent->dataset == NULL ? 0 : h5fd->hdirent->dataset->length) + offset;
            break;
        default:
            errno=EINVAL;
            return(-1);
    }
    if (new_offset < 0) {
        errno=EINVAL;
        return(-1);
    }
    h5fd->offset=new_offset;
    return(h5fd->offset);
}


ssize_t h5fd_write(h5fd_t * h5fd, const void * buf, size_t count) {
    if (count == 0)
        return(0);
    if (h5fd->rdonly) {
        LOG_DBG("write on read-only fd '%s'",h5fd->hdirent->name);
        errno = EBADF;
        return(-1);
    }
    if ((h5fd->hdirent->dataset==NULL) || (h5fd->hdirent->dataset->rdonly)) {
        if (tree->hdf_rw < 0) {
            LOG_ERR("write attempt, but no rw hdf '%s'",h5fd->hdirent->name);
            errno = ENOSPC;
            return(-1);
        }
        // TODO: HG had originally
        //   hsize_t initial_size = h5fd->offset+count+1;
        // but HG does not remember the reason why:
        //   * essentially, this meant one additional 0-byte in a file
        //     unless it was truncated or extended by subsequent writes
        //   * in the context of octopus, this caused NaNs and subsequent
        //     failure on restart-writing of the restart files
        hsize_t initial_size = h5fd->offset+count;
        if ((h5fd->hdirent->dataset!=NULL) && (h5fd->hdirent->dataset->length > initial_size))
            initial_size = h5fd->hdirent->dataset->length;
        hfile_ds_t * ds = hfile_ds_create(tree->hdf_rw, h5fd->hdirent->name, 0, initial_size, 0, 0);
        if (ds == NULL) {
            LOG_ERR("error creating dataset '%s'",h5fd->hdirent->name);
            errno = EIO;
            return(-1);
        }
        if ((h5fd->hdirent->dataset!=NULL) && (h5fd->hdirent->dataset->length > 0)) {
            herr_t copy_res = hfile_ds_copy_contents(ds, h5fd->hdirent->dataset, -1);
            if (copy_res <= 0) {
                LOG_ERR("error in COW '%s'",h5fd->hdirent->name);
                hfile_ds_close(ds);
                H5Ldelete(tree->hdf_rw, h5fd->hdirent->name, H5P_DEFAULT);
                errno=EIO;
                return(-1);
            }
            LOG_DBG("COW succeeded '%s'",h5fd->hdirent->name);
        }
        if (h5fd->hdirent->dataset!=NULL) {
            hfile_ds_close(h5fd->hdirent->dataset);
            ds->next=h5fd->hdirent->dataset;
        }
        h5fd->hdirent->dataset=ds;
    }
    if (h5fd->append)
        h5fd->offset=h5fd->hdirent->dataset->length;
    hssize_t bytes_written = hfile_ds_write(h5fd->hdirent->dataset,h5fd->offset,buf,count);
    if (bytes_written >= 0) {
        h5fd->offset+=bytes_written;
        return((ssize_t)bytes_written);
    } else if (bytes_written == -1) {
        errno=EBADF;
        return(-1);
    } else if (bytes_written == -2) {
        errno=EFBIG;
        return(-1);
    } else if (bytes_written == -3) {
        errno=EIO;
        return(-1);
    }
    errno = EINVAL;
    return(-1);
}


ssize_t h5fd_read(h5fd_t * h5fd, void *buf, size_t count) {
    // end of file
    if (h5fd->hdirent->dataset == NULL) {
        LOG_DBG("unitialized dataset '%s'",h5fd->hdirent->name);
        return(0);
    }
    hssize_t bytes_read = hfile_ds_read(h5fd->hdirent->dataset,h5fd->offset,buf,count);
    if (bytes_read >= 0) {
        h5fd->offset+=bytes_read;
        return((ssize_t)bytes_read);
    } else if (bytes_read == -1) {
        errno=EBADF;
        return(-1);
    } else if (bytes_read == -2) {
        errno=EIO;
        return(-1);
    }
    errno = EINVAL;
    return(-1);
}


int      h5fd_feof(h5fd_t * h5fd) {
    if (h5fd->hdirent->dataset == NULL)
        return(1);
    if (h5fd->offset >= h5fd->hdirent->dataset->length)
        return(1);
    return(0);
}


ssize_t h5fd_ftruncate(h5fd_t * h5fd, size_t length) {
    if (h5fd->rdonly) {
        LOG_DBG("ftruncate on read-only fd '%s'",h5fd->hdirent->name);
        errno = EBADF;
        return(-1);
    }
    // perform the COW dance
    if ((h5fd->hdirent->dataset==NULL) || (h5fd->hdirent->dataset->rdonly)) {
        // we need to create a COW copy in the RW overlay
        if (tree->hdf_rw < 0) {
            LOG_ERR("ftruncate attempt, but no rw hdf '%s'",h5fd->hdirent->name);
            errno = ENOSPC;
            return(-1);
        }
        hsize_t initial_size = length;
        hfile_ds_t * ds = hfile_ds_create(tree->hdf_rw, h5fd->hdirent->name, 0, initial_size, 0, 0);
        if (ds == NULL) {
            LOG_ERR("error creating dataset '%s'",h5fd->hdirent->name);
            errno = EIO;
            return(-1);
        }
        if ((h5fd->hdirent->dataset!=NULL) && (h5fd->hdirent->dataset->length > 0)) {
            // copy up to truncated length
            herr_t copy_res = hfile_ds_copy_contents(ds, h5fd->hdirent->dataset, length);
            if (copy_res <= 0) {
                LOG_ERR("error in COW '%s'",h5fd->hdirent->name);
                hfile_ds_close(ds);
                H5Ldelete(tree->hdf_rw, h5fd->hdirent->name, H5P_DEFAULT);
                errno=EIO;
                return(-1);
            }
            LOG_DBG("COW succeeded '%s'",h5fd->hdirent->name);
        }
        // close original (RO) dataset
        if (h5fd->hdirent->dataset!=NULL) {
            hfile_ds_close(h5fd->hdirent->dataset);
            ds->next=h5fd->hdirent->dataset;
        }
        // and make RW COW copy the dataset to represent the present file
        h5fd->hdirent->dataset=ds;
    }
    hssize_t truncate_result = hfile_ds_truncate(h5fd->hdirent->dataset,length);
    if (truncate_result == 0) {
        return(0);
    } else {
        errno=EIO;
        return(-1);
    }
    errno = EINVAL;
    return(-1);
}

int h5fs_mkdir(const char * name, mode_t mode) {
    char mapped_name[PATH_MAX];
    __h5fs_filename(name, mapped_name);
    LOG_DBG("called ""('%s', %4o)"", mapped: '%s'", name, (int)mode, mapped_name);
    hdirent_t * existing_dirent = hdirent_open(tree->root, mapped_name);
    if (existing_dirent != NULL) {
        errno = EEXIST;
        return(-1);
    }
    if (hdir_new(tree->root, mapped_name) == NULL) {
        errno = EACCES;
        return(-1);
    }
    return(0);
}
