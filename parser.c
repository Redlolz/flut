#include "parser.h"
#include "lexer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    RULE_TYPE_TERMINAL,
    RULE_TYPE_NON_TERMINAL,
    RULE_TYPE_GROUP,
    RULE_TYPE_OR,
} RULE_TYPE;

typedef enum {
    REPEAT_NONE,
    REPEAT_ZERO_OR_MORE,
    REPEAT_ONE_OR_MORE,
} REPEAT;

typedef enum {
    PRIORITY_PRIMARY,
    PRIORITY_SECONDARY,
} PRIORITY;

struct rule {
    RULE_TYPE type;
    LEX_SYMBOOL_TYPE symbol;
    REPEAT repeat;
    PARSER_NODE* (*func)(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index);

    PRIORITY priority;
    PARSER_TYPE node_type;

    struct rule **group;
    size_t group_size;
};

struct rule* rule_create(RULE_TYPE type)
{
    struct rule *rule = malloc(sizeof(struct rule));
    rule->type = type;

    rule->repeat = REPEAT_NONE;

    rule->group = NULL;
    rule->group_size = 0;

    return rule;
}

struct rule* rule_create_terminal(LEX_SYMBOOL_TYPE symbol, PRIORITY place, PARSER_TYPE node_type)
{
    struct rule *rule = rule_create(RULE_TYPE_TERMINAL);

    rule->repeat = REPEAT_NONE;

    rule->symbol = symbol;
    rule->priority = place;
    rule->node_type = node_type;

    return rule;
}

struct rule* rule_create_non_terminal(PARSER_NODE* (*func)(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index), PRIORITY priority)
{
    struct rule *rule = rule_create(RULE_TYPE_NON_TERMINAL);

    rule->repeat = REPEAT_NONE;

    rule->func = func;
    rule->priority = priority;

    return rule;
}

struct rule* rule_add_to_group(struct rule* rule, struct rule* subrule)
{
    rule->group_size += 1;
    rule->group = realloc(rule->group, sizeof(struct rule*) * rule->group_size);
    rule->group[rule->group_size-1] = subrule;

    return subrule;
}

void rule_set_repeat(struct rule* rule, REPEAT repeat)
{
    rule->repeat = repeat;
}

PARSER_NODE* parse_expression(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index);

bool unimportant_symbol(LEX_SYMBOOL_TYPE symbol)
{
    switch (symbol) {
        case LEX_SYM_SPATIE:
        case LEX_SYM_REGEL:
            return true;
        default:
            return false;
    }
}

void skip_unimportant_symbols(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    while (*symbols_index < symbols_size) {
        if (unimportant_symbol(symbols[*symbols_index].type)) {
            *symbols_index += 1;
        } else {
            return;
        }
    }
}

PARSER_NODE* lexer_symbol_to_node(PARSER_TYPE type, LEX_SYMBOL symbol)
{
    PARSER_NODE *node = malloc(sizeof(PARSER_NODE));
    node->type = type;

    switch (node->type) {
        case PARSER_TYPE_IDENTIFIER:
            node->identifier = malloc(strlen(symbol.tekenreeks) + 1);
            strcpy(node->identifier, symbol.tekenreeks);
            break;
        default:
            break;
    }

    return node;
}

