CC:=colorgcc
CFLAGS:=$(CFLAGS) -fpic -g -O0 -Wall -Werror -Wno-error=unused-variable
LDLIBS:=-ldl

all: espresso-io-preload.so
test: test_sanitize_path

espresso-io-preload.so: espresso-io-preload.o
	$(LD) $(LDFLAGS) -shared $^ -ldl -o $@

test_sanitize_path: test_sanitize_path.o espresso-io-preload.o

clean:
	rm -f *.o *.so test_sanitize_path

.PHONY: clean
