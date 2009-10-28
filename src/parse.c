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
 * Hand-crafted recursive descent parser for Web IDL grammar.
 ***********************************************************************/
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "comment.h"
#include "lex.h"
#include "misc.h"
#include "node.h"
#include "parse.h"

/***********************************************************************
 * tokerrorexit : error and exit with line number from token
 */
static void
tokerrorexit(struct tok *tok, const char *format, ...)
{
    va_list ap;
    char *m;
    va_start(ap, format);
    m = vmemprintf(format, ap);
    if (tok->type == TOK_EOF)
        locerrorexit(tok->filename, tok->linenum, "at end of input: %s", m);
    else
        locerrorexit(tok->filename, tok->linenum, "at '%.*s': %s", tok->len, tok->start, m);
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
static struct tok *
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
 * eat : check token then read the following one
 *
 * Enter:   tok struct
 *          type = type of token expected, error given if no match
 *
 * On return, tok is updated to the following token.
 */
static void
eat(struct tok *tok, int type)
{
    if (tok->type != type)
        tokerrorexit(tok, "expected '%c'", type);
    lexnocomment();
}

/***********************************************************************
 * setid : flag that an id attribute is required on node
 *
 * Enter:   node
 *
 * node->id is set to the value of the name attribute. This makes
 * outputnode give it an id attribute whose value is the id attribute
 * of the parent if any, then "::", then node->id.
 */
static void
setid(struct node *node)
{
    node->id = getattr(node, "name");
}

/***********************************************************************
 * setidentifier : allocate 0-terminated string for identifier token
 *
 * Enter:   tok = token, error given if not identifier
 *
 * Return:  allocated 0-terminated string
 */
static char *
setidentifier(struct tok *tok)
{
    char *s;
    if (tok->type != TOK_IDENTIFIER)
        tokerrorexit(tok, "expected identifier");
    s = memprintf("%.*s", tok->len, tok->start);
    return s;
}

/* Prototypes for funcs that have a forward reference. */
static struct node *parsetype(struct tok *tok);
static struct node *parseargumentlist(struct tok *tok);
static void parsedefinitions(struct tok *tok, struct node *parent);

/***********************************************************************
 * parsescopedname : parse [53] ScopedName
 *
 * Enter:   tok = next token
 *          name = name of attribute to put scoped name in
 *          ref = whether to enable enclosing of the name in <ref> in
 *                outputwidl
 *
 * Return:  node struct for new attribute
 *          tok updated
 */
static struct node *
parsescopedname(struct tok *tok, const char *name, int ref)
{
    const char *start = tok->start, *end;
    struct node *node;
    unsigned int len = 0;
    char *s = memalloc(3);
    if (tok->type == TOK_DOUBLECOLON) {
        strcpy(s, "::");
        len = 2;
        lexnocomment();
    }
    for (;;) {
        if (tok->type != TOK_IDENTIFIER)
            tokerrorexit(tok, "expected identifier in scoped name");
        s = memrealloc(s, len + tok->len + 1);
        memcpy(s + len, tok->start, tok->len);
        len += tok->len;
        end = tok->start + tok->len;
        lexnocomment();
        if (tok->type != TOK_DOUBLECOLON)
            break;
        lexnocomment();
    }
    s[len] = 0;
    node = newattr(name, s);
    if (ref) {
        node->start = start;
        node->end = end;
    }
    return node;
}

/***********************************************************************
 * parsescopednamelist : parse [51] ScopedNameList
 *
 * Enter:   tok = next token
 *          name = name of element for scoped name list
 *          name2 = name of element for entry in list
 *          comment = whether to attach documentation to each name
 *
 * Return:  node for list of scoped names
 *          tok updated
 */
static struct node *
parsescopednamelist(struct tok *tok, const char *name, const char *name2,
        int comment)
{
    struct node *node = newelement(name);
    for (;;) {
        struct node *attr = parsescopedname(tok, "name", 1);
        struct node *n = newelement(name2);
        if (comment)
            setcommentnode(n);
        addnode(n, attr);
        addnode(node, n);
        if (tok->type != ',')
            break;
        lexnocomment();
    }
    return node;
}

/***********************************************************************
 * parsereturntype : parse [50] ReturnType
 *
 * Enter:   tok = next token
 *
 * Return:  node for type
 *          tok updated
 */
static struct node *
parsereturntype(struct tok *tok)
{
    if (tok->type == TOK_void) {
        struct node *node = newelement("Type");
        addnode(node, newattr("type", "void"));
        lexnocomment();
        return node;
    }
    return parsetype(tok);
}

/***********************************************************************
 * parseunsignedintegertype : parse [46] UnsignedIntegerType
 *
 * Enter:   tok = next token (one of "unsigned", "short" or "long")
 *
 * Return:  0-terminated canonical string for the type
 *          tok updated to just past UnsignedIntegerType
 */
static const char *
parseunsignedintegertype(struct tok *tok)
{
    static const char *names[] = {
        "short", "long", "long long", 0,
        "unsigned short", "unsigned long", "unsigned long long" };
    enum { TYPE_SHORT, TYPE_LONG, TYPE_LONGLONG,
           TYPE_UNSIGNED = 4 };
    int type = 0;
    if (tok->type == TOK_unsigned) {
        type = TYPE_UNSIGNED;
        lexnocomment();
    }
    if (tok->type == TOK_short) {
        type |= TYPE_SHORT;
        lexnocomment();
    } else if (tok->type != TOK_long)
        tokerrorexit(tok, "expected 'short' or 'long' after 'unsigned'");
    else {
        type |= TYPE_LONG;
        lexnocomment();
        if (tok->type == TOK_long) {
            type += TYPE_LONGLONG - TYPE_LONG;
            lexnocomment();
        }
    }
    return names[type];
}

/***********************************************************************
 * parsenullabletype : parse [45] NullableType
 *
 * Enter:   tok = next token
 *
 * Return:  node for type
 *          tok updated
 */
static struct node *
parsenullabletype(struct tok *tok)
{
    struct node *node = newelement("Type");
    switch (tok->type) {
    case TOK_unsigned:
    case TOK_short:
    case TOK_long:
        addnode(node, newattr("type", parseunsignedintegertype(tok)));
        break;
    case TOK_sequence:
        lexnocomment();
        eat(tok, '<');
        addnode(node, parsetype(tok));
        eat(tok, '>');
        break;
    default:
        switch (tok->type) {
        default:
            tokerrorexit(tok, "expected type");
            break;
        case TOK_boolean:
            addnode(node, newattr("type", "boolean"));
            break;
        case TOK_octet:
            addnode(node, newattr("type", "octet"));
            break;
        case TOK_float:
            addnode(node, newattr("type", "float"));
            break;
        case TOK_double:
            addnode(node, newattr("type", "double"));
            break;
        case TOK_DOMString:
            addnode(node, newattr("type", "DOMString"));
            break;
        }
        lexnocomment();
    }
    if (tok->type == '?') {
        addnode(node, newattr("nullable", "nullable"));
        lexnocomment();
    }
    return node;
}

/***********************************************************************
 * parsetype : parse [44] Type
 *
 * Enter:   tok = next token
 *
 * Return:  node for type
 *          tok updated
 */
static struct node *
parsetype(struct tok *tok)
{
    struct node *node;
    switch (tok->type) {
    case TOK_DOUBLECOLON:
    case TOK_IDENTIFIER:
        node = newelement("Type");
        addnode(node, parsescopedname(tok, "name", 1));
        break;
    case TOK_any:
        node = newelement("Type");
        addnode(node, newattr("type", "any"));
        lexnocomment();
        break;
    case TOK_object:
        node = newelement("Type");
        addnode(node, newattr("type", "object"));
        lexnocomment();
        break;
    default:
        node = parsenullabletype(tok);
        break;
    }
    return node;
}

/***********************************************************************
 * parseextendedattribute : parse [39] ExtendedAttribute
 *
 * Enter:   tok = next token
 *
 * Return:  node for extended attribute
 *
 * This parses the various forms of extended attribute, as in
 * rules [57] [58] [59] [60] [61].
 *
 * This does not spot the error that you cannot have a ScopedName
 * and an ArgumentList.
 */
static struct node *
parseextendedattribute(struct tok *tok)
{
    struct node *node = newelement("ExtendedAttribute");
    addnode(node, newattr("name", setidentifier(tok)));
    lexnocomment();
    if (tok->type == '=') {
        lexnocomment();
        addnode(node, parsescopedname(tok, "value", 0));
    }
    if (tok->type == '(') {
        lexnocomment();
        addnode(node, parseargumentlist(tok));
        eat(tok, ')');
    }
    return node;
}

/***********************************************************************
 * parseextendedattributelist : parse [37] ExtendedAttributeList
 *
 * Enter:   tok = next token
 *
 * Return:  0 else node for extended attribute list
 *          tok updated if anything parsed
 */
static struct node *
parseextendedattributelist(struct tok *tok)
{
    struct node *node;
    if (tok->type != '[')
        return 0;
    node = newelement("ExtendedAttributeList");
    for (;;) {
        lexnocomment();
        addnode(node, parseextendedattribute(tok));
        if (tok->type != ',')
            break;
    }
    if (tok->type != ']')
        tokerrorexit(tok, "expected ',' or ']'");
    lexnocomment();
    return node;
}

/***********************************************************************
 * parseexceptionfield : parse [36] ExceptionField
 *
 * Enter:   tok = next token
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the exceptionfield
 *          tok updated
 */
static struct node *
parseexceptionfield(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("ExceptionField");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    tok = lexnocomment();
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    eat(tok, ';');
    return node;
}

/***********************************************************************
 * parseargument : parse [31] Argument
 *
 * Enter:   tok = next token
 *
 * Return:  new node
 *
 * tok updated on return
 */
static struct node *
parseargument(struct tok *tok)
{
    struct node *node = newelement("Argument");
    struct node *eal = parseextendedattributelist(tok);
    setcommentnode(node);
    if (eal) addnode(node, eal);
    if (tok->type == TOK_in) {
        addnode(node, newattr("in", "in"));
        lexnocomment();
    }
    if (tok->type == TOK_optional) {
        addnode(node, newattr("optional", "optional"));
        lexnocomment();
    }
    addnode(node, parsetype(tok));
    if (tok->type == TOK_ELLIPSIS) {
        addnode(node, newattr("ellipsis", "ellipsis"));
        lexnocomment();
    }
    addnode(node, newattr("name", setidentifier(tok)));
    lexnocomment();
    return node;
}

/***********************************************************************
 * parseargumentlist : parse [29] ArgumentList
 *
 * Enter:   tok = next token
 *
 * Return:  new node for the arglist
 *          tok updated
 */
static struct node *
parseargumentlist(struct tok *tok)
{
    struct node *node = newelement("ArgumentList");
    /* We rely on the fact that ArgumentList is always followed by ')'. */
    if (tok->type != ')') {
        for (;;) {
            addnode(node, parseargument(tok));
            if (tok->type != ',')
                break;
            lexnocomment();
        }
    }
    return node;
}

/***********************************************************************
 * parseexceptionlist : parse [28] ExceptionList
 *
 * Enter:   tok = next token
 *          name = element name for list node
 *
 * Return:  new node for the scopednamelist
 *          tok updated
 */
static struct node *
parseexceptionlist(struct tok *tok, const char *name)
{
    struct node *node;
    eat(tok, '(');
    node = parsescopednamelist(tok, name, "RaiseException", 1);
    eat(tok, ')');
    return node;
}

/***********************************************************************
 * parseoperationrest : parse [25] OperationRest
 *
 * Enter:   tok = next token
 *          eal = 0 else extended attribute list node
 *          attrs = list-of-attrs node containing attrs to add to new node
 *
 * Return:  new node
 *          tok on terminating ';'
 */
static struct node *
parseoperationrest(struct tok *tok, struct node *eal, struct node *attrs)
{
    struct node *node = newelement("Operation");
    setcommentnode(node);
    addnode(node, attrs);
    addnode(node, parsereturntype(tok));
    if (tok->type == TOK_IDENTIFIER) {
        addnode(node, newattr("name", setidentifier(tok)));
        lexnocomment();
    }
    eat(tok, '(');
    addnode(node, parseargumentlist(tok));
    eat(tok, ')');
    if (tok->type == TOK_raises) {
        lexnocomment();
        addnode(node, parseexceptionlist(tok, "Raises"));
    }
    return node;
}

/***********************************************************************
 * parseattribute : parse [17] Attribute
 *
 * Enter:   tok = next token ("readonly" or "attribute")
 *          eal = 0 else extended attribute list node
 *          attrs = list-of-attrs node containing attrs to add to new node
 *
 * Return:  new node
 *          tok on terminating ';'
 */
static struct node *
parseattribute(struct tok *tok, struct node *eal, struct node *attrs)
{
    struct node *node = newelement("Attribute");
    setcommentnode(node);
    addnode(node, attrs);
    if (tok->type == TOK_readonly) {
        lexnocomment();
        addnode(node, newattr("readonly", "readonly"));
    }
    eat(tok, TOK_attribute);
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    lexnocomment();
    if (tok->type == TOK_getraises) {
        lexnocomment();
        addnode(node, parseexceptionlist(tok, "GetRaises"));
    }
    if (tok->type == TOK_setraises) {
        lexnocomment();
        addnode(node, parseexceptionlist(tok, "SetRaises"));
    }
    return node;
}

/***********************************************************************
 * parseattributeoroperation : parse [15] AttributeOrOperation
 *
 * Enter:   tok = next token
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node
 *          tok on terminating ';'
 */
static struct node *
parseattributeoroperation(struct tok *tok, struct node *eal)
{
    struct node *attrs = newattrlist();
    if (tok->type == TOK_stringifier) {
        addnode(attrs, newattr("stringifier", "stringifier"));
        lexnocomment();
        if (tok->type == ';') {
            struct node *node = newelement("Stringifier");
            if (eal) addnode(node, eal);
            return node;
        }
    }
    if (tok->type == TOK_readonly || tok->type == TOK_attribute)
        return parseattribute(tok, eal, attrs);
    if (nodeisempty(attrs)) {
        if (tok->type == TOK_omittable) {
            lexnocomment();
            addnode(attrs, newattr("omittable", "omittable"));
        }
        for (;;) {
            static const int t[] = { TOK_getter,
                TOK_setter, TOK_creator, TOK_deleter, TOK_caller,
                0 };
            const int *tt = t;
            char *s;
            while (*tt && *tt != tok->type)
                tt++;
            if (!*tt)
                break;
            s = memprintf("%.*s", tok->len, tok->start);
            addnode(attrs, newattr(s, s));
            lexnocomment();
        }
    }
    return parseoperationrest(tok, eal, attrs);
}

/***********************************************************************
 * parseconst : parse [12] Const
 *
 * Enter:   tok = next token, known to be TOK_const
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the const
 *          tok on terminating ';'
 */
static struct node *
parseconst(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Const");
    char *s;
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    eat(tok, '=');
    switch(tok->type) {
    case TOK_true:
    case TOK_false:
    case TOK_INTEGER:
    case TOK_FLOAT:
        break;
    default:
        tokerrorexit(tok, "expected constant value");
        break;
    }
    s = memalloc(tok->len + 1);
    memcpy(s, tok->start, tok->len);
    s[tok->len] = 0;
    addnode(node, newattr("value", s));
    lexnocomment();
    return node;
}

/***********************************************************************
 * parseimplementsstatement : parse [11] ImplementsStatement
 *
 * Enter:   tok = next token, known to be :: or TOK_IDENTIFIER
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the typedef
 *          tok updated to the terminating ';'
 */
static struct node *
parseimplementsstatement(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Implements");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    addnode(node, parsescopedname(tok, "name1", 1));
    eat(tok, TOK_implements);
    addnode(node, parsescopedname(tok, "name2", 1));
    return node;
}

/***********************************************************************
 * parsetypedef : parse [10] Typedef
 *
 * Enter:   tok = next token, known to be TOK_typedef
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the typedef
 *          tok updated to the terminating ';'
 */
static struct node *
parsetypedef(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Typedef");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    addnode(node, parsetype(tok));
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    return node;
}

/***********************************************************************
 * parseexception : parse [8] Exception
 *
 * Enter:   tok = next token, known to be TOK_exception
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the module
 *          tok updated to the terminating ';'
 */
static struct node *
parseexception(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Exception");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    lexnocomment();
    eat(tok, '{');
    while (tok->type != '}') {
        struct node *node2;
        struct node *eal = parseextendedattributelist(tok);
        if (tok->type == TOK_const)
            node2 = parseconst(tok, eal);
        else
            node2 = parseexceptionfield(tok, eal);
        addnode(node, node2);
        setid(node2);
    }
    lexnocomment();
    return node;
}

/***********************************************************************
 * parseinterface : parse [4] Interface
 *
 * Enter:   tok = next token, known to be TOK_interface
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the module
 *          tok updated to the terminating ';'
 */
static struct node *
parseinterface(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Interface");
    if (eal) addnode(node, eal);
    setcommentnode(node);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    if (tok->type == ':') {
        lexnocomment();
        addnode(node, parsescopednamelist(tok, "InterfaceInheritance", "Name", 1));
    }
    eat(tok, '{');
    while (tok->type != '}') {
        struct node *eal = parseextendedattributelist(tok);
        const char *start = tok->prestart;
        struct node *node2;
        if (tok->type == TOK_const)
            addnode(node, node2 = parseconst(tok, eal));
        else
            addnode(node, node2 = parseattributeoroperation(tok, eal));
        node2->wsstart = start;
        node2->end = tok->start + tok->len;
        setid(node2);
        eat(tok, ';');
    }
    lexnocomment();
    return node;
}

/***********************************************************************
 * parsemodule : parse [3] Module
 *
 * Enter:   tok = next token, known to be TOK_module
 *          eal = 0 else extended attribute list node
 *
 * Return:  new node for the module
 *          tok updated to the terminating ';'
 */
static struct node *
parsemodule(struct tok *tok, struct node *eal)
{
    struct node *node = newelement("Module");
    setcommentnode(node);
    if (eal) addnode(node, eal);
    tok = lexnocomment();
    addnode(node, newattr("name", setidentifier(tok)));
    tok = lexnocomment();
    eat(tok, '{');
    parsedefinitions(tok, node);
    eat(tok, '}');
    return node;
}

/***********************************************************************
 * parsedefinitions : parse [1] Definitions
 *
 * Enter:   tok = next token
 *          parent = parent node to add definitions to
 *
 * On return, tok has been updated.
 */
static void
parsedefinitions(struct tok *tok, struct node *parent)
{
    for (;;) {
        const char *wsstart = tok->prestart;
        struct node *eal = parseextendedattributelist(tok);
        struct node *node;
        switch (tok->type) {
        case TOK_module:
            node = parsemodule(tok, eal);
            break;
        case TOK_interface:
            node = parseinterface(tok, eal);
            break;
        case TOK_exception:
            node = parseexception(tok, eal);
            break;
        case TOK_typedef:
            node = parsetypedef(tok, eal);
            break;
        case TOK_DOUBLECOLON:
        case TOK_IDENTIFIER:
            node = parseimplementsstatement(tok, eal);
            break;
        default:
            if (eal)
                tokerrorexit(tok, "expected definition after extended attribute list");
            node = 0;
            break;
        }
        if (!node)
            break;
        node->wsstart = wsstart;
        node->end = tok->start + tok->len;
        eat(tok, ';');
        addnode(parent, node);
        setid(node);
    }
}

/***********************************************************************
 * parse
 *
 * Return:  root element containing (possibly empty) list of definitions
 */
struct node *
parse(void)
{
    struct node *root = newelement("Definitions");
    struct tok *tok = lexnocomment();
    parsedefinitions(tok, root);
    if (tok->type != TOK_EOF)
        tokerrorexit(tok, "expected end of input");
    reversechildren(root);
    return root;
}

