#ifndef WRAPPER_FUNC_H
#define WRAPPER_FUNC_H

#include "khash.h"
#ifdef __LP64__
KHASH_MAP_INIT_INT64(WFILE,int)
#else
KHASH_MAP_INIT_INT(WFILE,int)
#endif
KHASH_MAP_INIT_INT(WFD,int)

extern khash_t(WFILE) * wrapper_files;
extern khash_t(WFD)   * wrapper_fds;

char* path_below_scratch(const char *filename);
#endif
