#include <sys/types.h>
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <hdf5.h>
#include "hdf5_fs.h"
#include "string_set.h"
#include "wrapper_limits.h"
#include "logger.h"
#include "hstack_tree.h"

struct stat64 hdf_file_stat;
hstack_tree_t * tree;

string_set * closed_empty_files;

typedef struct {
    hdirent_t * hdirent;
    int     append;
    int     rdonly;
    hsize_t offset;
} hdf5_data_t;
hdf5_data_t __hdf5_data_t_initializer = {
    .hdirent = NULL, .append = 0, .rdonly = 1, .offset = 0
};

int     last_handle=-1;
hdf5_data_t * hdf5_data[HANDLES_MAX];

int hdf5_fs_init(const char * hdf_filename) {
    tree=hstack_tree_new();
    if (tree == NULL) {
        LOG_ERR("error creating hstack_tree for '%s'",hdf_filename);
        return(-1);
    }
    if (hstack_tree_add(tree,hdf_filename,O_RDWR | O_CREAT) < 0) {
        LOG_ERR("error opening '%s'",hdf_filename);
        return(-1);
    }
    LOG_INFO("hdf_file opened: '%s', %d",hdf_filename,tree->hdf->hdf_id);
    int i;
    for (i=0;i<HANDLES_MAX;i++)
        hdf5_data[i]=NULL;
    last_handle = -1;
    if (stat64(hdf_filename,&hdf_file_stat) < 0) {
        LOG_WARN("error calling stat64 on '%s', %s",hdf_filename,strerror(errno));
    }
    closed_empty_files=string_set_new();
    return(1);
}

int hdf5_fs_fini() {
    int i;
    for (i=0;i<=last_handle;i++) {
        if (hdf5_data[i] != NULL)
            hdf5_close(i);
    }
    string_set_dump(closed_empty_files);
    string_set_free(closed_empty_files);
    hstack_tree_close(tree);
    return(1);
}

int hdf5_open(int fd, const char *pathname, int flags) {
    if (hdf5_data[fd] != NULL) {
        LOG_WARN("fd already in use");
        return(-1);
    }
    LOG_DBG(" %d, '%s', %o", fd,pathname,flags);
    hdirent_t * existing_dirent = hdir_get_dirent(tree->root,pathname);
    int set_exists = (existing_dirent != NULL);
    if (set_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            LOG_DBG("dataset already exists %d -> '%s'",fd,pathname);
            errno=EEXIST;
            return(-1);
        }
        LOG_DBG("set '%s' exists",pathname);
        if (existing_dirent->dataset != NULL) {
            hfile_ds_reopen(existing_dirent->dataset);
        }
    } else if ((flags & O_CREAT) == 0) {
        errno=ENOENT;
        return(-1);
    }
    hdf5_data[fd] = malloc(sizeof(hdf5_data_t));
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("error allocating hdf5_data[%d]",fd);
        errno = ENOMEM;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    *d = __hdf5_data_t_initializer;
    if (set_exists) {
        d->hdirent = existing_dirent;
        if ((existing_dirent->dataset!=NULL) && (flags & O_TRUNC)) {
            existing_dirent->dataset->length  = 0;
        }
    } else {
        d->hdirent = hdir_add_dirent(tree->root,pathname,NULL);
    }
    d->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    d->rdonly   = ((flags & O_ACCMODE) == O_RDONLY);
    d->offset = 0;
    if (fd > last_handle) last_handle = fd;
    return(fd);
}

int hdf5_close(int fd) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    if (d->hdirent->dataset != NULL) {
        if (! hfile_ds_close(d->hdirent->dataset)) {
            LOG_WARN("error closing dataset '%s'",d->hdirent->name);
            errno = EIO;
            return(-1);
        }
    } else {
        string_set_add(closed_empty_files, d->hdirent->name, NULL);
    }
    LOG_DBG(" %d, '%s", fd,d->hdirent->name);
    free(hdf5_data[fd]);
    hdf5_data[fd]=NULL;
    while ((last_handle >= 0) && (hdf5_data[last_handle] == NULL)) {
        last_handle--;
    }
    return(1);
}

int hdf5_write(int fd, const void *buf, size_t count) {
    if (count == 0)
        return(0);
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("write on unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    if (d->rdonly) {
        LOG_DBG("write on read-only fd '%s'",d->hdirent->name);
        errno = EBADF;
        return(-1);
    }
    hfile_ds_t * ds = d->hdirent->dataset;
    if (ds == NULL) {
        ds = hfile_ds_create(tree->hdf->hdf_id, d->hdirent->name, 0, count+1, 0, 0);
        if (ds == NULL) {
            LOG_ERR("error creating dataset '%s'",d->hdirent->name);
            errno = EIO;
            return(-1);
        }
        hdir_add_dirent(tree->root,d->hdirent->name,ds);
    } else {
        if (d->append) d->offset=d->hdirent->dataset->length;
    }
    hsize_t bytes_written = hfile_ds_write(ds,d->offset,buf,count);
    if (bytes_written >= 0) {
        d->offset+=bytes_written;
        return((int)bytes_written);
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

int hdf5_read(int fd, void *buf, size_t count) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("write on unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    hfile_ds_t * ds = d->hdirent->dataset;
    // end of file
    if (ds == NULL) {
        LOG_DBG("unitialized dataset '%s'",d->hdirent->name);
        return(0);
    }
    hsize_t bytes_read = hfile_ds_read(ds,d->offset,buf,count);
    if (bytes_read >= 0) {
        d->offset+=bytes_read;
        return((int)bytes_read);
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

int hdf5_lseek(int fd, off_t offset, int whence) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    switch (whence) {
        case SEEK_SET:
            d->offset=offset;
            break;
        case SEEK_END:
            d->offset=(d->hdirent->dataset == NULL ? offset : d->hdirent->dataset->length+offset);
            break;
        case SEEK_CUR:
            d->offset+=offset;
            break;
        default:
            errno=EINVAL;
            LOG_WARN("illegal whence: %d",whence);
            return(-1);
            break;
    }
    LOG_DBG("(%d='%s',%d,%d) = %d",fd,d->hdirent->name,(int)offset,whence,(int)d->offset);
    return(d->offset);
}

int hdf5_stat64(const char *pathname, struct stat64 *buf) {
    hdirent_t * hdirent = hdir_get_dirent(tree->root, pathname);
    if (hdirent == NULL) {
        errno=ENOENT;
        return(-1);
    }
    (*buf)=hdf_file_stat;
    hdir_stat64_helper(hdirent,buf);
    LOG_DBG("size of '%s': %d",pathname,(int)buf->st_size);
    return(0);
}

int hdf5_fstat64(int fd, struct stat64 *buf) {
    if (hdf5_data[fd] == NULL) {
        LOG_WARN("unknown fd %d",fd);
        errno = EBADF;
        return(-1);
    }
    hdirent_t * hdirent = hdf5_data[fd]->hdirent;
    (*buf)=hdf_file_stat;
    hdir_stat64_helper(hdirent,buf);
    LOG_DBG("size of '%s': %d",d->hdirent->name,(int)buf->st_size);
    return(0);
}

int hdf5_unlink(const char *pathname) {
    if (hfile_ds_exists(tree->hdf->hdf_id,pathname)==0) {
        if (string_set_remove(closed_empty_files,pathname))
            return(0);
        errno=ENOENT;
        return(-1);
    }
    herr_t res = H5Ldelete(tree->hdf->hdf_id,pathname,H5P_DEFAULT);
    if (res < 0) {
        errno=EIO;
        return(-1);
    }
    return(0);
}
