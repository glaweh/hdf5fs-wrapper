CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DDEBUG
LDLIBS:=-ldl -lhdf5_hl -lhdf5 -lc

all: espresso-io-preload.so
test: test_rel2abs test_pathcmp

espresso-io-preload.so: espresso-io-preload.o path_util.o hdf5_fs.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@

test_rel2abs: test_rel2abs.o path_util.o
test_pathcmp: test_pathcmp.o path_util.o

clean:
	rm -f *.o *.so test_rel2abs

.PHONY: clean
