#ifndef CLOSED_EMPTY_H
#define CLOSED_EMPTY_H
#define CLOSED_EMPTY_MAX 1024*64
typedef struct {
    int nitems;
    char * items[CLOSED_EMPTY_MAX];
} string_set;
int  string_set_add(string_set * self, const char *pathname);
int  string_set_remove(string_set * self, const char *pathname);
int  string_set_find(string_set * self, const char *pathname);
int  string_set_free(string_set * self);
void string_set_dump(string_set * self);
#endif
