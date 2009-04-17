/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#include <string.h>
#include "misc.h"
#include "process.h"

const char *progname;

/***********************************************************************
 * options : process command line options
 *
 * Enter:   argv
 *
 * Return:  argv stepped to point to first non-option argument
 */
static const char *const *
options(const char *const *argv)
{
    /* Set progname for error messages etc. */
    {
        const char *base;
        progname = argv[0];
        base = strrchr(progname, '/');
#ifdef DIRSEP
        {
            const char *base2 = strrchr(base, '\\');
            if (base2 > base)
                base = base2;
        }
#endif /* def DIRSEP */
        if (base)
            progname = base + 1;
    }
    return argv + 1;
}

/***********************************************************************
 * main : main code for bondiidl command
 */
int
main(int argc, char **argv)
{
    const char *const *parg;
    const char *arg;
    parg = options((const char *const *)argv);
    if (!*parg)
        errorexit("expected at least one argument");
    while ((arg = *parg++) != 0)
        processfile(arg);
    return 0;
}

