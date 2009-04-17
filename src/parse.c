/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 *
 * LL(1) parser for Web IDL grammar.
 ***********************************************************************/
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "comment.h"
#include "lex.h"
#include "misc.h"
#include "parse.h"
#include "process.h"

#define END 0
#define EMPTY 1

/* The data table for the parser. */
static const unsigned short parsertbl[] = {
#include "grammar.h"
};

/***********************************************************************
 * tokerrorexit : error and exit with line number from token
 */
static void
tokerrorexit(struct tok *tok, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(filename, tok->linenum, format, ap);
    va_end(ap);
}

/***********************************************************************
 * lexnocomment : lex next token, discarding or storing comments
 *
 * A block comment starting with |** or |*! is a doxygen comment.
 * If it starts with |**< or |*!< then it refers to the previous
 * identifier, not the next one. (I am using | to represent / otherwise
 * this comment would be illegal.)
 *
 * An inline comment starting with /// or //! is a doxygen comment.
 * If it starts with ///< or //!< then it refers to the previous
 * identifier, not the next one.
 */
struct tok *
lexnocomment(void)
{
    struct tok *tok;
    for (;;) {
        tok = lex();
        if (tok->type != TOK_BLOCKCOMMENT && tok->type != TOK_INLINECOMMENT)
            break;
        addcomment(tok);
    }
    return tok;
}

/***********************************************************************
 * foldscopedname : fold parsed ScopedName back into one string
 *
 * Enter:   scopedname = ScopedName node
 *
 * Return:  0-terminated string in memalloced buffer
 */
static char *
foldscopedname(struct node *scopedname)
{
    char *buf = 0;
    assert(scopedname->type == NT_ScopedName);
    /* Two passes -- one to count the string length, one to write it. */
    for (;;) {
        char *wp = buf;
        struct node *node = scopedname->children;
        while (node) {
            if (node->type >= NT_START) {
                assert(node->type >= NT_START);
                assert(!node->next);
                node = node->children;
                continue;
            }
            if (node->type == TOK_DOUBLECOLON) {
                if (buf) memcpy(wp, "::", 2);
                wp += 2;
            } else if (node->type == TOK_IDENTIFIER) {
                unsigned int len = strlen(node->name);
                if (buf) memcpy(wp, node->name, len);
                wp += len;
            }
            node = node->next;
        }
        /* End of pass. */
        if (buf) {
            *wp = 0;
            break;
        }
        buf = memalloc(wp - buf + 1);
    }
    return buf;
}

/***********************************************************************
 * parsenode : parse a node (and its subnodes)
 *
 * Enter:   parent = parent node to add to
 *          nonterm = entry in grammar table
 *          tok = current lookahead token
 *
 * Return:  new lookahead token
 */
