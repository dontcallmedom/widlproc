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
 *
 * Program to convert Web IDL grammar cut'n'pasted from browser displaying
 * http://dev.w3.org/2006/webapi/WebIDL/
 * to C data structure for LL(1) parser in parse.c.
 ***********************************************************************/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct item {
    struct item *next;
    enum { ITEM_NONTERMINAL, ITEM_TOK, ITEM_STRING } type;
    struct nonterminal *nonterminal;
    char name[1];
};

struct prod {
    struct prod *next;
    struct item *itemlist;
    unsigned int offset;
};

struct initterm {
    struct initterm *next;
    struct prod *prod;
    struct item *item;
};

struct nonterminal {
    struct nonterminal *next;
    struct prod *prodlist;
    struct initterm *inittermlist;
    unsigned int namelen;
    int canbeempty;
    unsigned int offset;
    char name[1];
};

static struct nonterminal *deflist;
static struct nonterminal *undeflist;
#define BUFMAX 4096
static char buf[BUFMAX];
static int headerfile;

static void *
memalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memset(ptr, 0, size);
    return ptr;
}

void
memfree(void *ptr)
{
    free(ptr);
}

static void *
reverselist(void *list)
{
    void **newlist = 0;
    void **item = list;
    while (item) {
        void **next = *item;
        *item = newlist;
        newlist = item;
        item = next;
    }
    return newlist;
}

static void
additem(struct item *item)
{
    item->next = deflist->prodlist->itemlist;
    deflist->prodlist->itemlist = item;
}

static void
addprod(struct prod *prod)
{
    prod->next = deflist->prodlist;
    deflist->prodlist = prod;
}

static struct nonterminal *
findnonterminal(const char *name, unsigned int namelen)
{
    struct nonterminal *nonterminal = deflist;
    while (nonterminal) {
        if (nonterminal->namelen == namelen && !memcmp(name, nonterminal->name, namelen))
            return nonterminal;
        nonterminal = nonterminal->next;
    }
    nonterminal = undeflist;
    while (nonterminal) {
        if (nonterminal->namelen == namelen && !memcmp(name, nonterminal->name, namelen))
            return nonterminal;
        nonterminal = nonterminal->next;
    }
    nonterminal = memalloc(sizeof(struct nonterminal) + namelen);
    memcpy(nonterminal->name, name, namelen);
    nonterminal->name[namelen] = 0;
    nonterminal->namelen = namelen;
    nonterminal->next = undeflist;
    undeflist = nonterminal;
    return nonterminal;
}

static void
defnonterminal(struct nonterminal *nonterminal)
{
    struct nonterminal **p = &undeflist;
    for (;;) {
        if (!*p) {
            fprintf(stderr, "non-terminal '%s' defined twice\n", nonterminal->name);
            exit(1);
        }
        if (*p == nonterminal)
            break;
        p = &(*p)->next;
    }
    *p = nonterminal->next;
    nonterminal->next = deflist;
    deflist = nonterminal;
}

