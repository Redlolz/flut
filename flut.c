#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

void gebruik(FILE *restrict __stream, char *exec_naam)
{
    fprintf(stderr, "Gebruik: %s [BESTAND]\n", exec_naam);
}

int main(int argc, char *argv[])
{
    FILE *f;
    long fsize = 0;
    char *buf;

    if (argc < 2) {
        fprintf(stderr, "Geen bestand opgegeven\n");
        gebruik(stderr, argv[0]);
        return 1;
    }

    f = fopen(argv[1], "r");
    if (f == NULL) {
        fprintf(stderr, "Kan bestand niet openen\n");
        gebruik(stderr, argv[0]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // printf("%ld\n", fsize);

    buf = malloc(fsize);
    fread(buf, fsize, 1, f);
    fclose(f);

    LEX_SYMBOL *symbols = NULL;
    size_t symbols_size;
    symbols = lex_parse_mem(buf, fsize, &symbols_size);

    lex_debug_print(symbols, symbols_size);

    PARSER_NODE_BODY *body = parse(symbols, symbols_size);

    parser_debug_print(body);
}
