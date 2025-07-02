#include "treewalker.h"
#include "parser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef enum {
    VARIABLE_TYPE_NONE,
    VARIABLE_TYPE_NUM,
    VARIABLE_TYPE_STR,
} VARIABLE_TYPE;

typedef struct {
    char *identifier;
    VARIABLE_TYPE type;
    union {
        uint32_t number;
        char *str;
    };
} VARIABLE;

static VARIABLE *vars = NULL;
static size_t vars_size = 0;

VARIABLE* get_variable(char *identifier)
{
    for (size_t i = 0; i < vars_size; i++) {
        if (strcmp(identifier, vars[i].identifier) == 0) {
            return &vars[i];
        }
    }

    return NULL;
}

void set_num_variable(char *identifier, uint32_t num)
{
    VARIABLE *var = get_variable(identifier);
    if (var == NULL) {
        vars_size += 1;
        vars = realloc(vars, sizeof(VARIABLE) * vars_size);

        vars[vars_size - 1].type = VARIABLE_TYPE_NUM;

        vars[vars_size - 1].identifier = malloc(strlen(identifier) + 1);
        strcpy(vars[vars_size - 1].identifier, identifier);

        vars[vars_size - 1].number = num;
    } else {
        if (var->type == VARIABLE_TYPE_STR && var->str != NULL) {
            free(var->str);
        }

        var->type = VARIABLE_TYPE_NUM;
        var->number = num;
    }
}

void set_str_variable(char *identifier, char *str)
{
    VARIABLE *var = get_variable(identifier);
    if (var == NULL) {
        vars_size += 1;
        vars = realloc(vars, sizeof(VARIABLE) * vars_size);

        vars[vars_size - 1].type = VARIABLE_TYPE_STR;

        vars[vars_size - 1].identifier = malloc(strlen(identifier) + 1);
        strcpy(vars[vars_size - 1].identifier, identifier);

        vars[vars_size - 1].str = malloc(strlen(str) + 1);
        strcpy(vars[vars_size - 1].str, str);
    } else {
        if (var->type == VARIABLE_TYPE_STR && var->str != NULL) {
            free(var->str);
        }

        var->type = VARIABLE_TYPE_STR;
        var->str = malloc(strlen(str) + 1);
        strcpy(var->str, str);
    }
}

void print_all_variables()
{
    for (size_t i = 0; i < vars_size; i++) {
        printf("%s\n", vars[i].identifier);
        if (vars[i].type == VARIABLE_TYPE_NUM) {
            printf("\t%u\n", vars[i].number);
        } else if (vars[i].type == VARIABLE_TYPE_STR) {
            printf("\t%s\n", vars[i].str);
        }
    }
}

uint32_t execute_operator(PARSER_NODE* node)
{
    uint32_t left = 0;
    uint32_t right = 0;

    if (node->left->type == PARSER_TYPE_LITERAL) {
        if (node->left->literal != PARSER_LITERAL_NUMBER) {
            printf("Can't do other types yet\n");
            return 0;
        }

        left = node->left->number;
    } else if (node->left->type == PARSER_TYPE_OPERATOR) {
        left = execute_operator(node->left);
    } else {
        printf("can't do other nodes yet\n");
        return 0;
    }

    if (node->right->type == PARSER_TYPE_LITERAL) {
        if (node->right->literal != PARSER_LITERAL_NUMBER) {
            printf("unsupported type\n");
            return 0;
        }

        right = node->right->number;
    } else if (node->right->type == PARSER_TYPE_OPERATOR) {
        right = execute_operator(node->right);
    } else {
        printf("unsupported node\n");
        return 0;
    }

    switch (node->operator) {
        case PARSER_OPERATOR_ADD:
            return left + right;
        case PARSER_OPERATOR_SUBTRACT:
            return left - right;
        case PARSER_OPERATOR_MULTIPLY:
            return left * right;
        case PARSER_OPERATOR_DIVIDE:
            return left / right;
        default:
            printf("unsupported operator\n");
            return 0;
    }
}

void execute_node(PARSER_NODE *node)
{
    uint32_t result = 0;
    switch (node->type) {
        case PARSER_TYPE_ASSIGNMENT:
            if (node->right->type == PARSER_TYPE_OPERATOR) {
                result = execute_operator(node->right);
                set_num_variable(node->left->identifier, result);
            } else if (node->right->type == PARSER_TYPE_LITERAL) {
                if (node->right->literal == PARSER_LITERAL_NUMBER) {
                    set_num_variable(node->left->identifier, node->right->number);
                } else if (node->right->literal == PARSER_LITERAL_STRING) {
                    set_str_variable(node->left->identifier, node->right->string);
                }
            }
            break;
        case PARSER_TYPE_BODY:
            treewalk(&node->body);
            break;
        case PARSER_TYPE_CONDITIONAL:
            if (node->expression->type == PARSER_TYPE_OPERATOR) {
                result = execute_operator(node->expression);
            } else if (node->expression->type == PARSER_TYPE_LITERAL) {
                if (node->expression->literal == PARSER_LITERAL_NUMBER) {
                    result = node->expression->number;
                }
            }
            if (result != 0 && node->right != NULL) {
                execute_node(node->right);
            } else if (result == 0 && node->left != NULL) {
                execute_node(node->left);
            }
            break;
        default:
            printf("Onbekende node\n");
    }
}

void treewalk(PARSER_NODE_BODY *body)
{
    for (size_t i = 0; i < body->expressions_size; i++) {
        execute_node(body->expressions[i]);
    }

    printf("VARS:\n");
    print_all_variables();
}
