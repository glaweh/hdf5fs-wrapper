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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "env_util.h"
#include "logger.h"
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
                    LOG_DBG("strn_env_expand: escape error");
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
                LOG_DBG("strn_env_expand: string length plausibility error");
                return(-1);
            }
            if ((*(input+1)) != '{') {
                LOG_DBG("missing '{' error");
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
                LOG_DBG("empty var name");
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
                                    LOG_DBG("def_val escape error");
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
            LOG_DBG("var: '%s', fstring: '%s', def_val: '%s'",env_name,fstring,value);
            char * env_val = getenv(env_name);
            if ((env_val == NULL) && (have_default == 0)) {
                LOG_DBG("neither var available nor default value set");
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
                                LOG_DBG("number parse error");
                                return(-1);
                            }
                            splen=sprintf(output,fstring,vali);
                            break;
                        case 's':
                            splen=sprintf(output,fstring,value);
                            break;
                        default:
                            LOG_DBG("not implemented: '%s'",fstring);
                            return(-1);
                    }
                    if (splen < 0) {
                        LOG_DBG("sprintf error");
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
