/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#ifndef misc_h
#define misc_h
#include <stdarg.h>
#include <stdlib.h>

void *memalloc(size_t size);
void *memrealloc(void *ptr, size_t size);
void memfree(void *ptr);

void vlocerrorexit(const char *filename, unsigned int linenum, const char *format, va_list ap);
void locerrorexit(const char *filename, unsigned int linenum, const char *format, ...);
void errorexit(const char *format, ...);

#endif /* ndef misc_h */

