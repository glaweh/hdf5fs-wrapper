#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "closed_empty_files.h"
int  n_closed_empty_files=0;
char * closed_empty_files[CLOSED_EMPTY_MAX];
int _closed_empty_add(const char *pathname) {
    int insert_pos=0;
    if (n_closed_empty_files>=CLOSED_EMPTY_MAX-1) {
        fprintf(stderr,"_closed_empty_add: too many closed empty files\n");
        return(0);
    }
    if (n_closed_empty_files!=0) {
        int left=0;
        int right=n_closed_empty_files-1;
        while (right>=left) {
            int middle=(right+left)/2;
//    fprintf(stderr,"_closed_empty_add: middle %d %d %d (%d)\n",middle,left,right,n_closed_empty_files);
            int cmpres=strcmp(closed_empty_files[middle],pathname);
//    fprintf(stderr,"_closed_empty_add: cmp '%s' '%s',  %d\n",closed_empty_files[middle],pathname,cmpres);
            if (cmpres==0)
                return(1);
            if (cmpres > 0) {
                right=middle-1;
            } else {
                left=middle+1;
            }
        }
        insert_pos=left;
    }

    fprintf(stderr,"_closed_empty_add: adding '%s'\n",pathname);
    if (insert_pos < n_closed_empty_files) {
        int i;
        for (i=n_closed_empty_files;i>insert_pos;i--) {
            closed_empty_files[i]=closed_empty_files[i-1];
        }
    }
    closed_empty_files[insert_pos]=malloc(strlen(pathname)+1);
    if (closed_empty_files[insert_pos]==NULL) {
        fprintf(stderr,"_closed_empty_add: malloc error\n");
        return(0);
    }
    strcpy(closed_empty_files[insert_pos],pathname);
    n_closed_empty_files++;
    return(1);
}
int _closed_empty_find(const char *pathname) {
    int item_pos=-1;
    if (n_closed_empty_files == 0)
        return(-1);
    int left=0;
    int right=n_closed_empty_files-1;
    while (right>=left) {
        int middle=(right+left)/2;
        int cmpres=strcmp(closed_empty_files[middle],pathname);
        if (cmpres==0) {
            item_pos=middle;
            break;
        }
        if (cmpres > 0) {
            right=middle-1;
        } else {
            left=middle+1;
        }
    }
    return(item_pos);
}
int _closed_empty_remove(const char *pathname) {
    int delete_pos = _closed_empty_find(pathname);
    if (delete_pos < 0)
        return(0);
    int i;
    free(closed_empty_files[delete_pos]);
    for (i=delete_pos;i<(n_closed_empty_files-1);i++)
        closed_empty_files[i]=closed_empty_files[i+1];
    n_closed_empty_files--;
    return(1);
}
void _closed_empty_dump() {
    int i;
    for (i=0;i<n_closed_empty_files;i++) {
        fprintf(stderr,"_closed_empty_dump: %s\n",closed_empty_files[i]);
    }
}
int _closed_empty_free() {
    int i;
    for (i=0;i<n_closed_empty_files;i++) {
        if (closed_empty_files[i]!=NULL) {
            free(closed_empty_files[i]);
        }
    }
    return(1);
}
