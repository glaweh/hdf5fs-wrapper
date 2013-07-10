CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=4
LDLIBS:=-ldl -lhdf5 -lc
HDFFS_OBJ:=wrapper-gen/real_func_auto.o logger.o process_info.o hfile_ds.o chunksize.o hdir.o path_util.o hstack_tree.o

all: hdf5fs-wrapper.so hdf5fs-repack hdf5fs-unpack
test: test_rel2abs test_pathcmp

hdf5fs-wrapper.so: hdf5fs-wrapper.o path_util.o hdf5_fs.o env_util.o $(HDFFS_OBJ)
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
hdf5fs-wrapper.o: hdf5fs-wrapper.c logger.h process_info.h

hdf5fs-repack: hdf5fs-repack.o logger.h process_info.h $(HDFFS_OBJ)
hdf5fs-unpack: hdf5fs-unpack.o logger.h process_info.h $(HDFFS_OBJ)

test_rel2abs:    test_rel2abs.o path_util.o     logger.o process_info.o
test_env_util:   test_env_util.o env_util.o     logger.o process_info.o
test_pathcmp:    test_pathcmp.o path_util.o     logger.o process_info.o
test_logger:     test_logger.o                  logger.o process_info.o

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp

.PHONY: clean
