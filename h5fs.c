#define _GNU_SOURCE
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <errno.h>
#include "hstack_tree.h"
#include "logger.h"
#include "path_util.h"
#include "env_util.h"
#include "h5fs.h"
typedef struct h5fd {
    hdirent_t * hdirent;
    int         append;
    int         rdonly;
    hsize_t     offset;
} h5fd_t;

typedef struct h5dd {
    hdirent_t * hdirent;
    int         append;
    int         rdonly;
    hsize_t     offset;
} h5dd_t;

h5fd_t __h5fd_t_initializer = {
    .hdirent = NULL, .append = 0, .rdonly = 1, .offset = 0
};

h5dd_t __h5dd_t_initializer = {
    .hdirent = NULL, .append = 0, .rdonly = 1, .offset = 0
};

struct stat64 hdf_file_stat;
hstack_tree_t * tree;
char hdf_file[PATH_MAX] = "./scratch${OMPI_COMM_WORLD_RANK:%04d:0}.h5";

int init_refcounts(const char * parent, hdirent_t * node, void * op_data) {
    LOG_DBG2("'%s' had ref_open %d",node->name,node->ref_open);
    node->ref_open=0;
    return(0);
}

void __attribute__ ((constructor)) h5fs_init(void) {
    tree=hstack_tree_new();
    if (tree == NULL) {
        LOG_ERR("error creating hstack_tree");
        exit(1);
    }
    char * env_ptr;
    char hdf_expand1[PATH_MAX];
    char hdf_expand2[PATH_MAX];
    env_ptr=getenv("HDF5FS_RO");
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
                exit(1);
            }
            if (rel2abs(hdf_expand1,hdf_expand2) == NULL) {
                LOG_ERR("error calling rel2abs('%s')",hdf_expand1);
                exit(1);
            }
            LOG_DBG("expanded hdf ro '%s' to '%s'",src_begin,hdf_expand2);
            if (hstack_tree_add(tree,hdf_expand2,O_RDONLY) < 0) {
                LOG_ERR("error opening '%s'",hdf_expand2);
                exit(1);
            }
            LOG_INFO("hdf_ro opened: '%s', %d",hdf_expand2,tree->hdf->hdf_id);
            if (we_are_at_end) break;
            *src_end = ':';
            src_begin = src_end+1;
        }
    }
    env_ptr=getenv("HDF5FS_FILE");
    if (env_ptr != NULL) {
        strncpy(hdf_file,env_ptr,PATH_MAX);
    }
    if (strn_env_expand(hdf_file,hdf_expand1,PATH_MAX) < 0) {
        LOG_FATAL("error expanding hdf filename '%s'",hdf_file);
        exit(1);
    }
    if (rel2abs(hdf_expand1,hdf_expand2) == NULL) {
        LOG_ERR("error calling rel2abs('%s')",hdf_expand1);
        exit(1);
    }
    if (hstack_tree_add(tree,hdf_expand2,O_RDWR | O_CREAT) < 0) {
        LOG_ERR("error opening '%s'",hdf_expand2);
        exit(1);
    }
    LOG_INFO("hdf_file opened: '%s', %d",hdf_expand2,tree->hdf->hdf_id);
    if (stat64(hdf_expand2,&hdf_file_stat) < 0) {
        LOG_WARN("error calling stat64 on '%s', %s",hdf_expand2,strerror(errno));
    }
    LOG_INFO("fixing refcounters");
    hdir_foreach_file(tree->root,HDIRENT_ITERATE_UNORDERED,init_refcounts,NULL);
}

void __attribute__ ((destructor)) h5fs_fini(void) {
    if (tree != NULL) hstack_tree_close(tree);
}
inline char * __h5fs_filename(const char * name) {
    char * newname = strdup(name);
    if (newname==NULL) return(NULL);
    char * iterator = newname;
    while (*iterator!=0) {
        if (*iterator=='/') *iterator='%';
        iterator++;
    }
    return(newname);
}
h5fd_t * h5fd_open(const char * name, int flags, mode_t mode) {
    char * h5fs_filename = NULL;
    hdirent_t * existing_dirent;
    int set_exists = 0;
    int file_exists = 0;
    h5fd_t * h5fd = NULL;
    int old_errno;
    h5fs_filename = __h5fs_filename(name);
    if (h5fs_filename==NULL) {
        LOG_FATAL("error mapping filename '%s'",name);
        errno=ENAMETOOLONG;
        return(NULL);
    }
    existing_dirent = hdir_open_dirent(tree->root,h5fs_filename);
    set_exists = (existing_dirent != NULL);
    if ((set_exists) && (existing_dirent->deleted==0))
        file_exists = 1;
    if (file_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            LOG_DBG("file already exists '%s'",h5fs_filename);
            errno=EEXIST;
            goto errlabel;
        }
    } else if ((flags & O_CREAT) == 0) {
        errno=ENOENT;
        goto errlabel;
    }
    if (set_exists) {
        LOG_DBG("set '%s' exists",h5fs_filename);
    }
    h5fd = malloc(sizeof(h5fd_t));
    *h5fd = __h5fd_t_initializer;
    if (set_exists) {
        h5fd->hdirent = existing_dirent;
        if ((existing_dirent->dataset!=NULL) && (flags & O_TRUNC)) {
            existing_dirent->dataset->length  = 0;
        }
    } else {
        h5fd->hdirent = hdir_add_dirent(tree->root,h5fs_filename,NULL);
    }
    h5fd->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    h5fd->rdonly   = ((flags & O_ACCMODE) == O_RDONLY);
    h5fd->offset = 0;
    return(h5fd);
errlabel:
    old_errno=errno;
    free(h5fd);
    free(h5fs_filename);
    errno=old_errno;
    return(NULL);
}
int h5fd_close(h5fd_t * h5fd) {
    if (h5fd == NULL) {
        LOG_WARN("unknown fh");
        errno = EBADF;
        return(-1);
    }
    if (hdir_close_dirent(h5fd->hdirent) < 1) {
        LOG_WARN("error closing dataset '%s'",h5fd->hdirent->name);
        errno = EIO;
        return(-1);
    }
    LOG_DBG(" '%s'", h5fd->hdirent->name);
    free(h5fd);
    return(1);
}
int h5fs_unlink(const char * name) {
    char * h5fs_filename = NULL;
    h5fs_filename = __h5fs_filename(name);
    int old_errno;
    if (h5fs_filename==NULL) {
        LOG_FATAL("error mapping filename '%s'",name);
        errno=ENAMETOOLONG;
        return(-1);
    }
    if (hdir_unlink(tree->root,h5fs_filename) < 0)
        goto errlabel;
    free(h5fs_filename);
    return(1);
errlabel:
    old_errno=errno;
    free(h5fs_filename);
    errno=old_errno;
    return(-1);
}
