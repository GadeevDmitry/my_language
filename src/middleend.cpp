#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../lib/logs/log.h"
#include "../lib/read_write/read_write.h"
#include "../lib/algorithm/algorithm.h"

#include "middleend.h"
#include "terminal_colors.h"

#define fprintf_err(message) fprintf(stderr, TERMINAL_RED "ERROR: " TERMINAL_CANCEL "%s", message)

//===========================================================================================================================
// MAIN
//===========================================================================================================================

int main(const int argc, const char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "you should give one parameter: name of ast format file to optimize\n");
        return false;
    }

    int buff_size    = 0;
    int buff_pos     = 0;
    const char *buff = (const char *) read_file(argv[1], &buff_size);

    if (!parse_name_list(buff, buff_size, &buff_pos)) { log_free((char *) buff); return 0; } // variable names parse
    if (!parse_name_list(buff, buff_size, &buff_pos)) { log_free((char *) buff); return 0; } // function names parse

    const int name_info_size = buff_pos;

    AST_node *tree = AST_parse(buff, buff_size, &buff_pos);
    if       (tree == nullptr)
    {
        log_free((char *) buff);
        return 0;
    }
    AST_tree_graphviz_dump(tree);
    optimize_ast(tree);
    AST_tree_graphviz_dump(tree);

    FILE *stream = fopen(argv[1], "w");

    fwrite     (buff, sizeof(char), (size_t) name_info_size, stream);
    fprintf    (stream, "\n");
    AST_convert(tree, stream);

    AST_tree_dtor(tree);
    fclose       (stream);
    log_free     ((char *) buff);
}

//===========================================================================================================================
// OPTIMIZE
//===========================================================================================================================

void optimize_ast(AST_node *const node)
{
    if (node == nullptr) return;

    optimize_ast(L);
    optimize_ast(R);

    if ($type != OPERATOR) return;

    switch ($op_type)
    {
        case OP_ADD        : optimize_add(node); break;
        case OP_SUB        : optimize_sub(node); break;
        case OP_MUL        : optimize_mul(node); break;
        case OP_DIV        : optimize_div(node); break;
        case OP_POW        : optimize_pow(node); break;

        case OP_EQUAL      : optimize_equal_type    (node); break;
        case OP_ABOVE      : optimize_above_type    (node); break;
        case OP_BELOW      : optimize_below_type    (node); break;
        case OP_ABOVE_EQUAL: optimize_above_eq_type (node); break;
        case OP_BELOW_EQUAL: optimize_below_eq_type (node); break;
        case OP_NOT_EQUAL  : optimize_not_equal_type(node); break;

        case OP_OR         : optimize_or (node); break;
        case OP_AND        : optimize_and(node); break;

        case OP_SIN        : optimize_sin (node); break;
        case OP_SQRT       : optimize_sqrt(node); break;
        case OP_NOT        : optimize_not (node); break;

        case OP_INPUT      :
        case OP_OUTPUT     :
        case ASSIGNMENT    : break;
        default            : assert(false && "default case in optimize_ast()"); return;
    }

    return;
}

void optimize_add(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_ADD);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, l_op + r_op, nullptr, nullptr, P);
}

void optimize_sub(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_SUB);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, l_op - r_op, nullptr, nullptr, P);
}

void optimize_mul(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_MUL);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, l_op * r_op, nullptr, nullptr, P);
}

void optimize_div(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_DIV);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    if (approx_equal(r_op, 0)) return;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, l_op / r_op, nullptr, nullptr, P);
}

void optimize_pow(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_POW);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    if (l_op < 0 || approx_equal(l_op, 0)) return;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, pow(l_op, r_op), nullptr, nullptr, P);
}

void optimize_and(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_AND);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, !approx_equal(l_op, 0) && !approx_equal(r_op, 0), nullptr, nullptr, P);
}

void optimize_or(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_OR);

    if (L->type != NUMBER) return;
    if (R->type != NUMBER) return;

    const double l_op = L->value.dbl_num;
    const double r_op = R->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, !approx_equal(l_op, 0) || !approx_equal(r_op, 0), nullptr, nullptr, P);
}

#define optimize_cmp(cmp_type, cmp_type_enum, optimized_val)                                                                \
void    optimize_##cmp_type##_type(AST_node *const node)                                                                    \
{                                                                                                                           \
    assert(node     != nullptr);                                                                                            \
    assert($type    == OPERATOR);                                                                                           \
    assert($op_type == cmp_type_enum);                                                                                      \
                                                                                                                            \
    if (L->type != NUMBER) return;                                                                                          \
    if (R->type != NUMBER) return;                                                                                          \
                                                                                                                            \
    const double l_op = L->value.dbl_num;                                                                                   \
    const double r_op = R->value.dbl_num;                                                                                   \
                                                                                                                            \
    AST_tree_dtor(L);                                                                                                       \
    AST_tree_dtor(R);                                                                                                       \
                                                                                                                            \
    AST_node_NUMBER_ctor(node, optimized_val, nullptr, nullptr, P);                                                         \
}

optimize_cmp(equal    , OP_EQUAL      ,  approx_equal(l_op, r_op))
optimize_cmp(not_equal, OP_NOT_EQUAL  , !approx_equal(l_op, r_op))
optimize_cmp(above    , OP_ABOVE      , (l_op >  r_op))
optimize_cmp(below    , OP_BELOW      , (l_op <  r_op))
optimize_cmp(above_eq , OP_ABOVE_EQUAL, (l_op >  r_op) || approx_equal(l_op, r_op))
optimize_cmp(below_eq , OP_BELOW_EQUAL, (l_op <  r_op) || approx_equal(l_op, r_op))

#undef optimize_cmp

void optimize_sqrt(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_SQRT);

    if (L->type != NUMBER) return;

    const double l_op = L->value.dbl_num;

    if (l_op < 0) return;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, sqrt(l_op), nullptr, nullptr, P);
}

void optimize_sin(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_SIN);

    if (L->type != NUMBER) return;

    const double l_op = L->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, sin(l_op), nullptr, nullptr, P);
}

void optimize_not(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_NOT);

    if (L->type != NUMBER) return;

    const double l_op = L->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, approx_equal(l_op, 0), nullptr, nullptr, P);
}

//===========================================================================================================================
// PARSE
//===========================================================================================================================

bool parse_name_list(const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    int name_num = 0;
    if (!get_buff_int(buff, buff_size, buff_pos, &name_num))
    {
        fprintf_err("middleend parse: expected number of names\n");
        return false;
    }
    if (name_num < 0)
    {
        fprintf_err("middleend parse: invalid number of names\n");
        return false;
    }

    for (int i = 0; i < name_num; ++i)
    {
        if (!get_ast_word(buff, buff_size, buff_pos))
        {
            fprintf_err("middleend parse: expected name\n");
            return false;
        }
    }
    return true;
}