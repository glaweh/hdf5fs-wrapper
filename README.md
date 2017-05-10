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

## Basic Usage
```sh
H5FS_FILE='./tmp.h5' LD_PRELOAD=h5fs-wrapper.so H5FS_BASE=./my_tmp my_program
```
Runs 'my_program', redirecting all file-IO to files in directory 'my_tmp'
to the hdf5 archive 'tmp.h5'.
So if my_program writes to a file 'my_tmp/blah.txt', that file will end up
as dataset 'blah.txt' inside 'tmp.h5'.

### MPI (parallel) usage (openmpi)
```sh
mpirun -np 2 -x H5FS_FILE='./tmp${OMPI_COMM_WORLD_RANK:%04d:0}.h5' -x LD_PRELOAD=h5fs-wrapper.so -x H5FS_BASE=./my_tmp my_program
```
Note:
 - each MPI rank needs to have its own hdf5 archive (the wrapper supports
   expansion of environment variables, with optional printf-format-string
   formatting); in the example, 'tmp0000.h5' and 'tmp0001.h5' are create
 - as each MPI rank effectively uses its own './my_tmp', this
   will only work if it only reads back files it has written by itself
 - at least for setting 'LD_PRELOAD', the "mpirun -x" mechanism or
   a simple shell loader must be used; mpirun itself should not be run
   with LD_PRELOAD

## Advanced usage
### Copy-on-write (COW)
An advanced feature of hdf5fs-wrapper is the ability to work in copy-on-write (COW) mode:
```sh
# run my_program to create 'basis' of COW
mpirun -np 2 -x H5FS_FILE='./tmp${OMPI_COMM_WORLD_RANK:%04d:0}.h5' -x LD_PRELOAD=h5fs-wrapper.so -x H5FS_BASE=./my_tmp my_program

# join/pack all created files into a single COW-basis
h5fs-repack COW_basis.h5 tmp0*.h5

# run my_postprocessor with COW_basis.h5 as read-only part of the COW stack
mpirun -np 2 -x H5FS_FILE='./tmp${OMPI_COMM_WORLD_RANK:%04d:0}.h5' -x H5FS_RO=COW_basis.h5 -x LD_PRELOAD=h5fs-wrapper.so -x H5FS_BASE=./my_tmp my_postprocessor
```
