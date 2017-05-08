/*
 * Utility functions to suggest chunk size of HDF5 datasets (1D, used
 * to represent files in hdf5fs-wrapper)
 *
 * Copyright (c) 2013, 2017 Henning Glawe <glaweh@debian.org>
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
#include "chunksize.h"
hsize_t chunksize_default = 64 * 1024;
hsize_t chunksize_suggest(const char * name,const hsize_t expected_length) {
    if (expected_length > 0) {
        if (expected_length < 16*1024) {
            return(2*1024);
        } else if (expected_length < 32*1024) {
            return(4*1024);
        } else if (expected_length < 128*1024) {
            return(64*1024);
        } else if (expected_length < 10*1024*1024) {
            return(256*1024);
        } else {
            return(1024*1024);
        }
    }
    return(chunksize_default);
}
