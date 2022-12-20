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
    
    optimize_const_ast(tree);
    optimize_diff_ast (tree);
    optimize_const_ast(tree);

    AST_tree_graphviz_dump(tree);

    fprintf(stderr, TERMINAL_GREEN "middleend success\n" TERMINAL_CANCEL);

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

void optimize_const_ast(AST_node *const node)
{
    if (node == nullptr) return;

    optimize_const_ast(L);
    optimize_const_ast(R);

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
        case OP_COS        : optimize_cos (node); break;
        case OP_LOG        : optimize_ln  (node); break;
        case OP_SQRT       : optimize_sqrt(node); break;
        case OP_NOT        : optimize_not (node); break;

        case OP_INPUT      :
        case OP_OUTPUT     :
        case ASSIGNMENT    :
        case OP_DIFF       : break;
        default            : assert(false && "default case in optimize_const_ast()"); return;
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

void optimize_cos(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_COS);

    if (L->type != NUMBER) return;

    const double l_op = L->value.dbl_num;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, cos(l_op), nullptr, nullptr, P);
}

void optimize_ln(AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_LOG);

    if (L->type != NUMBER) return;

    const double l_op = L->value.dbl_num;

    if (l_op < 0 || approx_equal(l_op, 0)) return;

    AST_tree_dtor(L);
    AST_tree_dtor(R);

    AST_node_NUMBER_ctor(node, log(l_op), nullptr, nullptr, P);
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
//---------------------------------------------------------------------------------------------------------------------------

bool optimize_diff_ast(AST_node *const node)
{
    if (node == nullptr) return true;

    if (!optimize_diff_ast(L)) return false;
    if (!optimize_diff_ast(R)) return false;

    if (!($type == OPERATOR && $op_type == OP_DIFF)) return true;

    optimize_const_ast(L);
    AST_node *diff = diff_do(L);
    optimize_const_ast(diff);

    if (P == nullptr)
    {
        AST_tree_dtor(diff);
        fprintf_err("the root of ast can't be \"OPERATOR\" type");
        return false;
    }
    if (P->left == node) P->left  = diff;
    else                 P->right = diff;

    diff->prev = P;
    AST_tree_dtor(node);

    return true;
}

AST_node *diff_do(const AST_node *const node)
{
    assert(node != nullptr);

    switch ($type)
    {
        case NUMBER    :
        case FUNC_CALL : return Null;
        case VARIABLE  : return One ;
        case OPERATOR  : return diff_op(node);
        case FICTIONAL :
        case OP_IF     :
        case IF_ELSE   :
        case OP_WHILE  :
        case VAR_DECL  :
        case FUNC_DECL :
        case OP_RETURN :
        default        : assert(false && "default case in diff_do()"); return nullptr;
    }
    return nullptr;
}

AST_node *diff_op(const AST_node *const node)
{
    assert(node  != nullptr);
    assert($type == OPERATOR);

    switch ($op_type)
    {
        case OP_ADD        : return Add   (dL, dR);
        case OP_SUB        : return Sub   (dL, dR);
        case OP_MUL        : return Add   (Mul(dL, cR), Mul(cL, dR));
        case OP_DIV        : return Div   (Sub(Mul(dL, cR), Mul(cL, dR)), Pow(cR, Two));
        case OP_SQRT       : return Div   (dL, Mul(Two, Sqrt(cL)));

        case OP_EQUAL      : return Equal (dL, dR);
        case OP_ABOVE      : return Above (dL, dR);
        case OP_BELOW      : return Below (dL, dR);
        case OP_ABOVE_EQUAL: return A_eq  (dL, dR);
        case OP_BELOW_EQUAL: return B_eq  (dL, dR);
        case OP_NOT_EQUAL  : return N_eq  (dL, dR);
        case OP_NOT        : return Not   (dL, dR);

        case OP_OR         : return Or    (dL, dR);
        case OP_AND        : return And   (dL, dR);

        case ASSIGNMENT    : return Assign(cL, dR);
        case OP_POW        : return diff_pow(node);
        case OP_SIN        : return Mul(Cos(cL), dL);
        case OP_COS        : return Mul(Mul(Sin(cL), dL), Sub_One);
        case OP_LOG        : return Div(dL, cL);

        case OP_DIFF       : assert(false && "OP_DIFF case in diff_op()"); return nullptr;
        default            : assert(false && "default case in diff_op()"); return nullptr;
    }
    return nullptr;
}

AST_node *diff_pow(const AST_node *const node)
{
    assert(node     != nullptr);
    assert($type    == OPERATOR);
    assert($op_type == OP_POW);

    if (L->type == NUMBER) return Mul(Pow(cL, cR), Mul(Ln(cL), dR));
    if (R->type == NUMBER)
    {
        AST_node *power = new_NUMBER_AST_node(R->value.dbl_num - 1);

        return Mul(Pow(cL, power), Mul(cR, dL));
    }
    return Mul(Pow(cL, cR), Add(Div(Mul(cR, dL), cL), Mul(dR, Ln(cL))));
}

AST_node *AST_node_dup(const AST_node *const node)
{
    if (node == nullptr) return nullptr;

    switch ($type)
    {
        case FICTIONAL : return new_FICTIONAL_AST_node(0          , AST_node_dup(L), AST_node_dup(R));
        case NUMBER    : return new_NUMBER_AST_node   ($dbl_num   , AST_node_dup(L), AST_node_dup(R));
        case VARIABLE  : return new_VARIABLE_AST_node (0          , AST_node_dup(L), AST_node_dup(R));
        case OP_IF     : return new_OP_IF_AST_node    (0          , AST_node_dup(L), AST_node_dup(R));
        case IF_ELSE   : return new_IF_ELSE_AST_node  (0          , AST_node_dup(L), AST_node_dup(R));
        case OP_WHILE  : return new_OP_WHILE_AST_node (0          , AST_node_dup(L), AST_node_dup(R));
        case OPERATOR  : return new_OPERATOR_AST_node ($op_type   , AST_node_dup(L), AST_node_dup(R));
        case VAR_DECL  : return new_VAR_DECL_AST_node ($var_index , AST_node_dup(L), AST_node_dup(R));
        case FUNC_DECL : return new_FUNC_DECL_AST_node($func_index, AST_node_dup(L), AST_node_dup(R));
        case FUNC_CALL : return new_FUNC_CALL_AST_node($func_index, AST_node_dup(L), AST_node_dup(R));
        case OP_RETURN : return new_OP_RETURN_AST_node(0          , AST_node_dup(L), AST_node_dup(R));

        default        : assert(false && "default case in AST_node_dup()");
                         break;
    }
    return nullptr;
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