CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DDEBUG
LDLIBS:=-ldl

all: espresso-io-preload.so
test: test_rel2abs test_pathcmp

espresso-io-preload.so: espresso-io-preload.o path_util.o
	$(LD) $(LDFLAGS) -shared $^ -ldl -o $@

test_rel2abs: test_rel2abs.o path_util.o
test_pathcmp: test_pathcmp.o path_util.o

clean:
	rm -f *.o *.so test_rel2abs

.PHONY: clean
