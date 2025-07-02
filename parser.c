#include "parser.h"
#include "lexer.h"

#include <inttypes.h>
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
    PRIORITY_NONE,
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
    node->left = NULL;
    node->right = NULL;

    switch (node->type) {
        case PARSER_TYPE_IDENTIFIER:
            node->identifier = malloc(strlen(symbol.tekenreeks) + 1);
            strcpy(node->identifier, symbol.tekenreeks);
            break;
        case PARSER_TYPE_OPERATOR:
            switch (symbol.type) {
                case LEX_SYM_PLUS:
                    node->operator = PARSER_OPERATOR_ADD;
                    break;
                case LEX_SYM_MIN:
                    node->operator = PARSER_OPERATOR_SUBTRACT;
                    break;
                case LEX_SYM_KEER:
                    node->operator = PARSER_OPERATOR_MULTIPLY;
                    break;
                case LEX_SYM_DELEN:
                    node->operator = PARSER_OPERATOR_DIVIDE;
                    break;
                case LEX_SYM_GELIJK_AAN:
                    node->operator = PARSER_OPERATOR_EQUAL_TO;
                    break;
                case LEX_SYM_NIET_GELIJK_AAN:
                    node->operator = PARSER_OPERATOR_NOT_EQUAL_TO;
                    break;
                case LEX_SYM_LAGER_DAN:
                    node->operator = PARSER_OPERATOR_LOWER_THAN;
                    break;
                case LEX_SYM_LAGER_DAN_GELIJK_AAN:
                    node->operator = PARSER_OPERATOR_LOWER_THAN_EQUAL_TO;
                    break;
                case LEX_SYM_HOGER_DAN:
                    node->operator = PARSER_OPERATOR_HIGHER_THAN;
                    break;
                case LEX_SYM_HOGER_DAN_GELIJK_AAN:
                    node->operator = PARSER_OPERATOR_HIGHER_THAN_EQUAL_TO;
                    break;
                default:
                    printf("Not an operator token!\n");
                    break;
            }
            break;
        case PARSER_TYPE_LITERAL:
            switch (symbol.type) {
                case LEX_SYM_NUMMER:
                    node->literal = PARSER_LITERAL_NUMBER;
                    node->number = symbol.nummer;
                    break;
                case LEX_SYM_TEKENREEKS:
                    node->literal = PARSER_LITERAL_STRING;
                    node->string = malloc(strlen(symbol.tekenreeks) + 1);
                    strcpy(node->string, symbol.tekenreeks);
                    break;
                case LEX_SYM_ONWAAR:
                    node->literal = PARSER_LITERAL_BOOLEAN;
                    node->boolean = false;
                    break;
                case LEX_SYM_WAAR:
                    node->literal = PARSER_LITERAL_BOOLEAN;
                    node->boolean = true;
                    break;
                default:
                    printf("Not a literal!\n");
                    break;
            }
            break;
        default:
            break;
    }

    return node;
}

