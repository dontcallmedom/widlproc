/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#ifndef os_h
#define os_h

/* Linux configuration */
#if defined(__gnu_linux__)


/* Windows configuration */
#elif defined(_MSC_VER)

#define inline __inline
#define strncasecmp strnicmp
#define snprintf _snprintf

#endif

#endif /* ndef os_h */