PARSER_NODE* parse_rule(struct rule **rule, size_t rule_size, LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    PARSER_NODE *parent_node = NULL;
    // PARSER_NODE *left_node = NULL;
    // PARSER_NODE *right_node = NULL;

    size_t i = 0;
    while (i < rule_size) {
        skip_unimportant_symbols(symbols, symbols_size, symbols_index);
        if (rule[i]->type == RULE_TYPE_TERMINAL) {
            // Eat spaces and newlines and stuff here
            if (rule[i]->symbol != symbols[*symbols_index].type) {
                // check if the next rule is an or and if so go to the rule after
                if (i + 1 < rule_size) {
                    if (rule[i+1]->type == RULE_TYPE_OR) {
                        i+=2;
                        continue;
                    }
                }
                // Rule not true
                if (parent_node != NULL) {
                    if (parent_node->left != NULL) {
                        free(parent_node->left);
                    }
                    if (parent_node->right != NULL) {
                        free(parent_node->right);
                    }
                    free(parent_node);
                }
                return NULL;
            }

            // From lexer format to parser
            PARSER_NODE *terminal = lexer_symbol_to_node(rule[i]->node_type, symbols[*symbols_index]);
            if (parent_node == NULL) {
                parent_node = terminal;
            } else if (terminal->right == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                terminal->right = parent_node;
                parent_node = terminal;
            } else if (terminal->left == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                terminal->left = parent_node;
                parent_node = terminal;
            } else if (terminal->right == NULL && rule[i]->priority == PRIORITY_SECONDARY) {
                parent_node->right = terminal;
            } else if (parent_node->left == NULL) {
                parent_node->left = parent_node->right;
                parent_node->right = terminal;
            }

            // if (rule[i]->priority == PLACE_LEFT) {
            //     left_node = terminal;
            // } else if (rule[i]->priority == PLACE_RIGHT) {
            //     right_node = terminal;
            // } else if (rule[i]->priority == PLACE_PARENT) {
            //     parent_node = terminal;
            // }
            *symbols_index += 1;
        } else if (rule[i]->type == RULE_TYPE_NON_TERMINAL) {
            // Execute non-terminal
            PARSER_NODE *non_terminal = rule[i]->func(symbols, symbols_size, symbols_index);
            if (parent_node == NULL) {
                parent_node = non_terminal;
            } else if (non_terminal->right == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                non_terminal->right = parent_node;
                parent_node = non_terminal;
            } else if (non_terminal->left == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                non_terminal->left = parent_node;
                parent_node = non_terminal;
            } else if (non_terminal->right == NULL && rule[i]->priority == PRIORITY_SECONDARY) {
                parent_node->right = non_terminal;
            } else if (parent_node->left == NULL) {
                parent_node->left = parent_node->right;
                parent_node->right = non_terminal;
            }
        } else if (rule[i]->type == RULE_TYPE_GROUP) {
            PARSER_NODE *group = parse_rule(rule[i]->group, rule[i]->group_size, symbols, symbols_size, symbols_index);
            if (group != NULL) {
                if (parent_node == NULL) {
                    parent_node = group;
                } else if (group->right == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                    group->right = parent_node;
                    parent_node = group;
                } else if (group->left == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                    group->left = parent_node;
                    parent_node = group;
                } else if (group->right == NULL && rule[i]->priority == PRIORITY_SECONDARY) {
                    parent_node->right = group;
                } else if (parent_node->left == NULL) {
                    parent_node->left = parent_node->right;
                    parent_node->right = group;
                }
            }
        }

        // skip if rule repeats
        i++;
    }

    if (parent_node == NULL) {
        return NULL;
    }

    // if (parent_node->left == NULL) {
    //     parent_node->left = left_node;
    // }
    // if (parent_node->right == NULL) {
    //     parent_node->right = right_node;
    // }

    return parent_node;
}

PARSER_NODE* parse_operator(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    PARSER_OPERATOR operator;
    switch (symbols[*symbols_index].type) {
        case LEX_SYM_PLUS:
            operator = PARSER_OPERATOR_ADD;
            break;
        case LEX_SYM_MIN:
            operator = PARSER_OPERATOR_SUBTRACT;
            break;
        case LEX_SYM_KEER:
            operator = PARSER_OPERATOR_MULTIPLY;
            break;
        case LEX_SYM_DELEN:
            operator = PARSER_OPERATOR_DIVIDE;
            break;
        default:
            return NULL;
    }

    PARSER_NODE *node = malloc(sizeof(PARSER_NODE));
    node->type = PARSER_TYPE_OPERATOR;
    node->operator = operator;

    return node;
}

PARSER_NODE* parse_primary(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    PARSER_LITERAL literal;
    char *string = NULL;
    uint32_t number;

    LEX_SYMBOL symbol = symbols[*symbols_index];
    switch (symbol.type) {
        case LEX_SYM_TEKENREEKS:
            literal = PARSER_LITERAL_STRING;
            string = malloc(strlen(symbol.tekenreeks) + 1);
            strcpy(string, symbol.tekenreeks);
            break;
        case LEX_SYM_NUMMER:
            literal = PARSER_LITERAL_NUMBER;
            number = symbol.nummer;
            break;
        case LEX_SYM_HAAK_OPEN:
            // TODO parse expression here
        default:
            return NULL;
    }

    PARSER_NODE *node = malloc(sizeof(PARSER_NODE));
    node->type = PARSER_TYPE_LITERAL;
    node->literal = literal;

    if (string != NULL) {
        node->string = string;
    } else {
        node->number = number;
    }

    *symbols_index += 1;

    return node;
}

PARSER_NODE* parse_unary(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    struct rule *rule[] = {
        rule_create_non_terminal(parse_primary, PRIORITY_PRIMARY),
    };

    return parse_rule(rule, sizeof(rule) / sizeof(struct rule*), symbols, symbols_size, symbols_index);
}

struct ruleset {
    struct rule **rule;
    size_t size;
};

struct rule* ruleset_add(struct ruleset *ruleset, struct rule *rule)
{
    ruleset->size += 1;
    ruleset->rule = realloc(ruleset->rule, sizeof(struct rule*) * ruleset->size);
    ruleset->rule[ruleset->size-1] = rule;
    return rule;
}

