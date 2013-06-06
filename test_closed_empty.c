#include <stdio.h>
#include "closed_empty_files.h"

int main(int argc,char * argv[]) {
    int i;
    string_set check_it;
    check_it.nitems=0;
    for (i=1;i<argc-1;i++) {
        fprintf(stderr,"ARG %d: %s\n",i,argv[i]);
        string_set_add(&check_it,argv[i]);
        string_set_dump(&check_it);
    }
    for (i=2;i<argc;i++) {
        fprintf(stderr,"ARG %d: %s\n",i,argv[i]);
        string_set_remove(&check_it,argv[i]);
        string_set_dump(&check_it);
    }
    return(0);
}
