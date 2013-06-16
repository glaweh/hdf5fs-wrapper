#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "file_ds.h"
#include "chunksize.h"
const file_ds_t __file_ds_initializer = {
    .space = -1, .set   = -1,
    .length_space = -1, .length_attrib = -1,
    .dims = { 0 }, .chunk = { 0 },
    .length = 0, .length_original = 0,
    .refcount = 0, .rdonly = 1,
    .next = NULL, .loc_id = -1, .name[0] = 0
};
const hsize_t maxdims[1] = {H5S_UNLIMITED};

herr_t file_ds_close(file_ds_t * info) {
    if (! info->rdonly) {
        if ((info->length != info->length_original) && 
                (H5Awrite(info->length_attrib,H5T_NATIVE_INT64,&info->length) < 0)) {
            LOG_ERR("error writing updated filesize attrib for '%s'",info->name);
        }
        hsize_t old_dim = info->dims[0];
        info->dims[0] = DIM_CHUNKED(info->length+1,info->chunk[0]);
        LOG_DBG("resizing %40s, length %6d, chunksize %6d, old_dim %6d, new_dim %6d",
                info->name,info->length,info->chunk[0],old_dim,info->dims[0]);
        if ((old_dim != info->dims[0]) && 
                (H5Dset_extent(info->set, info->dims)<0)) {
            LOG_ERR("error resizing datset for '%s' to %d",info->name,info->dims[0]);
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
    if ((info->set >= 0) && (H5Dclose(info->set) < 0)) {
        LOG_ERR("error closing dataset '%s'",info->name);
        status = 0;
    } else {
        info->set=-1;
    }
    return(status);
}

file_ds_t * file_ds_create(hid_t loc_id, const char *name, hsize_t chunk_size, hsize_t initial_dim, int deflate) {
    file_ds_t * info = malloc(sizeof(file_ds_t)+strlen(name));
    if (info == NULL) {
        LOG_ERR("error allocating file_ds_t");
        return(NULL);
    }
    *info = __file_ds_initializer;
    if (chunk_size <= 0) {
        LOG_ERR("no chunk size set for '%s'",name);
        goto errlabel;
    }
    if (initial_dim < 0) {
        LOG_ERR("initial_dim<0 set for '%s'",name);
        goto errlabel;
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
    strcpy(info->name,name);
    LOG_DBG("create %40s, chunksize %d, dim %d",name,info->chunk[0],info->dims[0]);
    if ((info->space = H5Screate_simple(1, info->dims, maxdims)) < 0) {
        LOG_ERR("error creating dataspace for '%s'",name);
        goto errlabel;
    }
    if ((info->set = H5Dcreate2(loc_id,name,H5T_FILE_DS, info->space, H5P_DEFAULT, create_params, H5P_DEFAULT)) < 0) {
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
    return(info);
    errlabel:
        H5Pclose(create_params);
        file_ds_close(info);
    return(NULL);
}

file_ds_t * file_ds_reopen(file_ds_t * info) {
    if ((info->set = H5Dopen(info->loc_id,info->name,H5P_DEFAULT)) < 0) {
        LOG_ERR("error opening dataset '%s'",info->name);
        goto errlabel;
    }
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
        file_ds_close(info);
    return(NULL);
}

file_ds_t * file_ds_open(hid_t loc_id, const char *name) {
    file_ds_t * info = malloc(sizeof(file_ds_t)+strlen(name));
    (*info) = __file_ds_initializer;
    info->loc_id = loc_id;
    strcpy(info->name,name);
    if (file_ds_reopen(info)==NULL) {
        free(info);
        return(NULL);
    }
    return(info);
}

file_ds_t * file_ds_copy(hid_t dst_loc_id, file_ds_t * src, hsize_t chunk_size, int deflate) {
    if (chunk_size == 0) chunk_size = chunksize_suggest(src->name, src->length);
    file_ds_t * target_set = file_ds_create(dst_loc_id, src->name,
        chunk_size, src->length, deflate);
    if (target_set == NULL) {
        LOG_ERR("target set creation failed when copying '%s'",src->name);
        return(NULL);
    }
    if (file_ds_copy_contents(target_set,src) < 0) {
        file_ds_close(target_set);
        return(NULL);
    }
    return(target_set);
}

herr_t file_ds_copy_contents(file_ds_t * dst, file_ds_t *src) {
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
        LOG_ERR("error allocating copy buffer for '%s', size %d",src->name,copy_block_size);
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
        LOG_DBG("file: %s, length: %d, offset: %d, size: %d, chunksize: %d",src->name,
                src->length,offset[0],hs_count[0], dst->chunk[0]);
        if (H5Sselect_hyperslab(source_space,H5S_SELECT_SET, offset, NULL, hs_count, NULL) < 0) {
            LOG_ERR("error selecting source hyperslab");
            goto errlabel;
        }
        if ((readspace = H5Screate_simple(1, hs_count, NULL)) < 0) {
            LOG_ERR("error creating readspace");
            goto errlabel;
        }
        if (H5Dread(src->set,H5T_FILE_DS,readspace,source_space,H5P_DEFAULT,buffer) < 0) {
            LOG_ERR("error reading source hyperslab");
            goto errlabel;
        }
        if (H5Sselect_hyperslab(dst_space,H5S_SELECT_SET, offset, NULL, hs_count, NULL) < 0) {
            LOG_ERR("error selecting dest hyperslab");
            goto errlabel;
        }
        if (H5Dwrite(dst->set,H5T_FILE_DS,readspace,dst_space,H5P_DEFAULT,buffer) < 0) {
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
