/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#ifndef process_h
#define process_h

#define NT_START 0x100
#include "nonterminals.h"

/* struct node : a node in the parse tree (excluding comments) */
struct node {
    struct node *next;
    struct node *parent;
    struct node *children;
    struct comment *comments; /* list of comments attached to this node */
    unsigned int type;
    char name[1];
};

void printtext(const char *s, unsigned int len);
void processfile(const char *name);

#endif /* ndef process_h */

