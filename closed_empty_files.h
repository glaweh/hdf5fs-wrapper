#ifndef CLOSED_EMPTY_H
#define CLOSED_EMPTY_H
#define CLOSED_EMPTY_MAX 1024*64
int _closed_empty_add(const char *pathname);
int _closed_empty_remove(const char *pathname);
int _closed_empty_find(const char *pathname);
int _closed_empty_free();
void _closed_empty_dump();
#endif
