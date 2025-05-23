#include "lexer.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    LEX_SYMBOL *syms;
    size_t size;
    size_t allocated;
} sym_array;

static void sym_array_init(sym_array *s)
{
    s->size = 0;
    s->allocated = 128;
    s->syms = malloc(sizeof(LEX_SYMBOL) * s->allocated);
}

static void sym_array_add(sym_array *s, LEX_SYMBOL symbol)
{
    if (s->allocated == s->size) {
        s->allocated += 128;
        s->syms = realloc(s->syms, sizeof(LEX_SYMBOL) * s->allocated);
    }

    s->syms[s->size] = symbol;
    s->size++;
}

static bool end_of_string(int c)
{
    if (c == '"') {
        return false;
    }
    return true;
}

static bool end_of_name(int c)
{
    if (isalnum(c) || c == '-' || c == '_') {
        return true;
    }
    return false;
}

static bool end_of_number(int c)
{
    if (isxdigit(c)) {
        return true;
    }
    return false;
}

static char* parse_string(char *buf, size_t buf_size, size_t string_offset, bool (*is_valid)(int))
{
    size_t string_size = 0;
    size_t string_allocated = 128;
    char *string = malloc(string_allocated);
    while (string_offset + string_size < buf_size) {
        if (string_size == string_allocated) {
            string_allocated += 128;
            string = realloc(string, string_allocated);
        }

        if (!is_valid(buf[string_offset + string_size])) {
            string[string_size] = '\0';
            break;
        }

        string[string_size] = buf[string_offset + string_size];
        string_size++;
    }

    string = realloc(string, string_size);

    return string;
}

typedef struct {
    char *keyword;
    LEX_SYMBOOL_TYPE symbool;
} keyword;

const keyword keywords[] = {
    { .keyword = "functie", .symbool = LEX_SYM_FUNCTIE },
    { .keyword = "teruggave", .symbool = LEX_SYM_TERUGGAVE },
};

LEX_SYMBOOL_TYPE is_keyword(char *str) {
    size_t keywords_size = sizeof(keywords) / sizeof(keyword);
    for (size_t i = 0; i < keywords_size; i++) {
        // TEDOEN traag want stopt niet onmiddelijk bij ongelijke tekenreeks
        if (strcmp(keywords[i].keyword, str) == 0) {
            return keywords[i].symbool;
        }
    }
    return LEX_SYM_ONBEKEND;
}

LEX_SYMBOL* lex_parse_mem(char *buf, size_t bufsize, size_t *symbols_size)
{
    size_t syms_allocated = 128;
    sym_array syms;
    size_t i = 0;
    size_t huidige_regel = 1;
    char *tekenreeks = NULL;

    sym_array_init(&syms);

    sym_array_add(&syms, (LEX_SYMBOL){
        .type = LEX_SYM_REGEL, .nummer = huidige_regel
    });

    while (i < bufsize) {
        char current = buf[i];
        switch (current) {
            case '"':
                tekenreeks = parse_string(buf, bufsize, i+1, end_of_string);
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_TEKENREEKS,
                    .tekenreeks = tekenreeks
                });
                i += strlen(tekenreeks) + 1;
                tekenreeks = NULL;
                break;
            case '(':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_HAAK_OPEN
                });
                break;
            case ')':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_HAAK_SLUIT
                });
                break;
            case '{':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_ACCOLADE_OPEN
                });
                break;
            case '}':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_ACCOLADE_SLUIT
                });
                break;
            case ' ':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_SPATIE
                });
                break;
            case ';':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_PUNTKOMMA
                });
                break;
            case '=':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_IS
                });
                break;
            case '+':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_PLUS
                });
                break;
            case '-':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_MIN
                });
                break;
            case '*':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_KEER
                });
                break;
            case '/':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_DELEN
                });
                break;
            case '\n':
                sym_array_add(&syms, (LEX_SYMBOL){
                    .type = LEX_SYM_REGEL,
                    .nummer = ++huidige_regel
                });
                break;
            default:
                if (isalpha(current)) {
                    tekenreeks = parse_string(buf, bufsize, i, end_of_name);

                    LEX_SYMBOOL_TYPE symbool = is_keyword(tekenreeks);
                    if (symbool != 0) {
                        sym_array_add(&syms, (LEX_SYMBOL){
                            .type = symbool
                        });
                    } else {
                        sym_array_add(&syms, (LEX_SYMBOL){
                            .type = LEX_SYM_NAAM,
                            .tekenreeks = tekenreeks
                        });
                    }

                    i += strlen(tekenreeks);
                    tekenreeks = NULL;
                    continue;
                } else if (isdigit(current)) {
                    tekenreeks = parse_string(buf, bufsize, i, end_of_number);
                    sym_array_add(&syms, (LEX_SYMBOL){
                        .type = LEX_SYM_NUMMER,
                        .nummer = strtol(tekenreeks, NULL, 0)
                    });
                    i += strlen(tekenreeks);
                    tekenreeks = NULL;
                    continue;
                } else {
                    sym_array_add(&syms, (LEX_SYMBOL){
                        .type = LEX_SYM_ONBEKEND
                    });
                    break;
                }
        }
        i++;
    }

    *symbols_size = syms.size;
    return syms.syms;
}

void lex_debug_print(LEX_SYMBOL *symbols, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        LEX_SYMBOL symbool = symbols[i];
        switch (symbool.type) {
            case LEX_SYM_ONBEKEND:
                putchar('?');
                break;
            case LEX_SYM_SPATIE:
                putchar(' ');
                break;
            case LEX_SYM_TEKENREEKS:
                printf("\"%s\"", symbool.tekenreeks);
                break;
            case LEX_SYM_HAAK_OPEN:
                putchar('(');
                break;
            case LEX_SYM_HAAK_SLUIT:
                putchar(')');
                break;
            case LEX_SYM_ACCOLADE_OPEN:
                putchar('{');
                break;
            case LEX_SYM_ACCOLADE_SLUIT:
                putchar('}');
                break;
            case LEX_SYM_PUNTKOMMA:
                putchar(';');
                break;
            case LEX_SYM_IS:
                putchar('=');
                break;
            case LEX_SYM_PLUS:
                putchar('+');
                break;
            case LEX_SYM_MIN:
                putchar('-');
                break;
            case LEX_SYM_KEER:
                putchar('*');
                break;
            case LEX_SYM_DELEN:
                putchar('/');
                break;
            case LEX_SYM_FUNCTIE:
                printf("functie");
                break;
            case LEX_SYM_TERUGGAVE:
                printf("teruggave");
                break;
            case LEX_SYM_NAAM:
                printf("%s", symbool.tekenreeks);
                break;
            case LEX_SYM_NUMMER:
                printf("%ld", symbool.nummer);
                break;
            case LEX_SYM_REGEL:
                if (symbool.nummer != 1) {
                    putchar('\n');
                }
                printf("%li: ", symbool.nummer);
                break;
            default:
                printf("?%d", symbool.type);
                break;
        }
    }
    putchar('\n');
}
