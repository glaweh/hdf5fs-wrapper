#include <sys/types.h>
#define __USE_LARGEFILE64
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <hdf5.h>
#include <hdf5_hl.h>
#include "hdf5_fs.h"
#include "wrapper_limits.h"

#define RANK 1
hid_t   hdf_file;
hid_t   create_params;
char    fillvalue = 0;
hsize_t chunk_dims[1]={1024*64};
hsize_t maxdims[1] = {H5S_UNLIMITED};
struct stat64 hdf_file_stat;
#define CLOSED_EMPTY_MAX 1024*64
int  n_closed_empty_files=0;
char * closed_empty_files[CLOSED_EMPTY_MAX];

typedef struct {
    hid_t   space;
    hid_t   set;
    int     append;
    int     truncate;
    hsize_t dims[RANK];
    int length;
    hsize_t offset[RANK];
    char  name[PATH_MAX];
} hdf5_data_t;

int     last_handle=-1;
hdf5_data_t * hdf5_data[HANDLES_MAX];

int _hdf5_path_exists(const char *pathname);
int _closed_empty_add(const char *pathname);
int _closed_empty_remove(const char *pathname);
int _closed_empty_find(const char *pathname);
int _closed_empty_free();
void _closed_empty_dump();

int hdf5_fs_init(const char * hdf_filename) {
    struct stat hdf_stat;
    herr_t status;
    if ((stat(hdf_filename,&hdf_stat)>=0) && (H5Fis_hdf5(hdf_filename) >= 0)) {
        hdf_file = H5Fopen(hdf_filename,H5F_ACC_RDWR,H5P_DEFAULT);
    } else {
        hdf_file = H5Fcreate(hdf_filename,H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    }
#ifdef DEBUG
    fprintf(stderr,"hdf_file opened: '%s', %d\n",hdf_filename,hdf_file);
#endif
    if (hdf_file < 0) {
        fprintf(stderr,"hdf_file error\n");
        return(-1);
    }
    create_params = H5Pcreate(H5P_DATASET_CREATE);
    status        = H5Pset_chunk(create_params, 1, chunk_dims);
    if (status < 0) {
        fprintf(stderr,"error setting um chunking\n");
        return(-1);
    }
    int i;
    for (i=0;i<HANDLES_MAX;i++)
        hdf5_data[i]=NULL;
    last_handle = -1;
    if (stat64(hdf_filename,&hdf_file_stat) < 0) {
        fprintf(stderr,"error calling stat64 on '%s', %s\n",hdf_filename,strerror(errno));
    }
    return(1);
}

int hdf5_fs_fini() {
    int i;
    for (i=0;i<=last_handle;i++) {
        if (hdf5_data[i] != NULL)
            hdf5_close(i);
    }
    _closed_empty_dump();
    _closed_empty_free();
    H5Fclose(hdf_file);
    return(1);
}

int hdf5_open(int fd, const char *pathname, int flags) {
    if (hdf5_data[fd] != NULL) {
        fprintf(stderr,"fd already in use\n");
        return(-1);
    }
    fprintf(stderr,"hdf5_open(%d,'%s',%o)\n",fd,pathname,flags);
    int set_exists = _hdf5_path_exists(pathname);

    hid_t  set=-1;
    if (set_exists) {
        if (((flags & O_CREAT)>0) && ((flags & O_EXCL)>0)) {
            fprintf(stderr,"dataset already exists %d -> '%s'\n",fd,pathname);
            errno=EEXIST;
            return(-1);
        }
        set = H5Dopen(hdf_file,pathname,H5P_DEFAULT);
    }
    hdf5_data[fd] = malloc(sizeof(hdf5_data_t));
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"error allocating hdf5_data[%d]\n",fd);
        errno = ENOMEM;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    strncpy(d->name,pathname,PATH_MAX);
    d->append   = ((flags & O_APPEND) != 0 ? 1 : 0);
    d->set      = set;
    d->space    = -1;
    if (d->set < 0) {
        if (flags & O_CREAT) {
            d->length  = 0;
            d->offset[0] = 0;
            d->dims[0] = 1;
            return(fd);
        } else {
            fprintf(stderr,"error opening dataset '%s' %d\n",hdf5_data[fd]->name,hdf5_data[fd]->set);
            free(hdf5_data[fd]);
            hdf5_data[fd]=NULL;
            errno=ENOENT;
            return(-1);
        }
    } else {
        d->space = H5Dget_space(set);
        H5Sget_simple_extent_dims(d->space, d->dims, NULL);
        if (flags & O_TRUNC) {
            d->length  = 0;
            d->offset[0] = 0;
        } else {
            H5LTget_attribute_int(hdf_file,pathname,"length",&d->length);
            d->offset[0] = 0;
        }
    }
    if (fd > last_handle) last_handle = fd;
    return(fd);
}

