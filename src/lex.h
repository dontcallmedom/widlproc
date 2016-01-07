
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
#ifndef lex_h
#define lex_h

// starting from "attribute" are the list of names allowed as arguments identifier
#define KEYWORDS \
    "DOMString\0" \
    "ByteString\0" \
    "USVString\0" \
    "RegExp\0" \
    "Error\0" \
    "DOMException\0" \
    "ArrayBuffer\0" \
    "DataView\0" \
    "Int8Array\0" \
    "Int16Array\0" \
    "Int32Array\0" \
    "Uint8Array\0" \
    "Uint16Array\0" \
    "Uint32Array\0" \
    "Uint8ClampedArray\0" \
    "Float32Array\0" \
    "Float64Array\0" \
    "false\0" \
    "object\0" \
    "true\0" \
    "any\0" \
    "boolean\0" \
    "byte\0" \
    "double\0" \
    "float\0" \
    "FrozenArray\0" \
    "Infinity\0"  \
    "-Infinity\0" \
    "iterable\0" \
    "long\0" \
    "NaN\0" \
    "null\0" \
    "octet\0" \
    "optional\0" \
    "or\0" \
    "Promise\0" \
    "readonly\0" \
    "sequence\0" \
    "short\0" \
    "unsigned\0" \
    "void\0" \
    "attribute\0" \
    "callback\0" \
    "const\0" \
    "creator\0" \
    "deleter\0" \
    "dictionary\0" \
    "enum\0" \
    "exception\0" \
    "getter\0" \
    "implements\0" \
    "inherit\0" \
    "interface\0" \
    "legacycaller\0" \
    "maplike\0" \
    "partial\0" \
    "required\0" \
    "serializer\0" \
    "setter\0" \
    "static\0" \
    "stringifier\0" \
    "typedef\0" \
    "unrestricted\0"


enum toktype {
    TOK_EOF = -1,
    TOK_BLOCKCOMMENT = 0x80,
    TOK_INLINECOMMENT, TOK_INTEGER, TOK_FLOAT, TOK_IDENTIFIER,
    TOK_STRING, TOK_ELLIPSIS,
    /* Keywords must be in the same order as above. */
    TOK_DOMString,
    TOK_ByteString,
    TOK_USVString,
    TOK_RegExp,
    TOK_Error,
    TOK_DOMException,
    TOK_ArrayBuffer,
    TOK_DataView,
    TOK_Int8Array,
    TOK_Int16Array,
    TOK_Int32Array,
    TOK_Uint8Array,
    TOK_Uint16Array,
    TOK_Uint32Array,
    TOK_Uint8ClampedArray,
    TOK_Float32Array,
    TOK_Float64Array,
    TOK_false,
    TOK_object,
    TOK_true,
    TOK_any,
    TOK_boolean,
    TOK_byte,
    TOK_double,
    TOK_float,
    TOK_frozenarray,
    TOK_infinity,
    TOK_minusinfinity,
    TOK_iterable,
    TOK_long,
    TOK_NaN,
    TOK_null,
    TOK_octet,
    TOK_optional,
    TOK_or,
    TOK_Promise,
    TOK_readonly,
    TOK_sequence,
    TOK_short,
    TOK_unsigned,
    TOK_void,
    /* Below that line are keywords that are allowed as arguments names */
    TOK_attribute,
    TOK_callback,
    TOK_const,
    TOK_creator,
    TOK_deleter,
    TOK_dictionary,
    TOK_enum,
    TOK_exception,
    TOK_getter,
    TOK_implements,
    TOK_inherit,
    TOK_interface,
    TOK_legacycaller,
    TOK_maplike,
    TOK_partial,
    TOK_required,
    TOK_serializer,
    TOK_setter,
    TOK_static,
    TOK_stringifier,
    TOK_typedef,
    TOK_unrestricted,
};

struct tok {
    enum toktype type;
    const char *filename;
    unsigned int linenum;
    const char *prestart;
    const char *start;
    unsigned int len;
};

extern const char *filename;
extern const char keywords[];

struct node;

void readinput(const char *const *argv);
struct tok *lex(void);
void outputwidl(struct node *node);

#endif /* ndef lex_h */
