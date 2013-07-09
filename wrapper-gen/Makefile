CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=4
LDLIBS:=-ldl -lc

all: io-wrapper.so

io-wrapper.so: io-wrapper.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
io-wrapper.o: io-wrapper.c
io-wrapper.c: io-calls.c wrapper-gen.pl
	./wrapper-gen.pl

clean:
	rm -f *.o *.so io-wrapper.c

.PHONY: clean
