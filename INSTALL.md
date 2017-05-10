## Build instructions
### Prerequisits

 - perl >= 5.8
 - GNU C Compiler gcc
 - GNU Make
 - HDF5 library (1.8) and development/header files, compiled for serial
   usage (i.e. without built-in MPI support)
 - OpenSSL and development/header files


### Build

Simply running
```sh
make
```
works on most Linux systems. In case 'pkg-config' is not available or not
configured for hdf5 and openssl, you need to adapt the Makefile manually.
