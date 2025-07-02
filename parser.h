#ifndef PARSER_H
#define PARSER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "lexer.h"

typedef enum {
    PARSER_TYPE_NONE,

    PARSER_TYPE_HEAD,
    PARSER_TYPE_BODY,

    PARSER_TYPE_IDENTIFIER,

    PARSER_TYPE_ASSIGNMENT,
    PARSER_TYPE_EXPRESSION,

    PARSER_TYPE_LITERAL,

    PARSER_TYPE_GROUPING,
    PARSER_TYPE_UNARY,
    PARSER_TYPE_BINARY,

    PARSER_TYPE_NEGATE,
    PARSER_TYPE_INVERT,

    PARSER_TYPE_OPERATOR,

    PARSER_TYPE_CONDITIONAL,
} PARSER_TYPE;

typedef enum {
    PARSER_LITERAL_STRING,
    PARSER_LITERAL_NUMBER,
    PARSER_LITERAL_BOOLEAN,
} PARSER_LITERAL;

typedef enum {
    PARSER_OPERATOR_ADD,
    PARSER_OPERATOR_SUBTRACT,
    PARSER_OPERATOR_MULTIPLY,
    PARSER_OPERATOR_DIVIDE,

    PARSER_OPERATOR_EQUAL_TO,
    PARSER_OPERATOR_NOT_EQUAL_TO,
    PARSER_OPERATOR_LOWER_THAN,
    PARSER_OPERATOR_LOWER_THAN_EQUAL_TO,
    PARSER_OPERATOR_HIGHER_THAN,
    PARSER_OPERATOR_HIGHER_THAN_EQUAL_TO,
} PARSER_OPERATOR;

typedef struct parser_node PARSER_NODE;

typedef struct parser_node_body {
    PARSER_NODE **expressions;
    size_t expressions_size;
} PARSER_NODE_BODY;

struct parser_node {
    PARSER_TYPE type;

    union {
        PARSER_LITERAL literal;
        PARSER_OPERATOR operator;
    };

    union {
        PARSER_NODE_BODY body;
        char *identifier;
        char *string;
        uint32_t number;
        bool boolean;
    };

    struct parser_node *expression;

    struct parser_node *left;
    struct parser_node *right;
};

PARSER_NODE_BODY* parser(LEX_SYMBOL *symbols, size_t symbols_size);
void parser_debug_print(PARSER_NODE_BODY *body);

#endif