int hdf5_close(int fd) {
    int status;
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"close on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    if (hdf5_data[fd]->set >= 0) {
        H5LTset_attribute_int(hdf_file,hdf5_data[fd]->name,"length",&hdf5_data[fd]->length,1);
        if ((status = H5Dclose(hdf5_data[fd]->set)) < 0) {
            fprintf(stderr,"error closing dataset '%s' %d\n",hdf5_data[fd]->name,status);
            errno = EIO;
            return(-1);
        }
    } else {
        _closed_empty_add(hdf5_data[fd]->name);
    }
    if ((hdf5_data[fd]->space >=0) && ((status = H5Sclose(hdf5_data[fd]->space)) < 0)) {
        fprintf(stderr,"error closing dataspace '%s' %d\n",hdf5_data[fd]->name,status);
        errno = EIO;
        return(-1);
    }
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
        fprintf(stderr,"write on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    if (d->append)
        d->offset[0]=d->length;
    int resize = 0;
    if ((d->offset[0]+count) > d->dims[0]) {
        resize = 1;
        d->dims[0]=d->offset[0]+count;
    }
    fprintf(stderr,"hdf5_write(%d='%s', %d) %d (%d)\n",fd,d->name,(int)count,(int)d->offset[0],(int)d->length);
    if (d->set < 0) {
        d->space = H5Screate_simple(RANK, d->dims, maxdims);
        d->set = H5Dcreate2(hdf_file, d->name, H5T_NATIVE_CHAR, d->space, H5P_DEFAULT, create_params, H5P_DEFAULT);
    } else if (resize) {
        H5Dset_extent(d->set, d->dims);
    }
    hsize_t hs_count[RANK];
    hs_count[0]=count;
    hid_t filespace = H5Dget_space(d->set);
    hid_t dataspace = H5Screate_simple(RANK, hs_count, NULL);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, d->offset, NULL, hs_count, NULL);

    H5Dwrite(d->set,H5T_NATIVE_CHAR,dataspace,filespace,H5P_DEFAULT,buf);
    H5Sclose(filespace);
    H5Sclose(dataspace);
    if ((d->offset[0]+count) > d->length)
        d->length = d->offset[0]+count;
    d->offset[0]+=count;
    return(count);
}

int hdf5_read(int fd, void *buf, size_t count) {
    if (count == 0)
        return(0);
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"write on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    // end of file
    if (d->offset[0] >= d->length)
        return(0);
    if (d->set < 0)
        return(0);
    size_t remaining_count = d->length - d->offset[0];
    if (remaining_count > count)
        remaining_count = count;
    fprintf(stderr,"hdf5_read(%d='%s', %d) %d (%d)\n",fd,d->name,(int)count,(int)d->offset[0],(int)d->length);

    hsize_t hs_count[RANK];
    hs_count[0]=remaining_count;
    hid_t filespace = H5Dget_space(d->set);
    hid_t dataspace = H5Screate_simple(RANK, hs_count, NULL);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, d->offset, NULL, hs_count, NULL);

    H5Dread(d->set,H5T_NATIVE_CHAR,dataspace,filespace,H5P_DEFAULT,buf);
    H5Sclose(filespace);
    H5Sclose(dataspace);
    d->offset[0]+=remaining_count;
    return(remaining_count);
}

int hdf5_lseek(int fd, off_t offset, int whence) {
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"write on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    switch (whence) {
        case SEEK_SET:
            d->offset[0]=offset;
            break;
        case SEEK_END:
            d->offset[0]=d->length+offset;
            break;
        case SEEK_CUR:
            d->offset[0]+=offset;
            break;
        default:
            errno=EINVAL;
            fprintf(stderr,"hdf5_lseek: illegal whence: %d\n",whence);
            return(-1);
            break;
    }
    fprintf(stderr,"hdf5_lseek(%d='%s',%d,%d) = %d\n",fd,d->name,(int)offset,whence,(int)d->offset[0]);
    return(d->offset[0]);
}

int hdf5_stat64(const char *pathname, struct stat64 *buf) {
    H5O_info_t object_info;
    if (_hdf5_path_exists(pathname) == 0) {
        if (_closed_empty_find(pathname) <= 0) {
            errno=ENOENT;
            return(-1);
        }
        (*buf)=hdf_file_stat;
        buf->st_blksize=chunk_dims[0];
        buf->st_size=0;
        buf->st_blocks=0;
        return(0);
    }
    herr_t status = H5Oget_info_by_name(hdf_file,pathname,&object_info,H5P_DEFAULT);
    if (status < 0) {
        fprintf(stderr,"hdf5_stat64: error getting status for '%s'\n",pathname);
        errno=ENOENT;
        return(-1);
    }
    (*buf)=hdf_file_stat;
    buf->st_blksize=chunk_dims[0];
    buf->st_atime  =object_info.atime;
    buf->st_mtime  =object_info.mtime;
    buf->st_ctime  =object_info.ctime;
    if (object_info.num_attrs > 0) {
        int fsize;
        H5LTget_attribute_int(hdf_file,pathname,"length",&fsize);
        buf->st_size=fsize;
        buf->st_blocks=fsize / 512 + 1;
    } else {
        buf->st_size=0;
        buf->st_blocks=0;
    }
    fprintf(stderr,"hdf5_stat64: size of '%s': %d\n",pathname,(int)buf->st_size);
    return(0);
}
int hdf5_fstat64(int fd, struct stat64 *buf) {
    if (hdf5_data[fd] == NULL) {
        fprintf(stderr,"write on unknown fd %d\n",fd);
        errno = EBADF;
        return(-1);
    }
    hdf5_data_t * d = hdf5_data[fd];
    (*buf)=hdf_file_stat;
    buf->st_blksize=chunk_dims[0];
    buf->st_size=d->length;
    buf->st_blocks=d->length / 512 + 1;
    if (d->set >= 0) {
        H5O_info_t object_info;
        herr_t status = H5Oget_info(d->set,&object_info);
        if (status < 0) {
            fprintf(stderr,"hdf5_fstat64: error getting status for '%s'\n",d->name);
            errno=EIO;
            return(-1);
        }
        buf->st_atime  =object_info.atime;
        buf->st_mtime  =object_info.mtime;
        buf->st_ctime  =object_info.ctime;
    }
    fprintf(stderr,"hdf5_fstat64: size of '%s': %d\n",d->name,(int)buf->st_size);
    return(0);
}

