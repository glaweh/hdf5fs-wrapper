#!/bin/bash 

# module load hdf5fs-wrapper/.170509
export LD_PRELOAD='/srv/work/build/hdf5fs-wrapper/h5fs-wrapper.so'
export H5FS_BASE=./h5fs_scratch 

# export H5FS_FILE='./scratch_${MP_CHILD:%s:nochild}.h5'
export H5FS_FILE='./scratch_${OMPI_COMM_WORLD_RANK:%04d:0}.h5'
exec "$@"
