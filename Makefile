#
# Copyright (c) 2013-2015 Henning Glawe <glaweh@debian.org>
#
# This file is part of hdf5fs-wrapper.
#
# hdf5fs-wrapper is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# hdf5fs-wrapper is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with hdf5fs-wrapper.  If not, see <http://www.gnu.org/licenses/>.
#
-include Makefile.inc
CC:=gcc
CFLAGS:=$(CFLAGS) -std=gnu99 -fpic -g -O2 -Wall -Werror

PREFIX?=/usr/local
WRAPPER_GEN_FLAGS?= #possible values: --noautowrap, --comparewrap
LOG_LEVEL?=4

ifeq ($(strip $(HDF5_LIBS)),)
HDF5_CFLAGS := $(shell pkg-config hdf5 --cflags 2>/dev/null)
HDF5_LIBS   := $(shell pkg-config hdf5 --libs)
endif

ifeq ($(strip $(SSL_LIBS)),)
SSL_CFLAGS  := $(shell pkg-config libssl --cflags 2>/dev/null)
SSL_LIBS    := $(shell pkg-config libssl --libs)
endif

ifneq ($(strip $(HARDCODE_PREFIX)),)
CFLAGS:=$(CFLAGS) -DHARDCODE_PREFIX=$(HARDCODE_PREFIX)
endif

CFLAGS:=$(CFLAGS) -DLOG_LEVEL=$(LOG_LEVEL) -DPREFIX=$(PREFIX) $(HDF5_CFLAGS)
LDLIBS:=-ldl $(HDF5_LIBS) -lc

# if DEBUG_TCMALLOC is 1, use tcmalloc to track possible memory leaks
# tcmalloc is part of "Google Performance Tools":
#   http://goog-perftools.sourceforge.net/
# currently, "DEBUG_TCMALLOC" is used in 'h5fs-repack', which traces/prints
#   heap usage
# Tracing heap usage within the wrapper itself is confused by the mallocs in
#   the wrapped code
ifeq ($(strip $(DEBUG_TCMALLOC)),1)
CFLAGS:=$(CFLAGS) -DDEBUG_TCMALLOC
LDLIBS:=-ltcmalloc $(LDLIBS)
LDLIBS_WRAPPER:=-ltcmalloc
endif


# stolen from: http://stackoverflow.com/questions/10858261/abort-makefile-if-variable-not-set
#   Macros which check defined-ness of variables, with user-defined error messages
check_defined = \
    $(strip $(foreach 1,$1, \
        $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
        $(error Undefined $1$(if $2, ($2))$(if $(value @), \
                required by target '$@')))


HDFFS_OBJ:=logger.o hfile_ds.o chunksize.o hdir.o path_util.o hstack_tree.o env_util.o

wrapper_libc_autogenerated.o: CFLAGS:=$(CFLAGS) -Wno-unused-variable -Wno-unused-label -Wno-unused-but-set-variable
# h5fs.o: CFLAGS:=$(CFLAGS) -ULOG_LEVEL -DLOG_LEVEL=5
h5fs-wrap: LDLIBS:=$(LDLIBS_WRAPPER)
h5fs-md5sum-size.o: CFLAGS:=$(CFLAGS) $(SSL_CFLAGS)

all: check_defined_libs h5fs-wrapper.so h5fs-repack h5fs-unpack h5fs-md5sum-size h5fs-wrap
check_defined_libs:
	@:$(call check_defined, SSL_LIBS, pkg-config libssl failed. please specify SSL_CFLAGS and SSL_LIBS manually)
	@:$(call check_defined, HDF5_LIBS, pkg-config hdf5 failed. please spcify HDF5_CFLAGS and HDF5_LIBS manually)

logger.o: logger.c logger.h
path_util.o: path_util.c path_util.h logger.h
wrapper_func.o: wrapper_func.c

h5fs-wrapper.so: wrapper_func.o wrapper_libc_autogenerated.o $(HDFFS_OBJ) h5fs.o wrapper_libmpi_io_abort.o
	gcc $(LDFLAGS) -shared -o $@ $^ $(LDLIBS)
wrapper_libc_autogenerated.c: wrapper-gen/wrapper-gen.pl wrapper-gen/io-calls.c wrapper_func.c
	./wrapper-gen/wrapper-gen.pl $(WRAPPER_GEN_FLAGS) wrapper-gen/io-calls.c wrapper_func.c

h5fs-repack: h5fs-repack.o logger.h $(HDFFS_OBJ)
h5fs-unpack: h5fs-unpack.o logger.h $(HDFFS_OBJ)
h5fs-md5sum-size:  h5fs-md5sum-size.o logger.h $(HDFFS_OBJ)
	gcc $(LDFLAGS) -o $@ $^ $(LDLIBS) $(SSL_LIBS)
h5fs-wrap: h5fs-wrap.o logger.o path_util.o

test_rel2abs:    test_rel2abs.o path_util.o     logger.o
test_env_util:   test_env_util.o env_util.o     logger.o
test_pathcmp:    test_pathcmp.o path_util.o     logger.o
test_logger:     test_logger.o                  logger.o
exp_truncate_setextent: exp_truncate_setextent.o logger.o

test_h5fs_01_hfile_ds.o: test_h5fs_01_hfile_ds.c hfile_ds.h
test_h5fs_01_hfile_ds: test_h5fs_01_hfile_ds.o hfile_ds.o logger.o chunksize.o path_util.o

install: all
	cp -p h5fs-wrapper.so $(PREFIX)/lib/
	cp -p h5fs-repack h5fs-unpack h5fs-md5sum-size h5fs-wrap $(PREFIX)/bin/

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp test_env_util test_logger test_h5fs_01_hfile_ds wrapper_libc_autogenerated* h5fs-repack h5fs-unpack h5fs-md5sum-size h5fs-wrap

.PHONY: clean install check_defined_libs
