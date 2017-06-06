## Build instructions
### Prerequisits

 - perl >= 5.8
 - GNU C Compiler gcc
 - GNU Make
 - HDF5 library (1.8) and development/header files, compiled for serial
   usage (i.e. without built-in MPI support)
 - OpenSSL and development/header files
 - exuberant ctags (http://ctags.sourceforge.net/)


### Build

Simply running
```sh
make
```
works on most Linux systems. In case 'pkg-config' is not available or not
configured for hdf5 and openssl, you need to specify related make variables
manually. This can be done via 'make' command line, or in a file
'Makefile.inc':
* HDF5_CFLAGS: flags for the C compiler (e.g. include paths) to compile
               code using HDF5
* HDF5_LIBS:   flags for the linker (e.g. library paths and linked libraries)
               to link objects using HDF5
* SSL_CFLAGS:  flags for the C compiler (e.g. include paths) to compile
               code using OpenSSL
* SSL_LIBS:    flags for the linker (e.g. library paths and linked libraries)
               to link objects using OpenSSL
Further make variables available:
* PREFIX:      Prefix, under which hdf5fs-wrapper will be installed (default: /usr/local)
* HARDCODE_PREFIX:
               Path to override the automatic detection mechanism for h5fs-wrapper.so
               (default search path is '.', '../lib', relative to h5fs-wrap)
