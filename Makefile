CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DDEBUG
LDLIBS:=-ldl -lhdf5_hl -lhdf5 -lc

all: hdf5fs-wrapper.so
test: test_rel2abs test_pathcmp

hdf5fs-wrapper.so: hdf5fs-wrapper.o path_util.o hdf5_fs.o string_set.o env_util.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@

test_rel2abs: test_rel2abs.o path_util.o
test_env_util: test_env_util.o env_util.o
test_pathcmp: test_pathcmp.o path_util.o
test_string_set: test_string_set.o string_set.o

clean:
	rm -f *.o *.so test_rel2abs test_pathcmp test_string_set

.PHONY: clean