static void
readfile(const char *filename)
{
    FILE *file;
    unsigned int linenum = 0;
    int startprod = 1;
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    for (;;) {
        const char *p, *p2;
        if (!fgets(buf, BUFMAX, file))
            break;
        linenum++;
        p = buf;
        for (;;) {
            int ch = *p;
            if (ch == '\n' || ch == 0)
                break;
            if (ch == ' ' || ch == '\t' || ch == '\r') {
                p++;
                continue;
            }
            if (ch == '#') /* comment */
                break;
            else if (ch == '[') {
                /* This must be the production number, eg [23]. */
                p2 = strchr(p + 1, ']');
                if (!p2) {
                    fprintf(stderr, "%i: missing ]\n", linenum);
                    exit(1);
                }
                p = p2 + 1;
                startprod = 1;
            } else if (ch == '"') {
                /* Quoted string -- find end. */
                struct item *item;
                p2 = strchr(p + 1, '"');
                if (!p2) {
                    fprintf(stderr, "%i: unterminated string\n", linenum);
                    exit(1);
                }
                item = memalloc(sizeof(struct item) + p2 - p - 1);
                memcpy(item->name, p + 1, p2 - p - 1);
                item->name[p2 - p - 1] = 0;
                item->type = ITEM_STRING;
                additem(item);
                p = p2 + 1;
                startprod = 0;
            } else if ((unsigned)(ch - 'a') <= 'z' - 'a') {
                struct item *item;
                p2 = p;
                do {
                    p++;
                } while ((ch = *p) == '_' 
                        || (unsigned)(ch - '0') < 10
                        || (unsigned)((ch & ~0x20) - 'A') <= 'Z' - 'A');
                item = memalloc(sizeof(struct item) + p - p2);
                memcpy(item->name, p2, p - p2);
                item->name[p - p2] = 0;
                item->type = ITEM_TOK;
                additem(item);
                startprod = 0;
            } else if ((unsigned)(ch - 'A') <= 'Z' - 'A') {
                struct nonterminal *nonterminal;
                p2 = p;
                do p++; while ((ch = *p) == '_' || (unsigned)(ch - '0') < 10
                        || (unsigned)((ch & ~0x20) - 'A') <= 'Z' - 'A');
                nonterminal = findnonterminal(p2, p - p2);
                if (startprod)
                    defnonterminal(nonterminal);
                else {
                    struct item *item;
                    item = memalloc(sizeof(struct item));
                    item->type = ITEM_NONTERMINAL;
                    item->nonterminal = nonterminal;
                    additem(item);
                }
                startprod = 0;
            } else if (ch == '|') {
                struct prod *prod = memalloc(sizeof(struct prod));
                addprod(prod);
                p++;
            } else if (!memcmp(p, "\xce\xb5", 2)) { /* epsilon */
                p += 2;
                startprod = 0;
            } else if (!memcmp(p, "\xe2\x86\x92", 3)) { /* arrow character */
                struct prod *prod = memalloc(sizeof(struct prod));
                addprod(prod);
                p++;
                p += 3;
            } else {
                fprintf(stderr, "%i: unrecognised character '%c'\n", linenum, ch);
                exit(1);
            }
        }
    }
    fclose(file);
    if (undeflist) {
        do {
            fprintf(stderr, "undefined non-terminal '%s'\n", undeflist->name);
            undeflist = undeflist->next;
        } while (undeflist);
        exit(1);
    }
}

static int
addinitterms(struct nonterminal *nonterminal, struct prod *resultingprod,
        struct prod *prod)
{
    struct item *item = prod->itemlist;
    struct initterm *initterm;
    if (!item)
        return 1;
    while (item->type == ITEM_NONTERMINAL) {
        struct prod *prod2 = item->nonterminal->prodlist;
        int canbeempty = 0;
        while (prod2) {
            canbeempty |= addinitterms(nonterminal, resultingprod, prod2);
            prod2 = prod2->next;
        }
        if (!canbeempty)
            return 0;
        item = item->next;
        if (!item)
            return 1;
    }
    /* We have an initial terminal that results in the production
     * resultingprod. If the same terminal is already in the list,
     * then either it results in the same production, which is OK,
     * or it results in a different production, in which case the
     * grammar is not LL(1). */
    initterm = nonterminal->inittermlist;
    while (initterm) {
        if (initterm->item
            && initterm->item->type == item->type
            && !strcmp(initterm->item->name, item->name))
        {
            if (initterm->prod == resultingprod)
                break;
            fprintf(stderr, "grammar not LL(1) at start of '%s'\n",
                    nonterminal->name);
            exit(1);
        }
        initterm = initterm->next;
    }
    if (!initterm) {
        /* Not found in inittermlist. Add a new one. */
        initterm = memalloc(sizeof(struct initterm));
        initterm->prod = resultingprod;
        initterm->item = item;
        initterm->next = nonterminal->inittermlist;
        nonterminal->inittermlist = initterm;
    }
    return 0;
}