PARSER_NODE* parse_factor(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {

        // Add to ruleset
        ruleset_add(&ruleset, rule_create_non_terminal(parse_unary, PRIORITY_SECONDARY));

        struct rule *group = ruleset_add(&ruleset, rule_create(RULE_TYPE_GROUP));

        struct rule *div_or_mul = rule_add_to_group(group, rule_create(RULE_TYPE_GROUP));
        rule_add_to_group(div_or_mul, rule_create_terminal(LEX_SYM_DELEN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));
        rule_add_to_group(div_or_mul, rule_create(RULE_TYPE_OR));
        rule_add_to_group(div_or_mul, rule_create_terminal(LEX_SYM_KEER, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));

        rule_add_to_group(group, rule_create_non_terminal(parse_unary, PRIORITY_SECONDARY));
        rule_set_repeat(group, REPEAT_ZERO_OR_MORE);
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE* parse_term(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {

        // Add to ruleset
        ruleset_add(&ruleset, rule_create_non_terminal(parse_factor, PRIORITY_SECONDARY));

        struct rule *group = ruleset_add(&ruleset, rule_create(RULE_TYPE_GROUP));

        struct rule *add_or_sub = rule_add_to_group(group, rule_create(RULE_TYPE_GROUP));
        rule_add_to_group(add_or_sub, rule_create_terminal(LEX_SYM_MIN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));
        rule_add_to_group(add_or_sub, rule_create(RULE_TYPE_OR));
        rule_add_to_group(add_or_sub, rule_create_terminal(LEX_SYM_PLUS, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));

        rule_add_to_group(group, rule_create_non_terminal(parse_factor, PRIORITY_SECONDARY));
        rule_set_repeat(group, REPEAT_ZERO_OR_MORE);
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE* parse_equality(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    //equality ::= comparison ( ( "!=" | "==" ) comparison )* ;
    struct rule rule[] = {
        {}
    };
}

PARSER_NODE* parse_expression(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    PARSER_NODE *node = NULL;

    skip_unimportant_symbols(symbols, symbols_size, symbols_index);

    // literal
    node = parse_term(symbols, symbols_size, symbols_index);
    if (node != NULL) { return node; }

    // unary


    // grouping


    return node;
}

PARSER_NODE* parse_assignment(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_NAAM, PRIORITY_SECONDARY, PARSER_TYPE_IDENTIFIER));
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_IS, PRIORITY_PRIMARY, PARSER_TYPE_ASSIGNMENT));
        ruleset_add(&ruleset, rule_create_non_terminal(parse_expression, PRIORITY_SECONDARY));
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE_BODY* parse(LEX_SYMBOL *symbols, size_t symbols_size)
{
    PARSER_NODE_BODY *body = malloc(sizeof(PARSER_NODE_BODY));
    body->expressions = malloc(sizeof(PARSER_NODE*));
    body->expressions_size = 1;
    size_t symbols_index = 0;

    body->expressions[0] = parse_assignment(symbols, symbols_size, &symbols_index);

    return body;
}

static char* get_parser_type(PARSER_TYPE type)
{
    switch (type) {
        case PARSER_TYPE_HEAD:
            return "PARSER_TYPE_HEAD";
        case PARSER_TYPE_BODY:
            return "PARSER_TYPE_BODY";
        case PARSER_TYPE_IDENTIFIER:
            return "PARSER_TYPE_IDENTIFIER";
        case PARSER_TYPE_ASSIGNMENT:
            return "PARSER_TYPE_ASSIGNMENT";
        case PARSER_TYPE_EXPRESSION:
            return "PARSER_TYPE_EXPRESSION";
        case PARSER_TYPE_LITERAL:
            return "PARSER_TYPE_LITERAL";
        case PARSER_TYPE_GROUPING:
            return "PARSER_TYPE_GROUPING";
        case PARSER_TYPE_UNARY:
            return "PARSER_TYPE_UNARY";
        case PARSER_TYPE_BINARY:
            return "PARSER_TYPE_BINARY";
        case PARSER_TYPE_OPERATOR:
            return "PARSER_TYPE_OPERATOR";
        default:
            return "UNKNOWN TYPE";
    }
}

static void recursive_node_print(PARSER_NODE *node, int level)
{
    printf("%*s%s\n", level*4, "", get_parser_type(node->type));
    if (node->type == PARSER_TYPE_LITERAL) {
        if (node->literal == PARSER_LITERAL_NUMBER) {
            printf("%*s  %i\n", level*4, "", node->number);
        }
    }
    if (node->left != NULL) {
        printf("%*sLeft Node:", level*4, "");
        recursive_node_print(node->left, level+1);
    }

    if (node->right != NULL) {
        printf("%*sRight Node:", level*4, "");
        recursive_node_print(node->right, level+1);
    }
}

void parser_debug_print(PARSER_NODE_BODY *body)
{
    for (size_t i = 0; i < body->expressions_size; i++) {
        printf("%p\n", (void*)body->expressions[i]);
        recursive_node_print(body->expressions[i], 0);
    }
}
