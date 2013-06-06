#include <stdio.h>
int _closed_empty_add(const char * pathname);
int _closed_empty_remove(const char * pathname);
void _closed_empty_dump();
int _closed_empty_free();

int main(int argc,char * argv[]) {
    int i;
    for (i=1;i<argc-1;i++) {
        fprintf(stderr,"ARG %d: %s\n",i,argv[i]);
        _closed_empty_add(argv[i]);
        _closed_empty_dump();
    }
    for (i=2;i<argc;i++) {
        fprintf(stderr,"ARG %d: %s\n",i,argv[i]);
        _closed_empty_remove(argv[i]);
        _closed_empty_dump();
    }
    return(0);
}