static void
build(void)
{
    struct nonterminal *nonterminal;
    deflist = reverselist(deflist);
    nonterminal = deflist;
    while (nonterminal) {
        struct prod *prod;
        nonterminal->prodlist = reverselist(nonterminal->prodlist);
        prod = nonterminal->prodlist;
        while (prod) {
            prod->itemlist = reverselist(prod->itemlist);
            if (!prod->itemlist)
                nonterminal->canbeempty = 1;
            prod = prod->next;
        }
        nonterminal = nonterminal->next;
    }
    nonterminal = deflist;
    while (nonterminal) {
        struct prod *prod;
        prod = nonterminal->prodlist;
        while (prod) {
            addinitterms(nonterminal, prod, prod);
            prod = prod->next;
        }
        nonterminal = nonterminal->next;
    }
}

static void
printitem(struct item *item)
{
    switch (item->type) {
    case ITEM_NONTERMINAL:
        printf(" NT_%s,", item->nonterminal->name);
        break;
    case ITEM_TOK:
        {
            const char *p;
            printf(" TOK_");
            p = item->name;
            for (;;) {
                int ch = *p++;
                if (!ch)
                    break;
                putchar(toupper(ch));
            }
            putchar(',');
        }
        break;
    case ITEM_STRING:
        if (strlen(item->name) == 1)
            printf(" '%c',", item->name[0]);
        else if (!strcmp(item->name, "::"))
            printf(" TOK_DOUBLECOLON,");
        else
            printf(" TOK_%s,", item->name);
        break;
    }
}

static void
outpass(int writing)
{
    unsigned int offset = 0;
    struct nonterminal *nonterminal = deflist;
    while (nonterminal) {
        struct initterm *initterm;
        struct prod *prod;
        nonterminal->offset = offset;
        if (!writing) {
            if (headerfile)
                printf("#define NT_%s (%i + NT_START)\n", nonterminal->name, nonterminal->offset);
        } else
            printf("/* %i: NT_%s */", nonterminal->offset, nonterminal->name);
        initterm = nonterminal->inittermlist;
        while (initterm) {
            if (writing) {
                printitem(initterm->item);
                printf(" %i,", initterm->prod->offset);
            }
            offset += 2;
            initterm = initterm->next;
        }
        if (writing) {
            if (nonterminal->canbeempty)
                printf(" EMPTY,\n");
            else
                printf(" END,\n");
        }
        offset++;
        prod = nonterminal->prodlist;
        while (prod) {
            struct item *item;
            prod->offset = offset;
            item = prod->itemlist;
            if (item) {
                if (writing) printf("    /* %i */", prod->offset);
                while (item) {
                    if (writing)
                        printitem(item);
                    offset++;
                    item = item->next;
                }
                if (writing) printf(" END,\n");
                offset++;
            }
            prod = prod->next;
        }
        nonterminal = nonterminal->next;
    }
}

static void
outstrings(void)
{
    struct nonterminal *nonterminal = deflist;
    printf("#define NTNAMES ");
    while (nonterminal) {
        printf("\\\n  \"\\%3.3o\\%3.3o%s\\0\"", nonterminal->offset & 0xff,
            nonterminal->offset >> 8, nonterminal->name);
        nonterminal = nonterminal->next;
    }
    printf("\n");
}

int
main(int argc, char **argv)
{
    if (!strcmp(*++argv, "-h")) {
        headerfile = 1;
        argv++;
    }
    if (!argv[0] || argv[1]) {
        fprintf(stderr, "usage: convgrammar grammarfile\n");
        exit(1);
    }
    readfile(argv[0]);
    printf("/* Automatically generated from %s by convgrammar.\n"
           " * Do not edit.\n"
           " */\n", argv[1]);
    build();
    outpass(0);
    if (headerfile)
        outstrings();
    if (!headerfile)
        outpass(1);
    return 0;
}

