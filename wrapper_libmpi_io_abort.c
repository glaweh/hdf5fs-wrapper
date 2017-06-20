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
#include "logger.h"

/*
 * The actual datatypes of MPI_File_open arguments are
 * implementation-dependent, so we just abort the wrapped process if any of
 * the MPI_File-IO symbols are called (without looking at the arguments).
 */

#define ABORT_MPI_IO(MPI_IO_CALL) \
void (MPI_IO_CALL)() { \
    LOG_ERR("MPI-IO is not supported by hdf5fs-wrapper, aborting"); \
    abort(); \
}

// C symbols
ABORT_MPI_IO(MPI_File_open)
ABORT_MPI_IO(PMPI_File_open)
ABORT_MPI_IO(ompi_file_open)

// F77 bindings
ABORT_MPI_IO(pmpi_file_open_)
ABORT_MPI_IO(pmpi_file_open__)
ABORT_MPI_IO(mpi_file_open)
ABORT_MPI_IO(PMPI_FILE_OPEN)
ABORT_MPI_IO(MPI_FILE_OPEN)
ABORT_MPI_IO(mpi_file_open_)
ABORT_MPI_IO(mpi_file_open__)
ABORT_MPI_IO(mpi_file_open_f)
ABORT_MPI_IO(pmpi_file_open)
