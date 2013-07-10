#ifndef WRAPPER_FUNC_H
#define WRAPPER_FUNC_H

#include "khash.h"
#ifdef __LP64__
KHASH_MAP_INIT_INT64(WFILE,int)
KHASH_MAP_INIT_INT64(WDIR,int)
#define PTR2INT int64_t
#else
KHASH_MAP_INIT_INT(WFILE,int)
KHASH_MAP_INIT_INT(WDIR,int)
#define PTR2INT int
#endif
KHASH_MAP_INIT_INT(WFD,int)

extern khash_t(WFILE) * wrapper_files;
extern khash_t(WFD)   * wrapper_fds;
extern khash_t(WDIR)  * wrapper_dirs;

char* path_below_scratch(const char *filename);
#endif
