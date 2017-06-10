# hdf5fs-wrapper -- redirect file-IO to HDF5 files

hdf5fs-wrapper maps file-IO operations for selected files to HDF5 archives.

It was developed to run scientific simulation programs on
High-Performance-Computing (HPC) Clusters and their parallel filesystems,
specifically simulation codes which work on a large number of temporary
files.

HPC filesystems perform well for large files, and may even run into
serious trouble with the 'many-files' pattern:
 - worst-case: running out of INODES (i.e. reaching the maximum possible
   number of files on the filesystem)
 - creation/deletion of a file is an expensive operation (involves updates
   of filesystem metadata)

## Installation:

Installation instructions can be found in [INSTALL.md](INSTALL.md).


## Usage

```sh
[H5FS_BASE=...] [H5FS_FILE=...] h5fs-wrap my_program
```
### Environment variables

Options to h5fs-wrapper.so can only be passed as environment variables and
support simple expansion (see below):

  - $H5FS_BASE [Default: './H5FS_SCRATCH']
    IO of my_program is redirected if file resides below this directory
  - $H5FS_FILE [Default: './scratch_${OMPI_COMM_WORLD_RANK:%04d:0}.h5']
    IO of my_program will be redirected to this file

Only for advanced usage (COW support)
  - $H5FS_RO [Default: empty]
    If set, build a Copy-On-Write (COW) stack and use this hdf5 file as the
    read-only base.

Simple expansion inside the wrapper is supported. String literals are expanded
from other environment variables in the following cases:
  - ${VARIABLE}
    expands to the contents of environment variabe $VARIABLE
  - ${VARIABLE:format_string:default_value}
    * value is taken from $VARIABLE, or the literal default_value if $VARIABLE
      is not defined in environment (fallback mechanism
    * value is then formatted via sprintf format_string (currently supported
      formats: '%s' and '%d').
      e.g. 'scratch${OMPI_COMM_WORLD_RANK:%04d:0}.h5' expands to scratch0000.h5

## Examples
### Basic Usage

```sh
H5FS_FILE='./tmp.h5' H5FS_BASE=./my_tmp h5fs-wrap my_program
```
Runs 'my_program', redirecting all file-IO to files in directory 'my_tmp'
to the hdf5 archive 'tmp.h5'.
So if my_program writes to a file 'my_tmp/blah.txt', that file will end up
as dataset 'blah.txt' inside 'tmp.h5'.


### MPI (parallel) usage (openmpi)

```sh
mpirun -np 2 -x H5FS_FILE='./tmp${OMPI_COMM_WORLD_RANK:%04d:0}.h5' -x H5FS_BASE=./my_tmp h5fs-wrap my_program
```

Note:
 - each MPI rank needs to have its own hdf5 archive (the wrapper supports
   expansion of environment variables, with optional printf-format-string
   formatting); in the example, 'tmp0000.h5' and 'tmp0001.h5' are created
 - as each MPI rank effectively uses its own './my_tmp', this
   will only work if it only reads back files it has written by itself
 - for setting 'LD_PRELOAD', h5fs-wrap should be used; mpirun itself should
   not be wrapped (conflicting writes to the hdf5 of MPI rank 0)


### Advanced usage: Copy-on-write (COW) stacks

An advanced feature of hdf5fs-wrapper is the ability to work in copy-on-write (COW) mode:
```sh
# run my_program to create 'basis' of COW
mpirun -np 2 -x H5FS_FILE='./tmp${OMPI_COMM_WORLD_RANK:%04d:0}.h5' -x H5FS_BASE=./my_tmp h5fs-wrap my_program

# join/pack all created files into a single COW-basis
h5fs-repack COW_basis.h5 tmp0*.h5

# run my_postprocessor with COW_basis.h5 as read-only part of the COW stack
mpirun -np 2 -x H5FS_FILE='./tmp${OMPI_COMM_WORLD_RANK:%04d:0}.h5' -x H5FS_RO=COW_basis.h5 -x H5FS_BASE=./my_tmp h5fs-wrap my_postprocessor
```


## Mechanism

hdf5fs-wrapper is a library, inserted via $LD_PRELOAD in between your
program and the system library (libc).
It then intercepts all symbols (functions) related to file-IO.

If a filename lies below the directory $H5FS_BASE, the operation is
executed internally in the wrapper.
For all other files, the intercepted call is forwarded to the system
library without modification.

This kind of implementation comes with no decrease in performance; in
fact, execution speed even increases (due to the cost of
metadata-modifications on parallel filesystems and the lack thereof
when using hdf5fs-wrapper).
