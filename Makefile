CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=4
LDLIBS:=-ldl -lhdf5 -lc

all: hdf5fs-wrapper.so hdf5fs-repack hdf5fs-unpack
test: test_rel2abs test_pathcmp

hdf5fs-wrapper.so: hdf5fs-wrapper.o path_util.o hdf5_fs.o string_set.o env_util.o logger.o process_info.o file_ds.o chunksize.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
hdf5fs-wrapper.o: hdf5fs-wrapper.c logger.h process_info.h

hdf5fs-repack: hdf5fs-repack.o logger.h process_info.h logger.o process_info.o file_ds.o chunksize.o
hdf5fs-unpack: hdf5fs-unpack.o logger.h process_info.h logger.o process_info.o file_ds.o chunksize.o

test_rel2abs:    test_rel2abs.o path_util.o     logger.o process_info.o
test_env_util:   test_env_util.o env_util.o     logger.o process_info.o
test_pathcmp:    test_pathcmp.o path_util.o     logger.o process_info.o
test_string_set: test_string_set.o string_set.o logger.o process_info.o
test_logger:     test_logger.o                  logger.o process_info.o

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp test_string_set

.PHONY: clean
