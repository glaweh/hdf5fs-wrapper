CC:=gcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=4 `pkg-config hdf5 --cflags`
LDLIBS:=-ldl `pkg-config hdf5 --libs` -lc
ifeq ($(strip $(DEBUG_TCMALLOC)),1)
	CFLAGS:=$(CFLAGS) -DDEBUG_TCMALLOC
	LDLIBS:=-ltcmalloc $(LDLIBS)
	LDLIBS_WRAPPER:=-ltcmalloc
endif

ifeq ($(strip $(SOFT_BASE_OS)),x86_64-centos-6.3)
LDFLAGS:="-Wl,-rpath,/work/glawe/.software/other/arch/x86_64-centos-6.3/lib64"
endif

ifneq ($(strip $(SOFT_BASE_OS)),)
PREFIX:=$(HOME)/.software/other/arch/$(SOFT_BASE_OS)
else
PREFIX:=/usr/local
endif
HDFFS_OBJ:=real_func_auto.o logger.o process_info.o hfile_ds.o chunksize.o hdir.o path_util.o hstack_tree.o env_util.o

wrapper_func_auto.o: CFLAGS:=$(CFLAGS) -Wno-unused-variable -Wno-unused-label -Wno-unused-but-set-variable
# h5fs.o: CFLAGS:=$(CFLAGS) -ULOG_LEVEL -DLOG_LEVEL=5
h5fs-wrap: LDFLAGS:=
h5fs-wrap: LDLIBS:=$(LDLIBS_WRAPPER)
h5fs-wrap.o: CFLAGS:=$(CFLAGS) -DPREFIX="\"$(PREFIX)\""

all: h5fs-wrapper.so h5fs-repack h5fs-unpack h5fs-md5sum-size h5fs-wrap
test: test_h5fs_01_hfile_ds

logger.o: logger.c real_func_auto.h
process_info.o: process_info.c real_func_auto.h
path_util.o: path_util.c real_func_auto.h
wrapper_func.o: wrapper_func.c real_func_auto.h

h5fs-wrapper.so: wrapper_func.o wrapper_func_auto.o $(HDFFS_OBJ) h5fs.o
	gcc $(LDFLAGS) -shared -o $@ $^ $(LDLIBS)
real_func_auto.c real_func_auto.h wrapper_func_auto.c: wrapper-gen/wrapper-gen.pl wrapper-gen/io-calls.c wrapper_func.c
	./wrapper-gen/wrapper-gen.pl wrapper-gen/io-calls.c wrapper_func.c

h5fs-repack: h5fs-repack.o logger.h process_info.h $(HDFFS_OBJ)
h5fs-unpack: h5fs-unpack.o logger.h process_info.h $(HDFFS_OBJ)
h5fs-md5sum-size:  h5fs-md5sum-size.o logger.h process_info.h $(HDFFS_OBJ) -lssl
h5fs-wrap: h5fs-wrap.o

test_rel2abs:    test_rel2abs.o path_util.o     logger.o process_info.o
test_env_util:   test_env_util.o env_util.o     logger.o process_info.o
test_pathcmp:    test_pathcmp.o path_util.o     logger.o process_info.o
test_logger:     test_logger.o                  logger.o process_info.o

test_h5fs_01_hfile_ds.o: test_h5fs_01_hfile_ds.c hfile_ds.h
test_h5fs_01_hfile_ds: test_h5fs_01_hfile_ds.o hfile_ds.o logger.o process_info.o chunksize.o path_util.o real_func_auto.o

install: all
	cp -p h5fs-wrapper.so $(PREFIX)/lib/
	cp -p h5fs-repack h5fs-unpack h5fs-md5sum-size h5fs-wrap $(PREFIX)/bin/

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp test_env_util test_logger *_auto.c *_auto.h h5fs-repack h5fs-unpack h5fs-md5sum-size h5fs-wrap

.PHONY: clean install
