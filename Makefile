CFLAGS:=$(CFLAGS) -fpic

all: espresso-io-preload.so

espresso-io-preload.so: espresso-io-preload.o
	$(LD) $(LDFLAGS) -shared $^ -ldl -o $@

clean:
	rm -f *.o *.so

.PHONY: clean
