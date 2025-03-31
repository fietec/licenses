/* 
    =========================================
    conp.h <https://github.com/fietec/conp>
    =========================================
    Copyright (c) 2025 Constantijn de Meer

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef _CONP_H
#define _CONP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>

#define conp_is_whitespace(c) ((c == ' ' || c == '\t'))
#define conp_check_line(lexer, c) do{if (c == '\n'){(lexer)->loc.row++; (lexer)->loc.column=1;}else{lexer->loc.column++;}}while(0)
#define conp_inc(lexer) do{lexer->index++; lexer->loc.column++;}while(0)
#define conp_get_char(lexer) (lexer->buffer[lexer->index])
#define conp_get_pointer(lexer) (lexer->buffer + lexer->index)
#define conp_loc_expand(loc) (loc).filename, (loc).row, (loc).column
#define conp_token_args_len(...) sizeof(ConpTokenType[]){__VA_ARGS__}/sizeof(ConpTokenType)
#define conp_token_args_array(...) (ConpTokenType[]){__VA_ARGS__}, conp_token_args_len(__VA_ARGS__)

#define CONP_LOC_FMT "%s:%d:%d"
#define conp_arr_len(arr) ((arr)!= NULL ? sizeof((arr))/sizeof((arr)[0]):0)

#define CONP_VALUES ConpToken_Field, ConpToken_Int, ConpToken_Float, ConpToken_String, ConpToken_True, ConpToken_False

#define conp_ansi_rgb(r, g, b) ("\e[38;2;" #r ";" #g ";" #b "m") 
#define conp_ansi_end ("\e[0m") 
#define conp_error(msg, ...) (fprintf(stderr, "%s[ERROR] %s:%d " msg "\n%s", conp_ansi_rgb(196, 0, 0), __FILE__, __LINE__, ##__VA_ARGS__, conp_ansi_end))

typedef enum{
    ConpToken_Sep,
    ConpToken_NewLine,
    ConpToken_End,
    ConpToken_Field,
    ConpToken_String,
    ConpToken_Int,
    ConpToken_Float,
    ConpToken_True,
    ConpToken_False,
    ConpToken__Count
} ConpTokenType;

const char* const ConpTokenTypeNames[] = {
    [ConpToken_Field] = "Field",
    [ConpToken_Sep] = "Sep",
    [ConpToken_NewLine] = "NewLine",
    [ConpToken_String] = "String",
    [ConpToken_Int] = "Int",
    [ConpToken_Float] = "Float",
    [ConpToken_True] = "Bool",
    [ConpToken_False] = "Bool",
    [ConpToken_End] = "--END--"
};

_Static_assert(ConpToken__Count == conp_arr_len(ConpTokenTypeNames), "ConpTokenType count has changed!");

typedef struct{
    char *filename;
    size_t row;
    size_t column;
} ConpLoc;

typedef struct{
    ConpTokenType type;
    char *start;
    char *end;
    size_t len;
    ConpLoc loc;
} ConpToken;

typedef struct{
    ConpToken key;
    ConpToken value;
} ConpEntry;

typedef struct{
    char *buffer;
    size_t buffer_size;
    size_t index;
    ConpLoc loc;
} ConpLexer;

typedef struct{
    ConpEntry *items;
    size_t count;
    size_t capacity;
} ConpEntries;

#define conp_expect(lexer, token, ...) conp__expect(lexer, token, conp_token_args_array(__VA_ARGS__)) // fetch the next token and expect one of the given token types
ConpLexer conp_init(char *buffer, size_t buffer_size, char *buffer_name); // initilize the lexer
bool conp_next(ConpLexer *lexer, ConpToken *token); // fetch the next token
bool conp_extract(ConpToken *token, char *buffer, size_t buffer_size); // extract the content of a token into a buffer
void conp_print(ConpToken token); // print a token
void conp_print_token(ConpToken token);

bool conp_parse(ConpLexer *lexer, ConpEntry *entry);
bool conp_parse_all(ConpEntries *entries, char *buffer, size_t buffer_size, char *buffer_name);
void conp_entries_add(ConpEntries *entries, ConpEntry entry);
bool conp_entries_get(ConpEntries *entries, char *key, ConpToken *token);
bool conp_entries_iskey(ConpEntries *entries, char *key);

// these functions are used internally, there should be no reason to call them yourself
void conp__trim_left(ConpLexer *lexer);
bool conp__find(ConpLexer *lexer, char c);
void conp__set_token(ConpToken *token, ConpTokenType type, char *start, char *end, ConpLoc loc);
bool conp__is_delimeter(char c);
bool conp__is_int(char *s, char *e);
bool conp__is_float(char *s, char *e);
bool conp__expect(ConpLexer *lexer, ConpToken *token, ConpTokenType types[], size_t count);

#endif // _CONP_H

#ifdef CONP_IMPLEMENTATION

bool conp_parse(ConpLexer *lexer, ConpEntry *entry)
{
    if (lexer == NULL || entry == NULL) return false;
    ConpToken token;
    bool loop = true;
    while (loop){
        conp_next(lexer, &token);
        switch (token.type){
            case ConpToken_NewLine: continue;
            case ConpToken_End: return false;
            default: {
                loop = false;
            }
        }
    }
    entry->key = token;
    if (!conp_expect(lexer, &token, ConpToken_Sep)) return false;
    if (!conp_expect(lexer, &token, CONP_VALUES)) return false;
    entry->value = token;
    return true;
}

bool conp_parse_all(ConpEntries *entries, char *buffer, size_t buffer_size, char *buffer_name)
{
    if (entries == NULL || buffer == NULL) return false;
    ConpLexer lexer = conp_init(buffer, buffer_size, buffer_name);
    ConpEntry entry;
    while (conp_parse(&lexer, &entry)){
        conp_entries_add(entries, entry);
    }
    return true;
}

bool conp_entries_get(ConpEntries *entries, char *key, ConpToken *token)
{
    if (entries == NULL || key == NULL || token == NULL) return false;
    size_t key_len = strlen(key);
    for (size_t i=0; i<entries->count; ++i){
        ConpEntry entry = entries->items[i];
        if (key_len != entry.key.len) continue;
        if (memcmp(entry.key.start, key, key_len) == 0){
            *token = entry.value;
            return true;
        }
    }
    return false;
}

void conp_entries_add(ConpEntries *entries, ConpEntry entry)
{
    if (entries == NULL) return;
    if (entries->count >= entries->capacity){
        size_t capacity = entries->capacity;
        entries->capacity = (capacity == 0)? 32:capacity*2;
        entries->items = realloc(entries->items, entries->capacity*sizeof(*entries->items));
        assert(entries->items != NULL && "Need more RAM!");
    }
    entries->items[entries->count++] = entry;
}

bool conp_entries_iskey(ConpEntries *entries, char *key)
{
    if (entries == NULL || key == NULL) return false;
    size_t key_len = strlen(key);
    for (size_t i=0; i<entries->count; ++i){
        ConpToken ikey = entries->items[i].key;
        if (key_len == ikey.len && memcmp(key, ikey.start, key_len) == 0) return true;
    }
    return false;
}

ConpLexer conp_init(char *buffer, size_t buffer_size, char *buffer_name)
{
    return (ConpLexer) {.buffer=buffer, .buffer_size=buffer_size, .index=0, .loc=(ConpLoc){.filename=buffer_name, .row=1, .column=1}};
}

bool conp_next(ConpLexer *lexer, ConpToken *token)
{
    if (lexer == NULL || token == NULL || lexer->index > lexer->buffer_size) return false;
    conp__trim_left(lexer);
    char *start = conp_get_pointer(lexer);
    ConpLoc loc = lexer->loc;
    switch (conp_get_char(lexer)){
        case '=':{
            conp__set_token(token, ConpToken_Sep, start, start+1, loc);
        } break;
        case '\n':{
            lexer->loc.row++;
            lexer->loc.column = 0;
            conp__set_token(token, ConpToken_NewLine, start, start+1, loc);
        } break;
        case '"':{
            // lex strings
            conp_inc(lexer);
            char *s_start = conp_get_pointer(lexer);
            if (!conp__find(lexer, '"')){
                fprintf(stderr, "[ERROR] Missing closing delimeter for '\"' at " CONP_LOC_FMT "\n", conp_loc_expand(loc));
                return false;
            }
            char *s_end = conp_get_pointer(lexer);
            conp__set_token(token, ConpToken_String, s_start, s_end, loc);
            break;
        }
        case '\0':
        case EOF:{
            conp__set_token(token, ConpToken_End, start, start+1, loc);
            conp_inc(lexer);
            return false;
        }
        default:{
            // multi-character literal
            // find end of literal
            char c;
            while (lexer->index < lexer->buffer_size && !conp__is_delimeter(c = conp_get_char(lexer))){
                conp_check_line(lexer, c);
                lexer->index++;
            }
            char *end = conp_get_pointer(lexer);
            size_t t_len = end-start;
            // check for known literals
            if (memcmp(start, "true", t_len) == 0){
                conp__set_token(token, ConpToken_True, start, end, loc);
                return true;
            }
            if (memcmp(start, "false", t_len) == 0){
                conp__set_token(token, ConpToken_False, start, end, loc);
                return true;
            }
            if (conp__is_int(start, end)){
                conp__set_token(token, ConpToken_Int, start, end, loc);
                return true;
            }
            if (conp__is_float(start, end)){
                conp__set_token(token, ConpToken_Float, start, end, loc);
                return true;
            }
            conp__set_token(token, ConpToken_Field, start, end, loc);
            return true;
        }
    }
    conp_inc(lexer);
    return true;
}

bool conp__expect(ConpLexer *lexer, ConpToken *token, ConpTokenType types[], size_t count)
{
    if (lexer == NULL || token == NULL) return false;
    conp_next(lexer, token);
    for (size_t i=0; i<count; ++i){
        if (token->type == types[i]) return true;
    }
    fprintf(stderr, "[ERROR] "CONP_LOC_FMT " Expected token of type [", conp_loc_expand(token->loc));
    size_t i;
    for (i=0; i<count-1; ++i){
        fprintf(stderr, "%s, ", ConpTokenTypeNames[types[i]]);
    }
    fprintf(stderr, "%s], but got %s!\n", ConpTokenTypeNames[types[i]], ConpTokenTypeNames[token->type]);
    return false;  
}

bool conp_extract(ConpToken *token, char *buffer, size_t buffer_size)
{
    if (token == NULL || buffer == NULL || buffer_size == 0) return false;
    size_t t_len = token->end-token->start;
    if (t_len >= buffer_size) return false;
    if (token->type == ConpToken_String){
        char temp_buffer[t_len+1];
        memset(temp_buffer, 0, t_len+1);
        char *r = token->start;
        char *w = temp_buffer;
        while (r != token->end){
            if (*r == '\\'){
                switch(*++r){
                    case '\'': *w = 0x27; break;
					case '"':  *w = 0x22; break;
					case '?':  *w = 0x3f; break;
					case '\\': *w = 0x5c; break;
					case 'a':  *w = 0x07; break;
					case 'b':  *w = 0x08; break;
					case 'f':  *w = 0x0c; break;
					case 'n':  *w = 0x0a; break;
					case 'r':  *w = 0x0d; break;
					case 't':  *w = 0x09; break;
					case 'v':  *w = 0x0b; break;
                    default:{
                        *w++ = '\\';
                        *w = *r;
                    }
                }
            }
            else{
                *w = *r;
            }
            r++;
            w++;
        }
        sprintf(buffer, "%.*s", w-temp_buffer+1, temp_buffer);
    }
    else{
        sprintf(buffer, "%.*s", t_len, token->start);
    }
    return true;
}

void conp_print_token(ConpToken token)
{
    switch (token.type){
        case ConpToken_End:
        case ConpToken_Sep:
        case ConpToken_NewLine:{
            printf("%s", ConpTokenTypeNames[token.type]);
        } break;
        default:{
            printf("%s: '%.*s'", ConpTokenTypeNames[token.type], token.len, token.start);
        }
    }
}

void conp_print(ConpToken token)
{
    printf(CONP_LOC_FMT": ", conp_loc_expand(token.loc));
    conp_print_token(token);
    putchar('\n');
}

void conp__set_token(ConpToken *token, ConpTokenType type, char *start, char *end, ConpLoc loc)
{
    if (token == NULL) return;
    token->type = type;
    token->start = start;
    token->end = end;
    token->len = end-start;
    token->loc = loc;
}

bool conp__find(ConpLexer *lexer, char c)
{
    char rc;
    while (lexer->index < lexer->buffer_size){
        if ((rc = conp_get_char(lexer)) == c) return true;
        conp_check_line(lexer, rc);
        lexer->index++;
    }
    return false;
}

void conp__trim_left(ConpLexer *lexer)
{
    char c;
    while (lexer->index <= lexer->buffer_size && conp_is_whitespace((c = conp_get_char(lexer)))){
        conp_check_line(lexer, c);
        lexer->index++;
    }
}

bool conp__is_delimeter(char c)
{
    switch (c){
        case '=':
        case ' ':
        case '\n':
        case '\t':{
            return true;
        }
        default: return false;
    }
}

bool conp__is_int(char *s, char *e)
{
	if (!s || !e || e-s < 1) return false;
	if (*s == '-' || *s == '+') s++;
	while (s < e){
		if (!isdigit(*s++)) return false;
	}
	return true;
}

bool conp__is_float(char *s, char *e)
{
    char* ep = NULL;
    strtod(s, &ep);
    return (ep && ep == e);
}
#endif // CONP_IMPLEMENTATION