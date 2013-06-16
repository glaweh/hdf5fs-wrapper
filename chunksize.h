#ifndef CHUNKSIZE_H
#define CHUNKSIZE_H
#include <hdf5.h>
extern hsize_t chunksize_default;
hsize_t chunksize_suggest(const char * name, const hsize_t expected_length);
#endif
