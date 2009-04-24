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
void processfiles(const char *const *names);

#endif /* ndef process_h */

