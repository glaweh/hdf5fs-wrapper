/*
 * Copyright (c) 2017 Henning Glawe <glaweh@debian.org>
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

#include <stdlib.h>
#include <hdf5.h>
#include <logger.h>
#include <hfile_ds.h>

const char * hdf5name = "test_setextend.h5";
hsize_t chunksize[1] = { 64*1 };
hsize_t dims[1] = { 0 };
hsize_t maximum_dims[1] = { H5S_UNLIMITED };
const char * dsname1 = "test_setextend";
const char * dsname2 = "truncated";
const int testsize0 = 64*4;
const int testsize1 = 84;
const int testsize2 = 192;

void die(const char * msg) {
    LOG_ERR(msg);
    abort();
}

int main (int argc, char * argv[]) {
    logger_init("H5FS");
    // create HDF file for test
    hid_t this_hdf = -1;
    if ((this_hdf = H5Fcreate(hdf5name,H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT)) <= 0)
        die("err01");
    
    // create chunked test dataset
    //   setup chunking parameters
    hid_t create_params = H5Pcreate(H5P_DATASET_CREATE);
    if (create_params < 0)
        die("err02_H5P");
    if (H5Pset_chunk(create_params, 1, chunksize) < 0)
        die("err03_HP5chunk");
    //   create space and dataset
    hid_t space;
    if ((space = H5Screate_simple(1, dims, maximum_dims))<0)
        die("err04_H5S");
    hid_t ds;
    if ((ds = H5Dcreate2(this_hdf,dsname1,H5T_HFILE_DS, space, H5P_DEFAULT, create_params, H5P_DEFAULT))<=0)
        die("err05_H5D");

    // set initial extent for dataset
    hsize_t dims[1];
    dims[0] = testsize0;
    if (H5Dset_extent(ds, dims) < 0)
        die("err06_setextend0");

    // initialize dataset with sequence of numbers
    //   setup sequence of numbers in-memory
    unsigned char testarr0[dims[0]];
    for (int i=0; i<dims[0]; i++)
        testarr0[i]=(char)(i & 255);
    //   write sequence to hdf5 dataset
    if (H5Dwrite(ds,H5T_HFILE_DS,H5S_ALL,H5S_ALL,H5P_DEFAULT,testarr0) < 0)
        die("err07_write");

    // truncate by resizing
    dims[0] = testsize1;
    if (H5Dset_extent(ds, dims) < 0)
        die("err08_setextend1");
    dims[0] = testsize0;
    if (H5Dset_extent(ds, dims) < 0)
        die("err09_setextend2");

    if (H5Dclose(ds) < 0)
        die("err11_close_ds");
    if (H5Sclose(space) < 0)
        die("err10_close_space");

    //   create space and dataset
    if ((space = H5Screate_simple(1, dims, maximum_dims))<0)
        die("err11_H5S");
    if ((ds = H5Dcreate2(this_hdf,dsname2,H5T_HFILE_DS, space, H5P_DEFAULT, create_params, H5P_DEFAULT))<=0)
        die("err12_H5D");

    // set initial extent for dataset
    dims[0] = testsize0;
    if (H5Dset_extent(ds, dims) < 0)
        die("err13_setextend0");

    // initialize dataset with sequence of numbers
    //   write sequence to hdf5 dataset
    if (H5Dwrite(ds,H5T_HFILE_DS,H5S_ALL,H5S_ALL,H5P_DEFAULT,testarr0) < 0)
        die("err14_write");

    // truncate by chunked overwrite by zeroes
    char * chunk_of_zeroes = calloc(chunksize[0], 1);
    hid_t filespace;
    if ((filespace=H5Dget_space(ds)) < 0)
        die("err15_filespace");
    hsize_t hs_count[1];
    // erase up to the end of first chunk
    hs_count[0] = chunksize[0] - (testsize1 % chunksize[0]);
    hid_t memspace;
    if ((memspace=H5Screate_simple(1, hs_count, maximum_dims)) < 0)
        die("err15_memspace");
    hsize_t hs_offset[1];
    hs_offset[0] = testsize1;
    if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, hs_offset, NULL, hs_count, NULL) < 0)
        die("err16_select");
    if (H5Dwrite(ds,H5T_HFILE_DS,memspace,filespace,H5P_DEFAULT,chunk_of_zeroes) < 0)
        die("err17_write");
    hs_offset[0] += hs_count[0];
    hs_count[0] = chunksize[0];
    if (H5Sset_extent_simple(memspace, 1, hs_count, maximum_dims) < 0)
        die("err18_Sset_extent");

    while (hs_offset[0] < testsize2) {
        if (H5Sselect_hyperslab(filespace, H5S_SELECT_SET, hs_offset, NULL, hs_count, NULL) < 0)
            die("err19_select");
        if (H5Dwrite(ds,H5T_HFILE_DS,memspace,filespace,H5P_DEFAULT,chunk_of_zeroes) < 0)
            die("err20_write");
        hs_offset[0] += chunksize[0];
    }

    free(chunk_of_zeroes);
    H5Fclose(this_hdf);
}
