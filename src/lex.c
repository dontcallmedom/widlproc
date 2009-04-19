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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "lex.h"
#include "misc.h"

static FILE *file;
const char *filename;
static char *buf;
unsigned int bufmax, buflen, pos, linenum;
int eof;

/***********************************************************************
 * lexerrorexit : error and exit with line number
 */
static void
lexerrorexit(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(filename, linenum, format, ap);
    va_end(ap);
}

/***********************************************************************
 * lexopen : open input file and prepare to lex it
 *
 * Enter:   name = filename
 */
void
lexopen(const char *name)
{
    filename = name;
    file = fopen(name, "rb");
    if (!file)
        errorexit("%s: %s", filename, strerror(errno));
    bufmax = 4096;
    buf = memalloc(bufmax + 1);
    buflen = pos = 0;
    linenum = 1;
    eof = 0;
}

/***********************************************************************
 * lexclose : close current input file
 */
void
lexclose(void)
{
    fclose(file);
    file = 0;
    filename = 0;
}

/***********************************************************************
 * readmore : read more data from the input file
 *
 * Enter:   start = start of token currently being lexed
 *          p = position reached in buffer when 0 byte was found
 *
 * Return:  start of token after moving in buffer
 *
 * p should be at the end of the buffer, since we detected a 0
 * byte. If we're not at the end of the buffer, then the input has
 * a 0 byte in, which we can't cope with.
 */
static const char *
readmore(const char *start, const char *p)
{
    unsigned int readlen;
    if (p != buf + buflen)
        lexerrorexit("\\0 byte not allowed");
    /* Shift the part token we have so far down to the bottom of the
     * buffer. */
    buflen = p - start;
    memmove(buf, start, buflen);
    /* If the buffer is still full, double the size of it. */
    if (buflen == bufmax) {
        bufmax *= 2;
        buf = memrealloc(buf, bufmax);
    }
    /* Read the next chunk of file. */
    readlen = fread(buf + buflen, 1, bufmax - buflen, file);
    if (!readlen) {
        if (ferror(file))
            errorexit("%s: I/O error", filename);
        eof = 1;
    }
    buflen += readlen;
    buf[buflen] = 0;
    return buf;
}

/***********************************************************************
 * lexblockcomment : lex a block comment
 *
 * Enter:   start = start of comment
 *
 * Return:  tok struct, lifetime until next call to lex
 */
static struct tok *
lexblockcomment(const char *start)
{
    static struct tok tok;
    const char *p = start + 1;
    tok.linenum = linenum;
    for (;;) {
        int ch = *++p;
        unsigned int offset;
        if (ch) {
            if (ch != '*') {
                if (ch == '\n')
                    linenum++;
                continue;
            }
            ch = p[1];
            if (ch == '/')
                break;
            if (ch)
                continue;
        }
        if (eof)
            lexerrorexit("unterminated block comment");
        offset = p - start;
        start = readmore(start, p);
        p = start + offset - 1;
    }
    p += 2;
    pos = p - buf;
    tok.type = TOK_BLOCKCOMMENT;
    tok.start = start + 2;
    tok.len = p - start - 4;
    return &tok;
}

/***********************************************************************
 * lexinlinecomment : lex an inline comment
 *
 * Enter:   start = start of comment, starts with "//"
 *
 * Return:  tok struct, lifetime until next call to lex
 */
static struct tok *
lexinlinecomment(const char *start)
{
    static struct tok tok;
    const char *p = start + 2;
    p = start + 1;
    for (;;) {
        int ch = *++p;
        unsigned int offset;
        if (ch) {
            if (ch == '\n')
                break;
            continue;
        }
        if (eof)
            break;
        offset = p - start;
        start = readmore(start, p);
        p = start + offset - 1;
    }
    p++;
    pos = p - buf;
    tok.type = TOK_INLINECOMMENT;
    tok.start = start + 2;
    tok.len = p - start - 2;
    tok.linenum = linenum++;
    return &tok;
}

/***********************************************************************
 * lexnumber : lex a number (or just a '-' symbol)
 *
 * Enter:   start = start of token
 *
 * Return:  tok struct, lifetime until next call to lex
 *
 * The IDL grammar seems to say that a float can't start with a
 * decimal point, so that's what we have implemented here.
 */
