#
# Copyright (c) 2013 Henning Glawe <glaweh@debian.org>
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
CC:=gcc
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