static struct tok *
parsenode(struct node *parent, unsigned int nonterm, struct tok *tok)
{
    const unsigned short *p;
    unsigned int prod;
    struct node *node = memalloc(sizeof(struct node));
    node->type = nonterm + NT_START;
    node->name[0] = 0;
    node->children = 0;
    node->comments = 0;
    node->next = parent->children;
    node->parent = parent;
    parent->children = node;
restart:
    /* Find token in list of tokens for this nonterminal. */
    p = parsertbl + nonterm;
    for (;;) {
        unsigned int toktype = *p;
        if (toktype == END) {
            tokerrorexit(tok, "unexpected token '%.*s'",
                    tok->len, tok->start);
        }
        if (toktype == EMPTY)
            goto end;
        if (toktype == TOK_OTHER && (unsigned)tok->type < 0x80
                && !strchr("()[]{},", tok->type))
        {
            break;
        }
        if (toktype == tok->type)
            break;
        p += 2;
    }
    /* Find start of production for that token. */
    prod = p[1];
    p = parsertbl + prod;
    for (;;) {
        unsigned int toktype = *p++;
        if (toktype == END)
            break;
        if (toktype >= NT_START) {
            /* Need a non-terminal. */
            toktype -= NT_START;
            if (toktype == nonterm && *p == END) {
                /* This is a recursion into the same non-terminal. We don't
                 * actually want to recurse. */
                goto restart;
            }
            tok = parsenode(node, toktype, tok);
        } else if ((toktype == TOK_OTHER && (unsigned)tok->type < 0x80
                && !strchr("()[]{},", tok->type))
                || toktype == tok->type)
        {
            struct node *node2 = memalloc(sizeof(struct node) + tok->len);
            memcpy(node2->name, tok->start, tok->len);
            node2->name[tok->len] = 0;
            node2->type = tok->type;
            node2->children = 0;
            node2->comments = 0;
            node2->next = node->children;
            node->children = node2;
            node2->parent = node;
            if (toktype == TOK_IDENTIFIER) {
                switch (node->type) {
                case NT_Module:
                case NT_Interface:
                case NT_Exception:
                case NT_Const:
                case NT_Attribute:
                case NT_Operation:
                case NT_Argument:
                case NT_ExceptionMember:
                case NT_Typedef:
                case NT_Valuetype:
                    /* We have found a new identifier to attach comments to. */
                    setidentifier(node2);
                }
            }
            tok = lexnocomment();
        }
    }
end:
    if (node->type == NT_Arguments || node->type == NT_ExtendedAttributes
            || node->type == NT_ScopedNames)
    {
        /* As a special case, we transfer Argument children of Arguments to the
         * parent ArgumentList, and we delete the Arguments node. This makes
         * the output XML easier to process.
         * Similarly for ExtendedAttribute children of ExtendedAttributes.
         * Similarly for ScopedName children of ScopedNames. */
        /* First reverse the children of the Arguments node, as they will
         * get reversed again on transferring. Also only keep the
         * Argument children (discarding the commas). */
        struct node *args = 0;
        struct node *child = node->children;
        while (child) {
            struct node *next = child->next;
            if (child->type == NT_Argument
                    || child->type == NT_ExtendedAttribute
                    || child->type == NT_ScopedName)
            {
                child->next = args;
                args = child;
            }
            child = next;
        }
        /* Delete the Arguments node from its parent. */
        assert(parent->type == NT_ArgumentList || parent->type == NT_ExtendedAttributeList || parent->type == NT_ScopedNameList);
        parent->children = node->next;
        memfree(node);
        /* Add the args to the parent ArgumentList. */
        while (args) {
            struct node *next = args->next;
            args->next = parent->children;
            parent->children = args;
            args->parent = parent;
            args = next;
        }
    } else if (node->type == NT_ScopedName) {
        /* Fold ScopedName back into one name and create an identifier
         * node for it, replacing the original children. */
        char *name = foldscopedname(node);
        struct node *newnode = memalloc(sizeof(struct node) + strlen(name));
        strcpy(newnode->name, name);
        memfree(name);
        newnode->type = TOK_IDENTIFIER;
        newnode->parent = node;
        node->children = newnode;
    } else if (!node->children) {
        /* Empty node -- remove it. */
        parent->children = node->next;
        memfree(node);
    }
    return tok;
}

/***********************************************************************
 * reversechildren : recursively reverse child lists
 *
 * Also sets parent field on each node.
 */
static void
reversechildren(struct node *node)
{
    struct node *newlist = 0;
    struct node *child = node->children;
    while (child) {
        struct node *next = child->next;
        child->parent = node;
        child->next = newlist;
        newlist = child;
        reversechildren(child);
        child = next;
    }
    node->children = newlist;
}

/***********************************************************************
 * parse : parse an input file
 *
 * Return:  root node of syntax tree
 */
struct node *
parse(void)
{
    struct tok *tok;
    struct node *rootroot = memalloc(sizeof(struct node)), *root;
    tok = lexnocomment();
    tok = parsenode(rootroot, 0, tok);
    if (tok->type != TOK_EOF)
        tokerrorexit(tok, "expected end of input");
    root = rootroot->children;
    memfree(rootroot);
    root->parent = 0;
    reversechildren(root);
    return root;
}

