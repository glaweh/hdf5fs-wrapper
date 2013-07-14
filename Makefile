CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=4
LDLIBS:=-ldl -lhdf5 -lc
HDFFS_OBJ:=real_func_auto.o logger.o process_info.o hfile_ds.o chunksize.o hdir.o path_util.o hstack_tree.o env_util.o

wrapper_func_auto.o: CFLAGS:=$(CFLAGS) -Wno-unused-variable -Wno-unused-label -Wno-unused-but-set-variable
h5fs.o: CFLAGS:=$(CFLAGS) -ULOG_LEVEL -DLOG_LEVEL=5

all: h5fs-wrapper.so hdf5fs-repack hdf5fs-unpack
test: test_h5fs_01_hfile_ds

hdf5fs-wrapper.so: hdf5fs-wrapper.o path_util.o hdf5_fs.o $(HDFFS_OBJ)
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
hdf5fs-wrapper.o: hdf5fs-wrapper.c logger.h process_info.h real_func_auto.h
logger.o: logger.c real_func_auto.h
process_info.o: process_info.c real_func_auto.h
path_util.o: path_util.c real_func_auto.h
wrapper_func.o: wrapper_func.c real_func_auto.h

h5fs-wrapper.so: wrapper_func.o wrapper_func_auto.o $(HDFFS_OBJ) h5fs.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
real_func_auto.c real_func_auto.h wrapper_func_auto.c: wrapper-gen/wrapper-gen.pl wrapper-gen/io-calls.c wrapper_func.c
	./wrapper-gen/wrapper-gen.pl wrapper-gen/io-calls.c wrapper_func.c

hdf5fs-repack: hdf5fs-repack.o logger.h process_info.h $(HDFFS_OBJ)
hdf5fs-unpack: hdf5fs-unpack.o logger.h process_info.h $(HDFFS_OBJ)

test_rel2abs:    test_rel2abs.o path_util.o     logger.o process_info.o
test_env_util:   test_env_util.o env_util.o     logger.o process_info.o
test_pathcmp:    test_pathcmp.o path_util.o     logger.o process_info.o
test_logger:     test_logger.o                  logger.o process_info.o

test_h5fs_01_hfile_ds.o: test_h5fs_01_hfile_ds.c hfile_ds.h
test_h5fs_01_hfile_ds: test_h5fs_01_hfile_ds.o hfile_ds.o logger.o process_info.o chunksize.o path_util.o real_func_auto.o

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp test_env_util test_logger *_auto.c *_auto.h

.PHONY: clean
