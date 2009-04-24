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

struct file {
    struct file *next;
    const char *filename;
    char *buf;
    const char *pos, *end;
    unsigned int linenum;
};

static struct file *file;

/***********************************************************************
 * readinput : read all input files into memory
 *
 * Enter:   argv = 0-terminated array of filenames
 */
void
readinput(const char *const *argv)
{
    struct file **pfile = &file;
    for (;;) {
        struct file *file;
        const char *filename = *argv++;
        char *buf = 0;
        int len = 0, thislen, isstdin;
        FILE *handle;
        if (!filename)
            break;
        /* Read the file. */
        isstdin = !strcmp(filename, "-");
        if (isstdin) {
            handle = stdin;
            filename = "<stdin>";
        } else {
            handle = fopen(filename, "rb");
            if (!handle)
                errorexit("%s: %s", filename, strerror(errno));
        }
        for (;;) {
            thislen = len ? len * 2 : 4096;
            buf = memrealloc(buf, len + thislen + 1);
            thislen = fread(buf + len, 1, thislen, handle);
            if (!thislen)
                break;
            len += thislen;
        }
        if (ferror(handle))
            errorexit("%s: I/O error", filename);
        if (!isstdin)
            fclose(handle);
        buf[len] = 0;
        buf = memrealloc(buf, len + 1);
        /* Create the file struct for it. */
        file = memalloc(sizeof(struct file));
        *pfile = file;
        pfile = &file->next;
        file->filename = filename;
        file->pos = file->buf = buf;
        file->end = buf + len;
        file->linenum = 0;
    }
    *pfile = 0;
}

/***********************************************************************
 * lexerrorexit : error and exit with line number
 */
static void
lexerrorexit(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(file->filename, file->linenum, format, ap);
    va_end(ap);
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
    tok.filename = file->filename;
    tok.linenum = file->linenum;
    for (;;) {
        int ch = *++p;
        if (!ch)
            lexerrorexit("unterminated block comment");
        if (ch != '*') {
            if (ch == '\n')
                file->linenum++;
            continue;
        }
        ch = p[1];
        if (!ch)
            lexerrorexit("unterminated block comment");
        if (ch == '/')
            break;
    }
    p += 2;
    file->pos = p;
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
        if (!ch || ch == '\n')
            break;
    }
    p++;
    file->pos = p;
    tok.type = TOK_INLINECOMMENT;
    tok.start = start + 2;
    tok.len = p - start - 2;
    tok.filename = file->filename;
    tok.linenum = file->linenum++;
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
        if (ch == '0') {
            state = STATE_OCTAL;
            ch = *++p;
            if ((ch & ~0x20) == 'X') {
                state = STATE_HEX;
                ch = *++p;
            }
        }
        for (;;) {
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
        tok.filename = file->filename;
        tok.linenum = file->linenum;
        file->pos = p;
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
        for (;;) {
            if (!ch || ch == '\n')
                lexerrorexit("unterminated string");
            if (ch == '"') {
                tok.type = TOK_STRING;
                tok.start = start + 1;
                tok.len = p - start - 1;
                tok.filename = file->filename;
                tok.linenum = file->linenum;
                file->pos = p + 1;
                return &tok;
            }
            /* Note the IDL spec doesn't seem to allow for escape sequences
             * in strings. */
            ch = *++p;
        }
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
        if (ch != '_' && (unsigned)(ch - '0') >= 10
                && (unsigned)((ch & ~0x20) - 'A') > 'Z' - 'A')
        {
            break;
        }
        p++;
    }
    tok.type = TOK_IDENTIFIER;
    tok.start = start;
    tok.len = p - start;
    tok.filename = file->filename;
    tok.linenum = file->linenum;
    file->pos = p;
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
    const char *p;
    int ch;
    for (;;) {
        if (!file) {
            tok.type = TOK_EOF;
            tok.start = "end of file";
            tok.len = strlen(tok.start);
            return &tok;
        }
        p = file->pos;
        /* Flush whitespace. */
        while ((ch = *p) == ' ' || ch == '\t' || ch == '\r'
                || (ch == '\n' && ++file->linenum))
        {
            p++;
        }
        if (ch)
            break;
        if (p != file->end)
            lexerrorexit("\\0 byte not allowed");
        file = file->next;
    }
    /* See if we have a comment. */
    if (ch == '/') {
        switch (*++p) {
        case '*':
            return lexblockcomment(p - 1);
        case '/':
            return lexinlinecomment(p - 1);
        }
        tok.type = '/';
    } else {
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
            if (*++p == ':') {
                tok.type = TOK_DOUBLECOLON;
                p++;
            }
        }
    }
    /* Single symbol token. */
    tok.type = ch;
    tok.filename = file->filename;
    tok.linenum = file->linenum;
    tok.start = p;
    tok.len = 1;
    p++;
    file->pos = p;
    return &tok;
}

