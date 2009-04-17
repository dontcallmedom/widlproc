/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#ifndef comment_h
#define comment_h

struct tok;
struct node;

void addcomment(struct tok *tok);
void setidentifier(struct node *node2);
void processcomments(struct node *root);
void outputdescriptive(struct node *node, unsigned int indent);

#endif /* ndef comment_h */
