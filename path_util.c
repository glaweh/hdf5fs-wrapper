/*
 * Copyright (c) 2013 Henning Glawe <glaweh@debian.org>
 *
 * This file is part of hdf5fs-wrapper.
 *
 * hdf5fs-wrapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hdf5fs-wrapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with hdf5fs-wrapper.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "logger.h"
#include "path_util.h"


char *rel2abs(const char *orig_path, char *new_path) {
    char tmp_path[PATH_MAX];
    if (orig_path[0] == '/') {
        strncpy(tmp_path,orig_path,PATH_MAX);
    } else {
        if (getcwd(tmp_path,PATH_MAX) == NULL) {
            LOG_WARN("error calling getwd");
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
            dot_count=0;
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
        while ((*end_block_pattern!=0) && (*end_block_pattern!='/')) {
            if (*end_block_pattern=='*')
                pattern_asterisk++;
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
        } else {
            if (len_block_test < (len_block_pattern-pattern_asterisk)) {
                // '*' expands to >=0 characters
                return(-1);
            } else if (len_block_pattern == 1) {
                // simple case: the only char in pattern is '*'
            } else {
                // compare up first asterisk, pointers will point at it after
                while (*pattern_path != '*') {
                    if ((*pattern_path != *test_path) && (*pattern_path != '?'))
                        return(-1);
                    pattern_path++;
                    test_path++;
                }
                // compare part after last asterisk if there is any
                const char *end_tmp_pattern=end_block_pattern;
                const char *end_tmp_test=end_block_test;
                while (*(end_tmp_pattern-1) != '*') {
                    LOG_DBG3("cmp: %c %c",*(end_tmp_pattern-1),*(end_tmp_test-1));
                    if ((*(end_tmp_pattern-1)!=*(end_tmp_test-1)) && (*(end_tmp_pattern-1)!='?')) {
                        return(-1);
                    }
                    end_tmp_pattern--;
                    end_tmp_test--;
                }
                int pattern_len = (end_tmp_pattern-pattern_path);
                int test_len = (end_tmp_test-test_path);
                // single asterisk will match independently of test length
                if (pattern_len > 1) {
                    //strip tailing asterisks
                    while (*(end_tmp_pattern-1) == '*') {
                        pattern_asterisk--;
                        pattern_len--;
                        end_tmp_pattern--;
                    }
#if (LOG_LEVEL >= LOG_DBG3)
                    char tmp_str[PATH_MAX];
                    strncpy(tmp_str,pattern_path,pattern_len);
                    tmp_str[pattern_len]=0;
                    LOG_DBG3("to_cmp pat: (%d) '%s'", pattern_len,tmp_str);
                    strncpy(tmp_str,test_path,test_len);
                    tmp_str[test_len]=0;
                    LOG_DBG3("to_cmp tst: (%d) '%s'", test_len,tmp_str);
#endif
                    if (test_len < (pattern_len-pattern_asterisk))
                        return(-1);
                    // now search for the floating parts
                    int j=0;
                    while (test_path < end_tmp_test) {
                        if (*pattern_path == '*') {
                            j=0;
                            pattern_path++;
                        }
                        LOG_DBG3("cmp p/t: %c %c",*pattern_path,*test_path);
                        if ((*pattern_path == *test_path) || (*pattern_path == '?')) {
                            pattern_path++;
                            j++;
                            if (pattern_path == end_tmp_pattern) break;
                        } else {
                            pattern_path-=j;
                        }
                        test_path++;
                    }
                    if (pattern_path != end_tmp_pattern)
                        return(-1);
                }
            }
            test_path=end_block_test;
            pattern_path=end_block_pattern;
        }
        // step beyond the terminating '/'
        test_path++;
        pattern_path++;
    }
    if (*pattern_path == 0) return(test_path - test_path_base);
    return(-1);
}


int mkpath(const char * path) {
    struct stat dirstat;
    // short circuit: path exists
    if (stat(path,&dirstat) >= 0) {
        if (S_ISDIR(dirstat.st_mode)) {
            return(1);
        } else {
            LOG_ERR("'%s' exists and is not a directory",path);
            return(0);
        }
    }
    char work_path[PATH_MAX];
    strcpy(work_path,path);
    int path_len=strlen(path);
    if ((path_len > 0) && (work_path[path_len-1] != '/')) {
        work_path[path_len]='/';
        work_path[path_len+1]=0;
        path_len++;
    }
    int i;
    for (i=0; i < path_len; i++) {
        if (work_path[i]=='/') {
            work_path[i]=0;
            if (stat(work_path,&dirstat) >= 0) {
                if (! S_ISDIR(dirstat.st_mode)) {
                    LOG_ERR("'%s' exists and is not a directory",work_path);
                    return(0);
                }
            } else {
                if (mkdir(work_path,0770) < 0) {
                    LOG_ERR("error creating directory '%s'",work_path);
                    return(0);
                }
            }
            work_path[i]='/';
        }
    }
    return(1);
}
