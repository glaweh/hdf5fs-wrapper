#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "logger.h"
#include "hfile_ds.h"
#include "chunksize.h"
#include "path_util.h"
const hfile_ds_t __hfile_ds_initializer = {
    .space = -1, .set   = -1,
    .length_space = -1, .length_attrib = -1,
    .dims = { 0 }, .chunk = { 0 },
    .length = 0, .length_original = 0,
    .rdonly = 1,
    .atime = 0, .mtime = 0, .ctime = 0,
    .next = NULL, .loc_id = -1, .name[0] = 0
};
const hsize_t __hfile_ds_maxdims[1] = {H5S_UNLIMITED};

herr_t hfile_ds_close(hfile_ds_t * info) {
    if (info->set < 0) {
        LOG_DBG("file dataset '%s' already closed",info->name);
        return(1);
    }
    if (! info->rdonly) {
        if ((info->length != info->length_original) && 
                (H5Awrite(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0)) {
            LOG_ERR("error writing updated filesize attrib for '%s'",info->name);
        }
        hsize_t old_dim = info->dims[0];
        info->dims[0] = DIM_CHUNKED(llabs(info->length)+1,info->chunk[0]);
        if (old_dim != info->dims[0]) {
            LOG_DBG("resizing %40s, length %6"PRIi64", chunksize %6llu, old_dim %6llu, new_dim %6llu",
                info->name,info->length,info->chunk[0],old_dim,info->dims[0]);
            if (H5Dset_extent(info->set, info->dims)<0) {
                LOG_ERR("error resizing datset for '%s' to %llu",info->name,info->dims[0]);
            }
        }
        // FIXME: use H5Dfill to fill the emtpy part
    }
    int status = 1;
    if ((info->length_space >= 0) && (H5Sclose(info->length_space) < 0)) {
        LOG_ERR("error closing length space for '%s'",info->name);
        status = 0;
    } else {
        info->length_space = -1;
    }
    if ((info->length_attrib >= 0) && (H5Aclose(info->length_attrib) < 0)) {
        LOG_ERR("error closing length attribute for '%s'",info->name);
        status = 0;
    } else {
        info->length_attrib=-1;
    }
    if ((info->space >= 0) && (H5Sclose(info->space) < 0)) {
        LOG_ERR("error closing data space for '%s'",info->name);
        status = 0;
    } else {
        info->space=-1;
    }
    hfile_ds_update_timestamps(info);
    if ((info->set >= 0) && (H5Dclose(info->set) < 0)) {
        LOG_ERR("error closing dataset '%s'",info->name);
        status = 0;
    } else {
        info->set=-1;
    }
    return(status);
}

hfile_ds_t * hfile_ds_create(hid_t loc_id, const char *name, hsize_t chunk_size, hssize_t initial_length, hsize_t expected_length, int deflate) {
    hfile_ds_t * info = malloc(sizeof(hfile_ds_t)+strlen(name));
    if (info == NULL) {
        LOG_ERR("error allocating hfile_ds_t");
        return(NULL);
    }
    *info = __hfile_ds_initializer;
    if (chunk_size == 0) {
        chunk_size = chunksize_suggest(name, expected_length);
    }
    hsize_t initial_dim = initial_length;
    if (initial_dim <= 0) {
        initial_dim = 1;
    }
    info->chunk[0]=chunk_size;
    hid_t   create_params = H5Pcreate(H5P_DATASET_CREATE);
    herr_t         status = H5Pset_chunk(create_params, 1, info->chunk);
    if (status < 0) {
        LOG_WARN("error setting up chunking");
        goto errlabel;
    }
    if (deflate > 0) {
        status=H5Pset_deflate(create_params,deflate);
        if (status < 0) {
            LOG_WARN("error setting up compression");
            goto errlabel;
        }
    }
    info->dims[0] = DIM_CHUNKED(initial_dim,chunk_size);
    if (info->dims[0] == 0) info->dims[0] = info->chunk[0];
    info->loc_id = loc_id;
    info->length = info->length_original = initial_length;
    strcpy(info->name,name);
    LOG_DBG("create %40s, chunksize %llu, dim %llu",name,info->chunk[0],info->dims[0]);
    if ((info->space = H5Screate_simple(1, info->dims, __hfile_ds_maxdims)) < 0) {
        LOG_ERR("error creating dataspace for '%s'",name);
        goto errlabel;
    }
    if ((info->set = H5Dcreate2(loc_id,name,H5T_HFILE_DS, info->space, H5P_DEFAULT, create_params, H5P_DEFAULT)) < 0) {
        LOG_ERR("error creating dataset '%s'",name);
        goto errlabel;
    }
    H5Pclose(create_params);
    if ((info->length_space = H5Screate(H5S_SCALAR)) < 0) {
        LOG_ERR("error creating filesize attribute dataspace of '%s'",name);
        goto errlabel;
    }
    if ((info->length_attrib = H5Acreate2(info->set,"Filesize",H5T_NATIVE_INT64,
                    info->length_space, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        LOG_ERR("error creating filesize attribute of '%s'",name);
        goto errlabel;
    }
    if (H5Awrite(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0) {
        LOG_ERR("Error resetting filesize attrib for '%s'",name);
        goto errlabel;
    }
    info->rdonly = 0;
    return(info);
    errlabel:
        H5Pclose(create_params);
        hfile_ds_close(info);
    return(NULL);
}

hfile_ds_t * hfile_ds_reopen(hfile_ds_t * info) {
    if (info->set >= 0) {
        return(info);
    }
    if ((info->set = H5Dopen(info->loc_id,info->name,H5P_DEFAULT)) < 0) {
        LOG_ERR("error opening dataset '%s'",info->name);
        goto errlabel;
    }
    int rdold = info->rdonly;
    info->rdonly = 0;
    hfile_ds_update_timestamps(info);
    info->rdonly = rdold;
    if ((info->space = H5Dget_space(info->set)) < 0) {
        LOG_ERR("error getting dataspace for '%s'",info->name);
        goto errlabel;
    }
    if (H5Sget_simple_extent_dims(info->space, info->dims, NULL) < 0) {
        LOG_ERR("error getting dimensions of '%s'",info->name);
        goto errlabel;
    }
    if ((info->length_attrib = H5Aopen(info->set,"Filesize",H5P_DEFAULT)) < 0) {
        LOG_ERR("error opening filesize attribute of '%s'",info->name);
        goto errlabel;
    }
    if ((info->length_space = H5Aget_space(info->length_attrib)) < 0) {
        LOG_ERR("error opening filesize attribute dataspace of '%s'",info->name);
        goto errlabel;
    }
    if (H5Aread(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0) {
        LOG_ERR("Error reading filesize attrib for '%s'",info->name);
        goto errlabel;
    }
    hid_t dcpl;
    if ((dcpl = H5Dget_create_plist(info->set)) < 0) {
        LOG_ERR("error getting create plist for '%s'",info->name);
        goto errlabel;
    }
    if (H5D_CHUNKED != H5Pget_layout(dcpl)) {
        LOG_ERR("dataset '%s' is not chunked",info->name);
        goto err_dcpl;
    }
    if (H5Pget_chunk(dcpl,1,info->chunk) < 0) {
        LOG_ERR("error getting chunk properties for '%s'",info->name);
        goto err_dcpl;
    }
    H5Pclose(dcpl);
    info->length_original = info->length;
    return(info);
    err_dcpl:
        H5Pclose(dcpl);
    errlabel:
        hfile_ds_close(info);
    return(NULL);
}

hfile_ds_t * hfile_ds_open(hid_t loc_id, const char *name) {
    hfile_ds_t * info = malloc(sizeof(hfile_ds_t)+strlen(name));
    (*info) = __hfile_ds_initializer;
    info->loc_id = loc_id;
    strcpy(info->name,name);
    if (hfile_ds_reopen(info)==NULL) {
        free(info);
        return(NULL);
    }
    return(info);
}

hfile_ds_t * hfile_ds_copy(hid_t dst_loc_id, hfile_ds_t * src, hsize_t chunk_size, int deflate) {
    hfile_ds_t * target_set = hfile_ds_create(dst_loc_id, src->name,
        chunk_size, src->length, src->length, deflate);
    if (target_set == NULL) {
        LOG_ERR("target set creation failed when copying '%s'",src->name);
        return(NULL);
    }
    if (src->length <= 0)
        return(target_set);
    if (hfile_ds_copy_contents(target_set,src) < 0) {
        hfile_ds_close(target_set);
        return(NULL);
    }
    return(target_set);
}

herr_t hfile_ds_copy_contents(hfile_ds_t * dst, hfile_ds_t *src) {
    if (dst->dims[0] < (src->length+1)) {
        dst->dims[0] = DIM_CHUNKED(src->length+1,dst->chunk[0]);
        if (H5Dset_extent(dst->set, dst->dims) < 0) {
            LOG_ERR("error resizing dst dataset when copying '%s'",src->name);
            return(-1);
        }
    }
    // by default, copy in blocks of 10MB, but adapt to be commensurate with dst chunk size
    hsize_t copy_block_size = DIM_CHUNKED(10*1024*1024,dst->chunk[0]);
    // for smaller sets, fall back to actual set length
    if (copy_block_size > src->length) copy_block_size = src->length;
    char *buffer = malloc(copy_block_size);
    if (buffer == NULL) {
        LOG_ERR("error allocating copy buffer for '%s', size %llu",src->name,copy_block_size);
        return(-1);
    }
    hid_t source_space = -1;
    hid_t dst_space    = -1;
    if ((source_space=H5Dget_space(src->set)) < 0) {
        LOG_ERR("error getting source space for '%s'",src->name);
        goto errlabel;
    }
    if ((dst_space=H5Dget_space(dst->set)) < 0) {
        LOG_ERR("error getting destination space for '%s'",dst->name);
        goto errlabel;
    }

    hsize_t offset[1] = { 0 };
    hsize_t to_copy = src->length;
    hsize_t hs_count[1];
    hid_t   readspace = -1;
    while (to_copy > 0) {
        hs_count[0] = (to_copy > copy_block_size ? copy_block_size : to_copy);
        LOG_DBG("file: %s, length: %"PRIi64", offset: %llu, size: %llu, chunksize: %llu",src->name,
                src->length,offset[0],hs_count[0], dst->chunk[0]);
        if (H5Sselect_hyperslab(source_space,H5S_SELECT_SET, offset, NULL, hs_count, NULL) < 0) {
            LOG_ERR("error selecting source hyperslab");
            goto errlabel;
        }
        if ((readspace = H5Screate_simple(1, hs_count, NULL)) < 0) {
            LOG_ERR("error creating readspace");
            goto errlabel;
        }
        if (H5Dread(src->set,H5T_HFILE_DS,readspace,source_space,H5P_DEFAULT,buffer) < 0) {
            LOG_ERR("error reading source hyperslab");
            goto errlabel;
        }
        if (H5Sselect_hyperslab(dst_space,H5S_SELECT_SET, offset, NULL, hs_count, NULL) < 0) {
            LOG_ERR("error selecting dest hyperslab");
            goto errlabel;
        }
        if (H5Dwrite(dst->set,H5T_HFILE_DS,readspace,dst_space,H5P_DEFAULT,buffer) < 0) {
            LOG_ERR("error writing dest hyperlab");
            goto errlabel;
        }
        H5Sclose(readspace);
        to_copy -= hs_count[0];
        offset[0]+=hs_count[0];
    }
    dst->length = src->length;

    free(buffer);
    H5Sclose(source_space);
    H5Sclose(dst_space);
    return(1);
errlabel:
    free(buffer);
    if (source_space >= 0) H5Sclose(source_space);
    if (dst_space >= 0) H5Sclose(dst_space);
    if (readspace >= 0) H5Sclose(readspace);
    return(-1);
}

int hfile_ds_exists(hid_t loc_id, const char *pathname) {
    if (pathname[0] == 0) return(1);
    int pathlen=strnlen(pathname,PATH_MAX);
    if (pathlen == 0) return(1);
    char testpath[PATH_MAX];
    strncpy(testpath,pathname,PATH_MAX);
    int i;
    htri_t testres;
    for (i=0;i<pathlen;i++) {
        if (testpath[i] == '/') {
            testpath[i] = 0;
            testres=H5Lexists(loc_id,testpath,H5P_DEFAULT);
            if (testres <= 0)
                return(0);
            testpath[i] = '/';
        }
    }
    testres=H5Lexists(loc_id,testpath,H5P_DEFAULT);
    if (testres <= 0)
        return(0);
    return(1);
}

hssize_t hfile_ds_read(hfile_ds_t * hfile_ds, hsize_t offset, void * buf, hsize_t count) {
    if (hfile_ds->set < 0) return(-1);
    if (count == 0) return(0);
    hssize_t remaining_count = hfile_ds->length - offset;
    if (remaining_count <= 0) return(0);
    if (remaining_count > count) remaining_count = count;
    LOG_DBG("'%s', offset: %llu, count: %llu, length: %"PRIi64", remcnt: %lld",
            hfile_ds->name,offset,count,hfile_ds->length,remaining_count);
    hsize_t hs_offset[1], hs_count[1];
    hs_offset[0]=offset;
    hs_count[0]=remaining_count;
    hid_t filespace=-1;
    hid_t dataspace=-1;
    if ((filespace=H5Dget_space(hfile_ds->set)) < 0) {
        LOG_WARN("error getting filespace for '%s'",hfile_ds->name);
        goto errlabel;
    }
    if ((dataspace = H5Screate_simple(1, hs_count, NULL)) < 0) {
        LOG_WARN("error getting dataspace for '%s'",hfile_ds->name);
        goto errlabel;
    }
    if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, hs_offset, NULL, hs_count, NULL) < 0) {
        LOG_WARN("error selecting hyperslab for '%s'",hfile_ds->name);
        goto errlabel;
    }
    if (H5Dread(hfile_ds->set,H5T_HFILE_DS,dataspace,filespace,H5P_DEFAULT,buf) < 0) {
        hsize_t fsdims[1];
        H5Sget_simple_extent_dims(hfile_ds->space, fsdims, NULL);
        LOG_WARN("error reading data from '%s', offset: %"PRIi64", count %"PRIi64", end %"PRIi64" (length: %"PRIi64", fsdims: %"PRIi64")",
            hfile_ds->name,(int64_t)offset,(int64_t)count,(int64_t)(offset+count),
            (int64_t)hfile_ds->length,
            (int64_t)fsdims[0]);
        goto errlabel;
    }
    H5Sclose(filespace);
    H5Sclose(dataspace);
    return(remaining_count);
errlabel:
    if (filespace >= 0) H5Sclose(filespace);
    if (dataspace >= 0) H5Sclose(dataspace);
    return(-2);
}

hssize_t hfile_ds_write(hfile_ds_t * hfile_ds, hsize_t offset, const void *buf, hsize_t count) {
    if (hfile_ds->set < 0) return(-1);
    if (hfile_ds->rdonly) return(-1);
    if (count == 0) return(0);
    hsize_t newlength = offset+count;
    if (newlength < hfile_ds->length) newlength = hfile_ds->length;
    if (hfile_ds->dims[0] < (newlength+1)) {
        hsize_t newdims[1];
        newdims[0] = DIM_CHUNKED(newlength+1,hfile_ds->chunk[0]);
        LOG_DBG("resizing dataset '%s' from %d to %d",hfile_ds->name,(int)hfile_ds->dims[0],(int)newdims[0]);
        if (H5Dset_extent(hfile_ds->set, newdims) < 0) {
            LOG_ERR("error resizing dataset '%s'",hfile_ds->name);
            return(-2);
        }
        hfile_ds->dims[0]=newdims[0];
    }
    LOG_DBG("'%s', offset: %d, count: %d, length: %d, newlength: %d",
            hfile_ds->name,(int)offset,(int)count,(int)hfile_ds->length,(int)newlength);
    hsize_t hs_offset[1], hs_count[1];
    hs_offset[0]=offset;
    hs_count[0]=count;
    hid_t filespace=-1;
    hid_t dataspace=-1;
    if ((filespace=H5Dget_space(hfile_ds->set)) < 0) {
        LOG_WARN("error getting filespace for '%s'",hfile_ds->name);
        goto errlabel;
    }
    if ((dataspace = H5Screate_simple(1, hs_count, NULL)) < 0) {
        LOG_WARN("error getting dataspace for '%s'",hfile_ds->name);
        goto errlabel;
    }
    if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, hs_offset, NULL, hs_count, NULL) < 0) {
        LOG_WARN("error selecting hyperslab for '%s'",hfile_ds->name);
        goto errlabel;
    }
    if (H5Dwrite(hfile_ds->set,H5T_HFILE_DS,dataspace,filespace,H5P_DEFAULT,buf) < 0) {
        LOG_WARN("error writing data to '%s'",hfile_ds->name);
        goto errlabel;
    }
    hfile_ds->length=newlength;
    H5Sclose(filespace);
    H5Sclose(dataspace);
    return(count);
errlabel:
    if (filespace >= 0) H5Sclose(filespace);
    if (dataspace >= 0) H5Sclose(dataspace);
    return(-3);
}

hssize_t hfile_ds_export(hfile_ds_t * src, const char * filename) {
    if (src->set < 0) return(-1);
    char export_name[PATH_MAX];
    strcpy(export_name,filename);
    // create dir if necessary
    int name_len=strlen(filename);
    int i;
    for (i=name_len; i > 0; i--) {
        if (export_name[i] == '/') break;
    }
    if (export_name[i]=='/') {
        char bkp = export_name[i+1];
        export_name[i+1]=0;
        if (! mkpath(export_name)) {
            LOG_ERR("error creating dir '%s'",export_name);
            return(-1);
        }
        export_name[i+1]=bkp;
    }

    FILE * export_file = fopen(filename,"wb");
    if (export_file == NULL) {
        LOG_ERR("error opening export file '%s': %s",filename,strerror(errno));
    }
    hsize_t copy_block_size = DIM_CHUNKED(10*1024*1024,src->chunk[0]);
    if (copy_block_size > src->length) copy_block_size = src->length;
    char *buffer = malloc(copy_block_size);
    if (buffer == NULL) {
        LOG_ERR("error allocating copy buffer for '%s', size %llu",src->name,copy_block_size);
        return(-2);
    }
    hid_t source_space = -1;
    if ((source_space=H5Dget_space(src->set)) < 0) {
        LOG_ERR("error getting source space for '%s'",src->name);
        goto errlabel;
    }
    hsize_t offset[1] = { 0 };
    hsize_t to_copy = src->length;
    hsize_t hs_count[1];
    hid_t   readspace = -1;
    while (to_copy > 0) {
        hs_count[0] = (to_copy > copy_block_size ? copy_block_size : to_copy);
        LOG_DBG("file: %s, length: %"PRIi64", offset: %llu, size: %llu",src->name,
                src->length,offset[0],hs_count[0]);
        if (H5Sselect_hyperslab(source_space,H5S_SELECT_SET, offset, NULL, hs_count, NULL) < 0) {
            LOG_ERR("error selecting source hyperslab");
            goto errlabel;
        }
        if ((readspace = H5Screate_simple(1, hs_count, NULL)) < 0) {
            LOG_ERR("error creating readspace");
            goto errlabel;
        }
        if (H5Dread(src->set,H5T_HFILE_DS,readspace,source_space,H5P_DEFAULT,buffer) < 0) {
            LOG_ERR("error reading source hyperslab");
            goto errlabel;
        }
        size_t to_write = hs_count[0];
        char * write_ptr = buffer;
        while (to_write > 0) {
            size_t n_written = fwrite(write_ptr,1,to_write,export_file);
            if ((n_written < to_write) && (ferror(export_file)!=0)) {
                LOG_ERR("error writing to file '%s'",filename);
                clearerr(export_file);
                goto errlabel;
            }
            write_ptr+=n_written;
            to_write-=n_written;
        }
        H5Sclose(readspace);
        to_copy -= hs_count[0];
        offset[0]+=hs_count[0];
    }
    fclose(export_file);
    free(buffer);
    H5Sclose(source_space);
    return(1);
errlabel:
    if (export_file != NULL) fclose(export_file);
    free(buffer);
    if (source_space >= 0) H5Sclose(source_space);
    if (readspace >= 0) H5Sclose(readspace);
    return(-1);

}
void hfile_ds_update_timestamps(hfile_ds_t * ds) {
    H5O_info_t objinfo;
    if ((ds->set < 0) || (ds->rdonly == 1)) return;
    H5Oget_info(ds->set,&objinfo);
    ds->atime=objinfo.atime;
    ds->mtime=objinfo.mtime;
    ds->ctime=objinfo.ctime;
}
