#ifndef WRAPPER_LIBC_H
#define WRAPPER_LIBC_H

#define _GNU_SOURCE
#define __USE_LARGEFILE64
#include <dirent.h>
#include <libio.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wchar.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>

#define PATHNAME char*
#define FD int

#endif
