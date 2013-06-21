#ifndef _PATH_UTIL_H
#define _PATH_UTIL_H
char *rel2abs(const char *orig_path, char *new_path);
int   pathcmp(const char *pattern_path,const char *test_path);
int   mkpath(const char * path);
#endif
