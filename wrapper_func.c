#include "real_func_auto.h"
#include "wrapper_func.h"
#include "logger.h"
#include "path_util.h"
#include <stdarg.h>

khash_t(WFILE) * wrapper_files;
khash_t(WFD)   * wrapper_fds;
char scratch_base[PATH_MAX] = "./SCRATCH";
char scratch_abs[PATH_MAX];

void __attribute__ ((constructor(300))) wrapper_func_init(void) {
    wrapper_files = kh_init(WFILE);
    wrapper_fds   = kh_init(WFD);
    char * env_ptr;
    env_ptr=getenv("SCRATCH_BASE");
    if (env_ptr != NULL) {
        strncpy(scratch_base,env_ptr,PATH_MAX);
    }
    rel2abs(scratch_base,scratch_abs);
    LOG_INFO("scratch_abs: '%s'",scratch_abs);
}

void __attribute__ ((destructor(300)))  wrapper_func_fini(void) {
    kh_destroy(WFILE,wrapper_files);
    kh_destroy(WFD,wrapper_fds);
}

int path_below_scratch(const char *filename, char *mapped) {
    char mapped0[PATH_MAX];
    rel2abs(filename,mapped0);
    int match_index = pathcmp(scratch_abs,mapped0);
    if (match_index < 0) return(-1);
    strcpy(mapped,mapped0+match_index);
    LOG_DBG("%d == pathcmp('%s','%s'), base: '%s'",
            match_index,scratch_abs,mapped,
            mapped);
    return(1);
}

