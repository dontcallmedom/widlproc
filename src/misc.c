/***********************************************************************
 * $Id$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
    /* Zero initialise memory from memalloc. */
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

