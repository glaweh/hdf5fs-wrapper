CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable -DLOG_LEVEL=5
LDLIBS:=-ldl -lc

all: io-wrapper.so

io-wrapper.so: wrapper_func_auto.o real_func_auto.o ../logger.o ../process_info.o
	$(LD) $(LDFLAGS) -shared $^ $(LDLIBS) -o $@
wrapper_func_auto.o: wrapper_func_auto.c
real_func_auto.o: real_func_auto.c
real_func_auto.c real_func_auto.h wrapper_func_auto.c: io-calls.c wrapper-gen.pl
	./wrapper-gen.pl

clean:
	rm -f *.o *.so io-wrapper.c

.PHONY: clean
