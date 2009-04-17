/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

extern const char *progname;

/***********************************************************************
 * memory allocation wrappers
 */
void *
memalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
        errorexit("out of memory");
    memset(ptr, 0, size);
    return ptr;
}

void *
memrealloc(void *ptr, size_t size)
{
    void *newptr = realloc(ptr, size);
    if (!newptr)
        errorexit("out of memory");
    return newptr;
}

void
memfree(void *ptr)
{
    *(int *)ptr = 0xfefefefe;
    free(ptr);
}

/***********************************************************************
 * errorexit : print error message then exit
 */
void
vlocerrorexit(const char *filename, unsigned int linenum,
        const char *format, va_list ap)
{
    if (filename)
        fprintf(stderr, linenum ? "%s: %i: " : "%s: ", filename, linenum);
    vfprintf(stderr, format, ap);
    fputc('\n', stderr);
    exit(1);
}

void
locerrorexit(const char *filename, unsigned int linenum,
        const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(filename, linenum, format, ap);
    va_end(ap);
}

void
errorexit(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(0, 0, format, ap);
    va_end(ap);
}

