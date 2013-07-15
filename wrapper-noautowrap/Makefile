CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=4 -I..
LDLIBS:=-ldl -lhdf5 -lc
HDFFS_OBJ:=../real_func_auto.o ../logger.o ../process_info.o ../hfile_ds.o ../chunksize.o ../hdir.o ../path_util.o ../hstack_tree.o ../env_util.o

wrapper_func_auto.o: CFLAGS:=$(CFLAGS) -Wno-unused-variable -Wno-unused-label -Wno-unused-but-set-variable
# h5fs.o: CFLAGS:=$(CFLAGS) -ULOG_LEVEL -DLOG_LEVEL=5

all: h5fs-wrapper.so

h5fs-wrapper.so: ../wrapper_func.o wrapper_func_auto.o $(HDFFS_OBJ) ../h5fs.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
real_func_auto.c real_func_auto.h wrapper_func_auto.c: ../wrapper-gen/wrapper-gen.pl ../wrapper-gen/io-calls.c ../wrapper_func.c
	../wrapper-gen/wrapper-gen.pl --noautowrap ../wrapper-gen/io-calls.c ../wrapper_func.c

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp test_env_util test_logger *_auto.c *_auto.h

.PHONY: clean
