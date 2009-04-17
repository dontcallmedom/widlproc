/***********************************************************************
 * $Id$
 * $URL$
 * Copyright 2009 Aplix Corporation. All rights reserved.
 ***********************************************************************/
#ifndef lex_h
#define lex_h

#define KEYWORDS \
    "DOMString\0" \
    "FALSE\0" \
    "Object\0" \
    "TRUE\0" \
    "any\0" \
    "attribute\0" \
    "boolean\0" \
    "const\0" \
    "exception\0" \
    "float\0" \
    "getraises\0" \
    "in\0" \
    "interface\0" \
    "long\0" \
    "module\0" \
    "octet\0" \
    "raises\0" \
    "readonly\0" \
    "sequence\0" \
    "setraises\0" \
    "short\0" \
    "typedef\0" \
    "unsigned\0" \
    "valuetype\0" \
    "void\0"


enum toktype {
    TOK_EOF = -1,
    TOK_BLOCKCOMMENT = 0x80,
    TOK_INLINECOMMENT, TOK_INTEGER, TOK_FLOAT, TOK_IDENTIFIER,
    TOK_STRING, TOK_DOUBLECOLON,
    TOK_OTHER, /* used in the parser tables to indicate any single char symbol */
    /* Keywords must be in the same order as above. */
    TOK_DOMString,
    TOK_FALSE,
    TOK_Object,
    TOK_TRUE,
    TOK_any,
    TOK_attribute,
    TOK_boolean,
    TOK_const,
    TOK_exception,
    TOK_float,
    TOK_getraises,
    TOK_in,
    TOK_interface,
    TOK_long,
    TOK_module,
    TOK_octet,
    TOK_raises,
    TOK_readonly,
    TOK_sequence,
    TOK_setraises,
    TOK_short,
    TOK_typedef,
    TOK_unsigned,
    TOK_valuetype,
    TOK_void,
};

struct tok {
    enum toktype type;
    unsigned int linenum;
    const char *start;
    unsigned int len;
};

extern const char *filename;

void lexopen(const char *name);
void lexclose(void);
struct tok *lex(void);

#endif /* ndef lex_h */
