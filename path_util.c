#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include "path_util.h"
char *rel2abs(const char *orig_path, char *new_path) {
    char tmp_path[PATH_MAX];
    if (orig_path[0] == '/') {
        strncpy(tmp_path,orig_path,PATH_MAX);
    } else {
        if (getcwd(tmp_path,PATH_MAX) == NULL) {
            fprintf(stderr,"error calling getwd\n");
            return(NULL);
        }
        int len=strnlen(tmp_path,PATH_MAX);
        tmp_path[len++]='/';
        tmp_path[len]=0;
        strncpy(tmp_path+len,orig_path, PATH_MAX-len-1);
    }
    int len=strnlen(tmp_path,PATH_MAX);
    int i,j;
    int had_slash=0;
    int dot_count=0;
    for (i=0,j=-1;i<=len;i++) {
        if ((tmp_path[i] == '/') || (i==len)) {
            if (had_slash) {
                if (dot_count == 1) {
                    j--;
                } else if (dot_count == 2) {
                    j-=3;
                    while (new_path[j]!='/') {
                        j--;
                        if (j<0) return(NULL);
                    }
                } else if (dot_count > 2) {
                    return(NULL);
                }
                dot_count=0;
                continue;
            }
            had_slash = 1;
        } else if (tmp_path[i] == '.') {
            dot_count++;
        } else {
            had_slash = 0;
        }
        new_path[++j]=tmp_path[i];
    }
    if ((new_path[j]=='/') && (j > 0)) j--;
    new_path[j+1]=0;
    return new_path;
}

int pathcmp(const char *pattern_path,const char *test_path) {
    return(-1);
}
