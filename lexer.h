#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    LEX_SYM_ONBEKEND,
    LEX_SYM_SPATIE,
    LEX_SYM_HAAK_OPEN,
    LEX_SYM_HAAK_SLUIT,
    LEX_SYM_ACCOLADE_OPEN,
    LEX_SYM_ACCOLADE_SLUIT,
    LEX_SYM_KOMMA,
    LEX_SYM_PUNTKOMMA,
    LEX_SYM_IS,
    LEX_SYM_PLUS,
    LEX_SYM_MIN,
    LEX_SYM_KEER,
    LEX_SYM_DELEN,

    LEX_SYM_FUNCTIE,
    LEX_SYM_TERUGGAVE,

    /* types met extra data */
    LEX_SYM_REGEL,
    LEX_SYM_NAAM,
    LEX_SYM_TEKENREEKS,
    LEX_SYM_NUMMER,
} LEX_SYMBOOL_TYPE;

typedef struct {
    LEX_SYMBOOL_TYPE type;
    union {
        char *tekenreeks;
        int64_t nummer;
    };
} LEX_SYMBOL;

LEX_SYMBOL* lex_parse_mem(char *buf, size_t bufsize, size_t *symbols_size);

void lex_debug_print(LEX_SYMBOL *symbols, size_t size);

#endif
