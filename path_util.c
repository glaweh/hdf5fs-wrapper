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
    int had_asterisk=0;
    int dot_count=0;
    for (i=0,j=-1;i<=len;i++) {
        if (tmp_path[i] != '*') had_asterisk=0;
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
        } else if (tmp_path[i] == '*') {
            if (had_asterisk) continue;
            had_slash=0;
            had_asterisk=1;
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
    const char *test_path_base = test_path;
    while ((*test_path) && (*pattern_path)) {
        // search for next '/' in test path
        const char *end_block_test=test_path;
        while ((*end_block_test!=0) && (*end_block_test!='/'))
            end_block_test++;
        int len_block_test = end_block_test - test_path;
        // search for next '/' in pattern path, check for asterisks
        const char *end_block_pattern=pattern_path;
        int pattern_asterisk=0;
        int first_asterisk=-1;
        while ((*end_block_pattern!=0) && (*end_block_pattern!='/')) {
            if (*end_block_pattern=='*') {
                pattern_asterisk++;
                if (first_asterisk<0) first_asterisk=end_block_pattern-pattern_path;
            }
            end_block_pattern++;
        }
        int len_block_pattern=end_block_pattern-pattern_path;
        if ((len_block_test == 0) && (len_block_pattern !=0))
            return(-1);
        if (pattern_asterisk==0) {
            // simple case, no asterisks.
            //   path components of differnt length cannot match
            if (len_block_test != len_block_pattern) return(-1);
            //   compare char-by-char. pointers will point to the blocks final '/'
            while (test_path < end_block_test) {
                if ((*pattern_path != *test_path) && (*pattern_path != '?'))
                    return(-1);
                test_path++;
                pattern_path++;
            }
        } else if (pattern_asterisk==1) {
            if (len_block_pattern == 1) {
                // simple case: the only char in pattern is '*'
            } else if (len_block_test < (len_block_pattern-1)) {
                // '*' expands to >=0 characters
                return(-1);
            } else {
                int i,j;
                for (i=0;i<first_asterisk;i++) {
                    if ((test_path[i]!=pattern_path[i]) && (pattern_path[i]!='?'))
                        return(-1);
                }
                for (i=first_asterisk+1,j=len_block_test-first_asterisk;i<len_block_pattern;i++,j++) {
                    if ((test_path[j]!=pattern_path[i]) && (pattern_path[i]!='?'))
                        return(-1);
                }
            }
            test_path=end_block_test;
            pattern_path=end_block_pattern;
        } else {
            fprintf(stderr,"pathcmp unimplemented for more than 1 '*' per path component\n");
            return(-1);
        }
        test_path++;
        pattern_path++;
    }
    if (*pattern_path == 0) return(test_path - test_path_base);
    return(-1);
}
