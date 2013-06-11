#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "env_util.h"
int strn_env_expand(const char * input, char * output, int length) {
    const char *input_end = input+strnlen(input,length);
    int had_backslash = 0;
    while (input <= input_end)  {
        if (had_backslash) {
            switch (*input) {
                case '\\':
                case '$':
                    (*output)=(*input);
                default:
#ifdef DEBUG_ENV_UTIL
                    fprintf(stderr,"strn_env_expand: escape error\n");
#endif
                    return(-1);
            }
            had_backslash=0;
        } else if ((*input) == '\\') {
            had_backslash=1;
            input++;
            continue;
        } else if ((*input) != '$') {
            (*output) = (*input);
        } else {
            // substitute environment variable
            // syntax ${name[:[fstring][:default]]}
            // need at least ${A}
            if ((input_end - input) < 4) {
#ifdef DEBUG_ENV_UTIL
                fprintf(stderr,"strn_env_expand: string length plausibility error\n");
#endif
                return(-1);
            }
            if ((*(input+1)) != '{') {
#ifdef DEBUG_ENV_UTIL
                fprintf(stderr,"strn_env_expand: missing '{' error\n");
#endif
                return(-1);
            }
            input+=2;

            char env_name[length];
            env_name[0]=0;
            char fstring[length];
            fstring[0]=0;
            char value[length];
            value[0]=0;
            int have_default = 0;
            int offset;
            offset=0;
            while ((input <= input_end) && ((*input) != '}') && (*input != ':')) {
                env_name[offset]=(*input);
                input++;
                offset++;
            }
            env_name[offset]=0;
            if (env_name[0]==0) {
#ifdef DEBUG_ENV_UTIL
                fprintf(stderr,"strn_env_expand: empty var name\n");
#endif
                return(-1);
            }
            if ((input<input_end) && ((*input) == ':')) {
                input++;
                offset=0;
                while ((input <= input_end) && ((*input) != '}') && (*input != ':')) {
                    fstring[offset]=(*input);
                    input++;
                    offset++;
                }
                fstring[offset]=0;
                if ((input<=input_end) && ((*input) == ':')) {
                    have_default = 1;
                    input++;
                    //maybe go to recursion
                    int def_had_backslash=0;
                    offset=0;
                    while (input<=input_end) {
                        if (def_had_backslash) {
                            switch (*input) {
                                case '\\':
                                case '}':
                                    value[offset]=(*input);
                                    def_had_backslash=0;
                                    break;
                                default:
#ifdef DEBUG_ENV_UTIL
                                    fprintf(stderr,"strn_env_expand: def_val escape error\n");
#endif
                                    return(-1);
                            }
                        } else if ((*input) == '\\') {
                            def_had_backslash = 1;
                            input++;
                            continue;
                        } else if ((*input) == '}') {
                            break;
                        } else {
                            value[offset]=(*input);
                        }
                        offset++;
                        input++;
                    }
                }
                value[offset]=0;
            }
#ifdef DEBUG_ENV_UTIL
            fprintf(stderr,"var: '%s', fstring: '%s', def_val: '%s'\n",env_name,fstring,value);
#endif
            char * env_val = getenv(env_name);
            if ((env_val == NULL) && (have_default == 0)) {
#ifdef DEBUG_ENV_UTIL
                fprintf(stderr,"neither var available nor default value set\n");
#endif
                return(-1);
            }
            if (env_val != NULL) strcpy(value,env_val);
            if (value[0]!=0) {
                if (fstring[0] == 0) {
                    strcpy(output,value);
                    output+=strlen(value)-1;
                } else {
                    int lenf = strlen(fstring);
                    int vali;
                    int splen=0;
                    char * endptr = value;
                    switch (fstring[lenf-1]) {
                        case 'd':
                            vali=strtol(value,&endptr,10);
                            if ((*endptr) != 0) {
#ifdef DEBUG_ENV_UTIL
                                fprintf(stderr,"number parse error\n");
#endif
                                return(-1);
                            }
                            splen=sprintf(output,fstring,vali);
                            break;
                        case 's':
                            splen=sprintf(output,fstring,value);
                            break;
                        default:
#ifdef DEBUG_ENV_UTIL
                            fprintf(stderr,"not implemented: '%s'\n",fstring);
#endif
                            return(-1);
                    }
                    if (splen < 0) {
#ifdef DEBUG_ENV_UTIL
                        fprintf(stderr,"sprintf error\n");
#endif
                        return(-1);
                    }
                    output+=splen-1;
                }
            }
        }
        input++;
        output++;
    }
    return(1);
}
