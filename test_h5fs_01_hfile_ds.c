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
#include "hfile_ds.h"
#include <stdio.h>
#include <stdlib.h>

const char * hdf_name = "test_1.h5";
const char * ds_name1="ds_name1.dat";
const char * ds_name2="ds_name2.dat";
const char * ds_name3="ds_name3.dat";

#define DATALEN 42
char data[DATALEN];

int main(int argc, char *argv[]) {
    puts("creating/truncating hdf_file: ");
    hid_t hdf = H5Fcreate(hdf_name,H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
    puts(hdf < 0 ? "failed\n" : "success\n");
    if (hdf < 0) exit(1);

    puts("creating negative-length ds: ");
    hfile_ds_t * fn = hfile_ds_create(hdf, ds_name1, 0, -1, 0, 0);
    puts(fn == NULL ? "failed\n" : "success\n");
    if (fn == NULL) exit(1);

    int i;
    for (i=0;i<DATALEN;i++) data[i]=i;

    puts("creating empty ds: ");
    hfile_ds_t * f1 = hfile_ds_create(hdf, ds_name2, 0, 0, 0, 0);
    puts(fn == NULL ? "failed\n" : "success\n");
    if (fn == NULL) exit(1);

    hssize_t written;
    puts("writing data to ds: ");
    written=hfile_ds_write(f1,0,data,42);
    puts(written != 42 ? "failed\n" : "success\n");
    if (written != 42) exit(1);

    puts("inserting data into ds");
    written=hfile_ds_write(f1,33,data,42);
    puts(written != 42 ? "failed\n" : "success\n");
    if (written != 42) exit(1);

    puts("checking length of ds");
    puts(f1->length != 75 ? "failed\n" : "success\n");
    if (f1->length != 75) exit(1);

    hfile_ds_close(fn);
    hfile_ds_close(f1);

    H5Fclose(hdf);
    return(0);
}