static struct tok *
lexnumber(const char *start)
{
    static struct tok tok;
    for (;;) {
        const char *p = start;
        const char *octalend = start;
        int ch = *p;
        enum { STATE_START, STATE_INT, STATE_HEX, STATE_OCTAL, STATE_BADOCTAL,
                STATE_DP, STATE_EXPSTART, STATE_EXPSIGN, STATE_EXP
                } state = STATE_START;
        if (ch == '-')
            ch = *++p;
        if (!ch) {
            start = readmore(start, p);
            continue;
        }
        if (ch == '0') {
            state = STATE_OCTAL;
            ch = *++p;
            if (!ch) {
                start = readmore(start, p);
                continue;
            }
            if ((ch & ~0x20) == 'X') {
                state = STATE_HEX;
                ch = *++p;
            }
        }
        while (ch) {
            if ((unsigned)(ch - '0') >= 8) {
                if ((ch & -2) == '8') {
                    if (state == STATE_OCTAL) {
                        state = STATE_BADOCTAL;
                        octalend = p;
                    }
                } else if ((unsigned)((ch & ~0x20) - 'A') <= 'F' - 'A') {
                    if (state != STATE_HEX)
                        break;
                } else if (ch == '.') {
                    if (state == STATE_HEX || state >= STATE_DP)
                        break;
                } else if (ch == '-') {
                    if (state != STATE_EXPSTART)
                        break;
                    state = STATE_EXPSIGN;
                } else {
                    if ((ch & ~0x20) != 'E')
                        break;
                    if (state == STATE_HEX || state >= STATE_EXP || state == STATE_START)
                        break;
                }
            }
            ch = *++p;
            if (state == STATE_START)
                state = STATE_INT;
            else if (state == STATE_EXPSTART || state == STATE_EXPSIGN)
                state = STATE_EXP;
        }
        if (!ch && !eof) {
            start = readmore(start, p);
            continue;
        }
        switch (state) {
        case STATE_START:
            /* Must have just been a - character by itself. */
            tok.type = '-';
            p = start + 1;
            break;
        case STATE_BADOCTAL:
            p = octalend;
            /* fall through... */
        case STATE_INT:
        case STATE_OCTAL:
            tok.type = TOK_INTEGER;
            break;
        case STATE_HEX:
            if (p - start == 2 || (p - start == 3 && *start == '-'))
                p = start + 1;
            tok.type = TOK_INTEGER;
            break;
        case STATE_EXP:
        case STATE_DP:
            tok.type = TOK_FLOAT;
            break;
        case STATE_EXPSIGN:
            p--;
            /* fall through... */
        case STATE_EXPSTART:
            p--;
            tok.type = TOK_FLOAT;
            break;
        }
        tok.start = start;
        tok.len = p - start;
        tok.linenum = linenum;
        pos = p - buf;
        return &tok;
    }
}

/***********************************************************************
 * lexstring : lex a quoted string
 *
 * Enter:   start = start of token
 *
 * Return:  tok struct, lifetime until next call to lex
 */
static struct tok *
lexstring(const char *start)
{
    static struct tok tok;
    for (;;) {
        const char *p = start + 1;
        int ch = *p;
        while (ch) {
            if (ch == '\n')
                lexerrorexit("unterminated string");
            if (ch == '"') {
                tok.type = TOK_STRING;
                tok.start = start + 1;
                tok.len = p - start - 1;
                tok.linenum = linenum;
                pos = p + 1 - buf;
                return &tok;
            }
            /* Note the IDL spec doesn't seem to allow for escape sequences
             * in strings. */
            ch = *++p;
        }
        start = readmore(start, p);
    }
}

/***********************************************************************
 * lexidentifier : lex an identifier
 *
 * Enter:   start = start of token
 *
 * Return:  tok struct, lifetime until next call to lex
 */
static struct tok *
lexidentifier(const char *start)
{
    static struct tok tok;
    const char *p = start + 1;
    for (;;) {
        int ch = *p;
        if (!ch) {
            if (eof)
                break;
            p = start = readmore(start, p);
        } else if (ch != '_' && (unsigned)(ch - '0') >= 10
                && (unsigned)((ch & ~0x20) - 'A') > 'Z' - 'A')
        {
            break;
        }
        p++;
    }
    tok.type = TOK_IDENTIFIER;
    tok.start = start;
    tok.len = p - start;
    tok.linenum = linenum;
    pos = p - buf;
    /* See if this is a keyword. (This search is a bit n-squared.) */
    {
        unsigned int type = TOK_DOMString;
        p = KEYWORDS;
        for (;;) {
            unsigned int len = strlen(p);
            if (!len)
                break;
            if (len == tok.len && !memcmp(start, p, len)) {
                tok.type = type;
                break;
            }
            p += len + 1;
            type++;
        }
    }
    return &tok;
}

/***********************************************************************
 * lex : retrieve next token
 *
 * Return:  tok struct, lifetime until next call to lex
 */
struct tok *
lex(void)
{
    static struct tok tok;
    const char *p, *start;
    /* Multiple tries until we get a complete token in the buffer. */
    for (;;) {
        int ch;
        p = buf + pos;
        start = p;
        /* Flush whitespace. */
        while ((ch = *p) == ' ' || ch == '\t' || ch == '\r'
                || (ch == '\n' && ++linenum))
        {
            p++;
        }
        start = p;
        /* See if we have a comment. */
        if (ch == '/') {
            switch (*++p) {
            case 0:
                if (!eof)
                    goto readmore;
                break;
            case '*':
                return lexblockcomment(p - 1);
            case '/':
                return lexinlinecomment(p - 1);
            }
            tok.type = '/';
            break;
        }
        /* Handle things that start with '-', which is either '-' as a token,
         * or a number. Handle numbers. */
        if (ch == '-' || (unsigned)(ch - '0') < 10)
            return lexnumber(p);
        /* Handle string. */
        if (ch == '"')
            return lexstring(p);
        /* Handle identifier. */
        if (ch == '_' || (unsigned)((ch & ~0x20) - 'A') <= 'Z' - 'A')
            return lexidentifier(p);
        /* The only multi-symbol token is :: */
        if (ch == ':') {
            tok.type = ':';
            switch (*++p) {
            case 0:
                if (!eof)
                    goto readmore;
                break;
            case ':':
                tok.type = TOK_DOUBLECOLON;
                p++;
                break;
            }
            break;
        }
        if (!ch) {
            if (eof) {
                tok.type = TOK_EOF;
                tok.start = "end of file";
                tok.len = strlen(tok.start);
                return &tok;
            }
            goto readmore;
        }
        /* Single symbol token. */
        tok.type = ch;
        tok.linenum = linenum;
        tok.start = p;
        tok.len = 1;
        p++;
        break;
readmore:
        readmore(start, p);
    }
    pos = p - buf;
    return &tok;
}

