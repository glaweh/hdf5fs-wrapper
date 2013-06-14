#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_set.h"
#include "logger.h"
string_set * string_set_new() {
    string_set * self = malloc(sizeof(string_set));
    self->nitems=0;
    return(self);
}
int string_set_add(string_set * self,const char *pathname, void * attribute) {
    int insert_pos=0;
    if (self->nitems>=CLOSED_EMPTY_MAX-1) {
        LOG_WARN("too many items");
        return(0);
    }
    if (self->nitems!=0) {
        int left=0;
        int right=self->nitems-1;
        while (right>=left) {
            int middle=(right+left)/2;
            int cmpres=strcmp(self->items[middle]->string,pathname);
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

    LOG_DBG("adding '%s'",pathname);
    if (insert_pos < self->nitems) {
        int i;
        for (i=self->nitems;i>insert_pos;i--) {
            self->items[i]=self->items[i-1];
        }
    }
    self->items[insert_pos]=malloc(sizeof(string_set_member) + sizeof(char)*(strlen(pathname)+1));
    if (self->items[insert_pos]==NULL) {
        LOG_WARN("malloc error");
        return(0);
    }
    strcpy(self->items[insert_pos]->string,pathname);
    self->items[insert_pos]->attribute = attribute;
    self->nitems++;
    return(1);
}
int string_set_find(string_set * self,const char *pathname) {
    int item_pos=-1;
    if (self->nitems == 0)
        return(-1);
    int left=0;
    int right=self->nitems-1;
    while (right>=left) {
        int middle=(right+left)/2;
        int cmpres=strcmp(self->items[middle]->string,pathname);
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
int string_set_remove(string_set * self, const char *pathname) {
    int delete_pos = string_set_find(self,pathname);
    if (delete_pos < 0)
        return(0);
    int i;
    free(self->items[delete_pos]);
    for (i=delete_pos;i<(self->nitems-1);i++)
        self->items[i]=self->items[i+1];
    self->nitems--;
    return(1);
}
void string_set_dump(string_set * self) {
    int i;
    for (i=0;i<self->nitems;i++) {
        fprintf(stderr,"string_set_dump: %s\n",self->items[i]->string);
    }
}
int string_set_free(string_set * self) {
    int i;
    for (i=0;i<self->nitems;i++) {
        if (self->items[i]!=NULL) {
            free(self->items[i]);
        }
    }
    free(self);
    return(1);
}