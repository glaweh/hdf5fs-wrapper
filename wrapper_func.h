#ifndef WRAPPER_FUNC_H
#define WRAPPER_FUNC_H
#include "h5fs.h"
#include "khash.h"
#ifdef __LP64__
KHASH_MAP_INIT_INT64(WFILE,h5fd_t*)
KHASH_MAP_INIT_INT64(WDIR,h5dd_t*)
#define PTR2INT int64_t
#else
KHASH_MAP_INIT_INT(WFILE,h5fd_t*)
KHASH_MAP_INIT_INT(WDIR,h5dd_t*)
#define PTR2INT int
#endif
KHASH_MAP_INIT_INT(WFD,h5fd_t*)

extern khash_t(WFILE) * wrapper_files;
extern khash_t(WFD)   * wrapper_fds;
extern khash_t(WDIR)  * wrapper_dirs;

char* path_below_scratch(const char *filename);
int fopen_mode2open_flags(const char * mode);
#endif
