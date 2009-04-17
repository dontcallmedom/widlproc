/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "comment.h"
#include "lex.h"
#include "misc.h"
#include "os.h"
#include "parse.h"
#include "process.h"

static const char ntnames[] = { NTNAMES };

/***********************************************************************
 * printtext : print text with xml entity escapes
 *
 * Enter:   s = text
 *          len = number of bytes
 *
 * This also escapes double quote mark so it can be used for an
 * attribute value.
 */
void
printtext(const char *s, unsigned int len)
{
    const char *p = s, *end = s + len;
    while (p != end) {
        int ch = *p;
        char buf[6];
        const char *seq = 0;
        switch (ch) {
        case '<':
            seq = "&lt;";
            break;
        case '&':
            seq = "&amp;";
            break;
        case '"':
            seq = "&quot;";
            break;
        default:
            if ((unsigned char)ch >= 0x20 || ch == '\n') {
                p++;
                continue;
            }
            snprintf(buf, 6, "&#%i;", ch);
            seq = buf;
            break;
        }
        if (p - s != fwrite(s, 1, p - s, stdout))
            errorexit("write error");
        fputs(seq, stdout);
        s = ++p;
    }
    if (p - s != fwrite(s, 1, p - s, stdout))
        errorexit("write error");
}

/***********************************************************************
 * outputnodeastext : output parse node and descendents as deparsed text
 *
 * Enter:   node = parse node
 *          needspace = true if last output char was an identifier char
 *
 * Return:  updated needspace value
 */
static int
outputnodeastext(struct node *node, int needspace)
{
    if (node->type >= NT_START) {
        struct node *child = node->children;
        while (child) {
            needspace = outputnodeastext(child, needspace);
            child = child->next;
        }
    } else {
        unsigned int len = strlen(node->name);
        if (len) {
            int ch = node->name[0];
            if (ch == '_' || ((unsigned)(ch - '0') < 10
                    || (unsigned)((ch & ~0x20) - 'A') < 26))
            {
                if (needspace) putchar(' ');
            }
            ch = node->name[len - 1];
            if (ch == '_' || ((unsigned)(ch - '0') < 10
                    || (unsigned)((ch & ~0x20) - 'A') < 26))
            {
                needspace = 1;
            }
            printtext(node->name, len);
        }
    }
    return needspace;
}

/***********************************************************************
 * output : output subtree of parse tree
 *
 * Enter:   node = root of subtree
 *          extendedattributelist = 0 else extended attribute list node
 *                                  applying to node
 *          indent = indent (nesting) level
 */
static void outputchildren(struct node *node, struct node *identifier, unsigned int indent);

static void
output(struct node *node, struct node *extendedattributelist,
       unsigned int indent)
{
    if (node->type == NT_ExtendedAttribute) {
        printf("%*s<ExtendedAttribute value=\"", indent, "");
        outputnodeastext(node, 0);
        printf("\"/>\n");
    } else if (node->type == NT_BooleanLiteral) {
        printf("%*s<BooleanLiteral value=\"%s\"/>", indent, "",
            node->children->name);
    } else if (node->type == NT_ReadOnly) {
        printf("%*s<ReadOnly/>\n", indent, "");
    } else if (node->type >= NT_START) {
        const char *ntname;
        /* Find identifier child if any. */
        struct node *identifier = node->children;
        while (identifier) {
            if (identifier->type == TOK_IDENTIFIER)
                break;
            identifier = identifier->next;
        }
        /* Find nonterminal name. */
        ntname = ntnames + 2;
        while (node->type - NT_START != ((unsigned char)ntname[-2] | (unsigned char)ntname[-1] << 8))
            ntname += strlen(ntname) + 3;
        /* Output start of element. */
        printf("%*s<%s", indent, "", ntname);
        /* Output identifier if any as attribute. */
        if (identifier) {
            printf(" identifier=\"");
            printtext(identifier->name, strlen(identifier->name));
            printf("\"");
        }
        if (!identifier && !extendedattributelist && !node->children && !node->comments)
            printf("/>\n");
        else {
            printf(">\n");
            /* Output descriptive elements (doxygen comments) for node. */
            outputdescriptive(node, indent + 2);
            /* Output descriptive elements (doxygen comments) for identifier. */
            if (identifier)
                outputdescriptive(identifier, indent + 2);
            /* Output extended attribute list. */
            if (extendedattributelist)
                output(extendedattributelist, 0, indent + 2);
            /* Output children (excluding identifier child). */
            outputchildren(node, identifier, indent + 2);
            printf("%*s</%s>\n", indent, "", ntname);
        }
    } else switch (node->type) {
    case TOK_DOMString:
    case TOK_any:
    case TOK_boolean:
    case TOK_octet:
    case TOK_float:
    case TOK_Object:
    case TOK_unsigned:
    case TOK_short:
    case TOK_long:
    case TOK_void:
        printf("%*s<%s/>\n", indent, "", node->name);
        break;
    case TOK_INTEGER:
        printf("%*s<integer value=\"", indent, "");
        printtext(node->name, strlen(node->name));
        printf("\"/>\n");
        break;
    case TOK_FLOAT:
        printf("%*s<Float value=\"", indent, "");
        printtext(node->name, strlen(node->name));
        printf("\"/>\n");
        break;
    case TOK_STRING:
        printf("%*s<string value=\"", indent, "");
        printtext(node->name, strlen(node->name));
        printf("\"/>\n");
        break;
    }
}

/***********************************************************************
 * outputchildren : call output for each child of node
 *
 * Enter:   node
 *          identifier = child node to omit from output
 *          indent = indent (nesting) level
 */
static void
outputchildren(struct node *node, struct node *identifier, unsigned int indent)
{
    struct node *extendedattributelist;
    struct node *child;
    child = node->children;
    extendedattributelist = 0;
    while (child) {
        if (child->type == NT_ExtendedAttributeList && node->type != NT_Argument)
            extendedattributelist = child;
        else {
            if (identifier != child)
                output(child, extendedattributelist, indent);
            extendedattributelist = 0;
        }
        child = child->next;
    }
}

/***********************************************************************
 * processfile : process one input file
 *
 * Enter:   name = filename
 */
void
processfile(const char *name)
{
    struct node *root;
    lexopen(name);
    root = parse();
    processcomments(root);
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
           "<!DOCTYPE Definitions SYSTEM \"widlprocxml.dtd\">\n");
    output(root, 0, 0);
    lexclose();
}

