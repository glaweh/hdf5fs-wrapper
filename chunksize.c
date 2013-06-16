#include "chunksize.h"
hsize_t chunksize_default = 64 * 1024;
hsize_t chunksize_suggest(const char * name,const hsize_t expected_length) {
    if (expected_length > 0) {
        if (expected_length < 16*1024) {
            return(2*1024);
        } else if (expected_length < 32*1024) {
            return(4*1024);
        } else if (expected_length < 128*1024) {
            return(64*1024);
        } else if (expected_length < 10*1024*1024) {
            return(256*1024);
        } else {
            return(1024*1024);
        }
    }
    return(chunksize_default);
}