PARSER_NODE* parse_rule(struct rule **rule, size_t rule_size, LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    PARSER_NODE *parent_node = NULL;

    size_t i = 0;
    while (i < rule_size) {
        skip_unimportant_symbols(symbols, symbols_size, symbols_index);

        PARSER_NODE *node = NULL;

        if (symbols[*symbols_index].type == LEX_SYM_PUNTKOMMA) {
            *symbols_index += 1;
            break;
        }

        if (rule[i]->type == RULE_TYPE_TERMINAL) {
            if (rule[i]->symbol == symbols[*symbols_index].type) {
                if (rule[i]->node_type != PARSER_TYPE_NONE) {
                    // From lexer format to parser
                    node = lexer_symbol_to_node(rule[i]->node_type, symbols[*symbols_index]);
                }
                *symbols_index += 1;
            }
        } else if (rule[i]->type == RULE_TYPE_NON_TERMINAL) {
            // Execute non-terminal
            node = rule[i]->func(symbols, symbols_size, symbols_index);
        } else if (rule[i]->type == RULE_TYPE_GROUP) {
            node = parse_rule(rule[i]->group, rule[i]->group_size, symbols, symbols_size, symbols_index);
        }

        // If not NULL, put it somewhere
        if (node != NULL && rule[i]->priority == PRIORITY_NONE) {

        } else if (node != NULL) {
            if (parent_node == NULL) {
                parent_node = node;
            } else if (node->right == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                node->right = parent_node;
                parent_node = node;
            } else if (node->left == NULL && rule[i]->priority == PRIORITY_PRIMARY) {
                node->left = parent_node;
                parent_node = node;
            } else if (parent_node->right == NULL && rule[i]->priority == PRIORITY_SECONDARY) {
                parent_node->right = node;
            } else if (parent_node->left == NULL) {
                parent_node->left = parent_node->right;
                parent_node->right = node;
            } else { // No available spots in parent node
                printf("hit!\n");
            }
        } else { // Rule not true
            // check if a following rule is an or and if so go to the rule after
            bool or_found = false;
            for (size_t x = i; x < rule_size; x++) {
                if (rule[x]->type == RULE_TYPE_OR) {
                    or_found = true;
                    i = x+1;
                    break;
                }
            }
            if (or_found) {
                if (parent_node == NULL) {
                    continue;
                } else {
                    return parent_node;
                }
            }

            // if a rule had a repeat we expected it to fail at some point
            if (rule[i]->repeat == REPEAT_ZERO_OR_MORE) {
                i++;
                continue;
            }

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

        // skip if rule repeats
        // TODO add check for REPEAT_ONE_OR_MORE
        if (rule[i]->repeat != REPEAT_ZERO_OR_MORE) {
            i++;
        }
    }

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

PARSER_NODE* parse_primary(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_NUMMER, PRIORITY_PRIMARY, PARSER_TYPE_LITERAL));
        ruleset_add(&ruleset, rule_create(RULE_TYPE_OR));
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_TEKENREEKS, PRIORITY_PRIMARY, PARSER_TYPE_LITERAL));
        ruleset_add(&ruleset, rule_create(RULE_TYPE_OR));
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_WAAR, PRIORITY_PRIMARY, PARSER_TYPE_LITERAL));
        ruleset_add(&ruleset, rule_create(RULE_TYPE_OR));
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_ONWAAR, PRIORITY_PRIMARY, PARSER_TYPE_LITERAL));
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE* parse_unary(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {
        struct rule *group = ruleset_add(&ruleset, rule_create(RULE_TYPE_GROUP));
        rule_add_to_group(group, rule_create_terminal(LEX_SYM_UITROEPTEKEN, PRIORITY_PRIMARY, PARSER_TYPE_INVERT));
        rule_add_to_group(group, rule_create(RULE_TYPE_OR));
        rule_add_to_group(group, rule_create_terminal(LEX_SYM_MIN, PRIORITY_PRIMARY, PARSER_TYPE_NEGATE));

        ruleset_add(&ruleset, rule_create_non_terminal(parse_unary, PRIORITY_SECONDARY));
        ruleset_add(&ruleset, rule_create(RULE_TYPE_OR));
        ruleset_add(&ruleset, rule_create_non_terminal(parse_primary, PRIORITY_SECONDARY));
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
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

PARSER_NODE* parse_comparison(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {
        // Add to ruleset
        ruleset_add(&ruleset, rule_create_non_terminal(parse_term, PRIORITY_SECONDARY));

        struct rule *group = ruleset_add(&ruleset, rule_create(RULE_TYPE_GROUP));

        struct rule *subgroup = rule_add_to_group(group, rule_create(RULE_TYPE_GROUP));
        rule_add_to_group(subgroup, rule_create_terminal(LEX_SYM_HOGER_DAN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));
        rule_add_to_group(subgroup, rule_create(RULE_TYPE_OR));
        rule_add_to_group(subgroup, rule_create_terminal(LEX_SYM_HOGER_DAN_GELIJK_AAN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));
        rule_add_to_group(subgroup, rule_create(RULE_TYPE_OR));
        rule_add_to_group(subgroup, rule_create_terminal(LEX_SYM_LAGER_DAN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));
        rule_add_to_group(subgroup, rule_create(RULE_TYPE_OR));
        rule_add_to_group(subgroup, rule_create_terminal(LEX_SYM_LAGER_DAN_GELIJK_AAN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));

        rule_add_to_group(group, rule_create_non_terminal(parse_term, PRIORITY_SECONDARY));
        rule_set_repeat(group, REPEAT_ZERO_OR_MORE);
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE* parse_equality(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {
        // Add to ruleset
        ruleset_add(&ruleset, rule_create_non_terminal(parse_comparison, PRIORITY_SECONDARY));

        struct rule *group = ruleset_add(&ruleset, rule_create(RULE_TYPE_GROUP));

        struct rule *subgroup = rule_add_to_group(group, rule_create(RULE_TYPE_GROUP));
        rule_add_to_group(subgroup, rule_create_terminal(LEX_SYM_NIET_GELIJK_AAN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));
        rule_add_to_group(subgroup, rule_create(RULE_TYPE_OR));
        rule_add_to_group(subgroup, rule_create_terminal(LEX_SYM_GELIJK_AAN, PRIORITY_PRIMARY, PARSER_TYPE_OPERATOR));

        rule_add_to_group(group, rule_create_non_terminal(parse_comparison, PRIORITY_SECONDARY));
        rule_set_repeat(group, REPEAT_ZERO_OR_MORE);
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE* parse_expression(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    return parse_equality(symbols, symbols_size, symbols_index);
}

PARSER_NODE* parse_assignment(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    static struct ruleset ruleset = { .rule = NULL, .size = 0 };
    if (ruleset.rule == NULL) {
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_NAAM, PRIORITY_SECONDARY, PARSER_TYPE_IDENTIFIER));
        ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_IS, PRIORITY_PRIMARY, PARSER_TYPE_ASSIGNMENT));
        ruleset_add(&ruleset, rule_create_non_terminal(parse_expression, PRIORITY_SECONDARY));
        // ruleset_add(&ruleset, rule_create_terminal(LEX_SYM_PUNTKOMMA, PRIORITY_NONE, PARSER_TYPE_NONE));
    }

    return parse_rule(ruleset.rule, ruleset.size, symbols, symbols_size, symbols_index);
}

PARSER_NODE_BODY* parse(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index);

PARSER_NODE* parse_if(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    if (*symbols_index >= symbols_size) return NULL;

    if (symbols[*symbols_index].type != LEX_SYM_ALS) return NULL;
    *symbols_index += 1;

    skip_unimportant_symbols(symbols, symbols_size, symbols_index);

    PARSER_NODE *expression = parse_expression(symbols, symbols_size, symbols_index);
    if (expression == NULL) return NULL;

    skip_unimportant_symbols(symbols, symbols_size, symbols_index);

    if (*symbols_index >= symbols_size) return NULL;

    if (symbols[*symbols_index].type != LEX_SYM_ACCOLADE_OPEN) return NULL;
    *symbols_index += 1;

    PARSER_NODE_BODY *true_body = parse(symbols, symbols_size, symbols_index);

    skip_unimportant_symbols(symbols, symbols_size, symbols_index);
    if (symbols[*symbols_index].type != LEX_SYM_ACCOLADE_SLUIT) return NULL;

    PARSER_NODE *node = malloc(sizeof(PARSER_NODE));
    node->type = PARSER_TYPE_CONDITIONAL;
    node->expression = expression;
    node->left = NULL;
    node->right = NULL;

    PARSER_NODE *true_node = malloc(sizeof(PARSER_NODE));
    true_node->type = PARSER_TYPE_BODY;
    true_node->right = NULL;
    true_node->body = *true_body;
    node->right = true_node;

    return node;
}

PARSER_NODE_BODY* parse(LEX_SYMBOL *symbols, size_t symbols_size, size_t *symbols_index)
{
    PARSER_NODE_BODY *body = malloc(sizeof(PARSER_NODE_BODY));
    body->expressions = NULL;
    body->expressions_size = 0;

    PARSER_NODE* (*rule_funcs[])(LEX_SYMBOL*, size_t, size_t*) = {
        parse_if,
        parse_assignment,
    };
    size_t rule_funcs_size = sizeof(rule_funcs) / sizeof(rule_funcs[0]);

    while (*symbols_index < symbols_size) {
        skip_unimportant_symbols(symbols, symbols_size, symbols_index);
        // if (symbols[*symbols_index].type != LEX_SYM_PUNTKOMMA && body->expressions_size > 0) {
        //     printf("%d\n", symbols[*symbols_index].type);
        //     printf("error here\n");
        // }

        // if (body->expressions_size > 0) {
        //     symbols_index++;
        // }

        PARSER_NODE *current_node = NULL;
        for (size_t i = 0; i < rule_funcs_size; i++) {
            current_node = (rule_funcs[i])(symbols, symbols_size, symbols_index);
            if (current_node != NULL) {
                body->expressions = realloc(body->expressions, sizeof(PARSER_NODE*) * body->expressions_size + 1);
                body->expressions[body->expressions_size++] = current_node;
                break;
            }
        }
        if (current_node == NULL) {
            // error here
            break;
        }
    }

    return body;
}

PARSER_NODE_BODY* parser(LEX_SYMBOL *symbols, size_t symbols_size)
{
    size_t symbols_index = 0;
    return parse(symbols, symbols_size, &symbols_index);
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
        case PARSER_TYPE_NEGATE:
            return "PARSER_TYPE_NEGATE";
        case PARSER_TYPE_INVERT:
            return "PARSER_TYPE_INVERT";
        case PARSER_TYPE_OPERATOR:
            return "PARSER_TYPE_OPERATOR";
        case PARSER_TYPE_CONDITIONAL:
            return "PARSER_TYPE_CONDITIONAL";
        default:
            return "UNKNOWN TYPE";
    }
}

static void recursive_node_print(PARSER_NODE *node, int level)
{
    printf("%s ", get_parser_type(node->type));
    if (node->type == PARSER_TYPE_LITERAL) {
        if (node->literal == PARSER_LITERAL_NUMBER) {
            printf("%i", node->number);
        } else if (node->literal == PARSER_LITERAL_BOOLEAN) {
            printf("%s", node->boolean ? "waar" : "onwaar");
        } else if (node->literal == PARSER_LITERAL_STRING) {
            printf("\"%s\"", node->string);
        }
    } else if (node->type == PARSER_TYPE_IDENTIFIER) {
        printf("%s", node->identifier);
    } else if (node->type == PARSER_TYPE_OPERATOR) {
        switch (node->operator) {
            case PARSER_OPERATOR_ADD: printf("+"); break;
            case PARSER_OPERATOR_SUBTRACT: printf("-"); break;
            case PARSER_OPERATOR_MULTIPLY: printf("*"); break;
            case PARSER_OPERATOR_DIVIDE: printf("/"); break;
            case PARSER_OPERATOR_EQUAL_TO: printf("=="); break;
            case PARSER_OPERATOR_NOT_EQUAL_TO: printf("!="); break;
            case PARSER_OPERATOR_LOWER_THAN: printf("<"); break;
            case PARSER_OPERATOR_LOWER_THAN_EQUAL_TO: printf("<="); break;
            case PARSER_OPERATOR_HIGHER_THAN: printf(">"); break;
            case PARSER_OPERATOR_HIGHER_THAN_EQUAL_TO: printf(">="); break;
        }
    } else if (node->type == PARSER_TYPE_CONDITIONAL) {
        printf("\n%*sCondition: ", level*4, "");
        recursive_node_print(node->expression, level+1);
    } else if (node->type == PARSER_TYPE_BODY) {
        printf("\n");
        for (size_t i = 0; i < node->body.expressions_size; i++) {
            printf("%*s", level*4, "");
            // printf("%p\n", (void*)body->expressions[i]);
            if (node->body.expressions[i] != NULL) {
                recursive_node_print(node->body.expressions[i], level+1);
            } else {
                printf("NULL\n");
            }
        }
    }
    printf("\n");

    if (node->left != NULL) {
        printf("%*sLeft Node: ", level*4, "");
        recursive_node_print(node->left, level+1);
    }

    if (node->right != NULL) {
        printf("%*sRight Node: ", level*4, "");
        recursive_node_print(node->right, level+1);
    }
}

void parser_debug_print(PARSER_NODE_BODY *body)
{
    for (size_t i = 0; i < body->expressions_size; i++) {
        // printf("%p\n", (void*)body->expressions[i]);
        if (body->expressions[i] != NULL) {
            recursive_node_print(body->expressions[i], 1);
        } else {
            printf("NULL\n");
        }
    }
}