int hdf5_unlink(const char *pathname) {
    if (_hdf5_path_exists(pathname)==0) {
        if (_closed_empty_remove(pathname))
            return(0);
        errno=ENOENT;
        return(-1);
    }
    herr_t res = H5Ldelete(hdf_file,pathname,H5P_DEFAULT);
    if (res < 0) {
        errno=EIO;
        return(-1);
    }
    return(0);
}

int _hdf5_path_exists(const char *pathname) {
    int pathlen=strnlen(pathname,PATH_MAX);
    if (pathlen == 0) return(1);
    char testpath[PATH_MAX];
    strncpy(testpath,pathname,PATH_MAX);
    int i;
    htri_t testres;
    for (i=0;i<pathlen;i++) {
        if (testpath[i] == '/') {
            testpath[i] = 0;
            testres=H5Lexists(hdf_file,testpath,H5P_DEFAULT);
            if (testres <= 0)
                return(0);
            testpath[i] = '/';
        }
    }
    testres=H5Lexists(hdf_file,testpath,H5P_DEFAULT);
    if (testres <= 0)
        return(0);
    return(1);
}

int _closed_empty_add(const char *pathname) {
    int insert_pos=0;
    if (n_closed_empty_files>=CLOSED_EMPTY_MAX-1) {
        fprintf(stderr,"_closed_empty_add: too many closed empty files\n");
        return(0);
    }
    if (n_closed_empty_files!=0) {
        int left=0;
        int right=n_closed_empty_files-1;
        while (right>=left) {
            int middle=(right+left)/2;
//    fprintf(stderr,"_closed_empty_add: middle %d %d %d (%d)\n",middle,left,right,n_closed_empty_files);
            int cmpres=strcmp(closed_empty_files[middle],pathname);
//    fprintf(stderr,"_closed_empty_add: cmp '%s' '%s',  %d\n",closed_empty_files[middle],pathname,cmpres);
            if (cmpres==0)
                return(1);
            if (cmpres > 0) {
                right=middle-1;
            } else {
                left=middle+1;
            }
        }
        insert_pos=left;
    }

    fprintf(stderr,"_closed_empty_add: adding '%s'\n",pathname);
    if (insert_pos < n_closed_empty_files) {
        int i;
        for (i=n_closed_empty_files;i>insert_pos;i--) {
            closed_empty_files[i]=closed_empty_files[i-1];
        }
    }
    closed_empty_files[insert_pos]=malloc(strlen(pathname)+1);
    if (closed_empty_files[insert_pos]==NULL) {
        fprintf(stderr,"_closed_empty_add: malloc error\n");
        return(0);
    }
    strcpy(closed_empty_files[insert_pos],pathname);
    n_closed_empty_files++;
    return(1);
}
int _closed_empty_find(const char *pathname) {
    int item_pos=-1;
    if (n_closed_empty_files == 0)
        return(-1);
    int left=0;
    int right=n_closed_empty_files-1;
    while (right>=left) {
        int middle=(right+left)/2;
        int cmpres=strcmp(closed_empty_files[middle],pathname);
        if (cmpres==0) {
            item_pos=middle;
            break;
        }
        if (cmpres > 0) {
            right=middle-1;
        } else {
            left=middle+1;
        }
    }
    return(item_pos);
}
int _closed_empty_remove(const char *pathname) {
    int delete_pos = _closed_empty_find(pathname);
    if (delete_pos < 0)
        return(0);
    int i;
    free(closed_empty_files[delete_pos]);
    for (i=delete_pos;i<(n_closed_empty_files-1);i++)
        closed_empty_files[i]=closed_empty_files[i+1];
    n_closed_empty_files--;
    return(1);
}
void _closed_empty_dump() {
    int i;
    for (i=0;i<n_closed_empty_files;i++) {
        fprintf(stderr,"_closed_empty_dump: %s\n",closed_empty_files[i]);
    }
}
int _closed_empty_free() {
    int i;
    for (i=0;i<n_closed_empty_files;i++) {
        if (closed_empty_files[i]!=NULL) {
            free(closed_empty_files[i]);
        }
    }
    return(1);
}
