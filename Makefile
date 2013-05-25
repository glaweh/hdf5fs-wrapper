CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable
LDLIBS:=-ldl

all: espresso-io-preload.so
test: test_rel2abs

espresso-io-preload.so: espresso-io-preload.o
	$(LD) $(LDFLAGS) -shared $^ -ldl -o $@

test_rel2abs: test_rel2abs.o espresso-io-preload.o

clean:
	rm -f *.o *.so test_rel2abs

.PHONY: clean
