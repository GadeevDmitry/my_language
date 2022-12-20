#ifndef MIDDLEEND
#define MIDDLEEND

#include "ast.h"

//===========================================================================================================================
// DSL
//===========================================================================================================================

#define Sub_One new_NUMBER_AST_node(-1)
#define Null    new_NUMBER_AST_node(0)
#define One     new_NUMBER_AST_node(1)
#define Two     new_NUMBER_AST_node(2)

#define     dL  diff_do(L)
#define     dR  diff_do(R)
#define     cL  AST_node_dup(L)
#define     cR  AST_node_dup(R)

#define    Add(left, right) new_OPERATOR_AST_node(OP_ADD        , left, right)
#define    Sub(left, right) new_OPERATOR_AST_node(OP_SUB        , left, right)
#define    Mul(left, right) new_OPERATOR_AST_node(OP_MUL        , left, right)
#define    Div(left, right) new_OPERATOR_AST_node(OP_DIV        , left, right)
#define   Sqrt(left       ) new_OPERATOR_AST_node(OP_SQRT       , left       )

#define  Equal(left, right) new_OPERATOR_AST_node(OP_EQUAL      , left, right)
#define  Above(left, right) new_OPERATOR_AST_node(OP_ABOVE      , left, right)
#define  Below(left, right) new_OPERATOR_AST_node(OP_BELOW      , left, right)
#define   A_eq(left, right) new_OPERATOR_AST_node(OP_ABOVE_EQUAL, left, right)
#define   B_eq(left, right) new_OPERATOR_AST_node(OP_BELOW_EQUAL, left, right)
#define   N_eq(left, right) new_OPERATOR_AST_node(OP_NOT_EQUAL  , left, right)
#define    Not(left, right) new_OPERATOR_AST_node(OP_NOT        , left, right)

#define     Or(left, right) new_OPERATOR_AST_node(OP_OR         , left, right)
#define    And(left, right) new_OPERATOR_AST_node(OP_AND        , left, right)

#define Assign(left, right) new_OPERATOR_AST_node(ASSIGNMENT    , left, right)
#define    Pow(left, right) new_OPERATOR_AST_node(OP_POW        , left, right)
#define    Sin(left       ) new_OPERATOR_AST_node(OP_SIN        , left       )
#define    Cos(left       ) new_OPERATOR_AST_node(OP_COS        , left       )
#define     Ln(left       ) new_OPERATOR_AST_node(OP_LOG        , left       )

//===========================================================================================================================
// OPTIMIZE
//===========================================================================================================================

void optimize_const_ast (AST_node *const node);

void optimize_add  (AST_node *const node);
void optimize_sub  (AST_node *const node);
void optimize_mul  (AST_node *const node);
void optimize_div  (AST_node *const node);
void optimize_pow  (AST_node *const node);
void optimize_and  (AST_node *const node);
void optimize_or   (AST_node *const node);
void optimize_sqrt (AST_node *const node);
void optimize_sin  (AST_node *const node);
void optimize_cos  (AST_node *const node);
void optimize_ln   (AST_node *const node);
void optimize_not  (AST_node *const node);

bool      optimize_diff_ast (      AST_node *const node);
AST_node *diff_op           (const AST_node *const node);
AST_node *diff_pow          (const AST_node *const node);

AST_node *diff_do           (const AST_node *const node);
AST_node *AST_node_dup      (const AST_node *const node);
//---------------------------------------------------------------------------------------------------------------------------

#define optimize_cmp(cmp_type)  void optimize_##cmp_type##_type(AST_node *const node);

optimize_cmp(equal)
optimize_cmp(not_equal)
optimize_cmp(above)
optimize_cmp(below)
optimize_cmp(above_eq)
optimize_cmp(below_eq)

#undef optimize_cmp

//===========================================================================================================================
// PARSE
//===========================================================================================================================

bool parse_name_list(const char *buff, const int buff_size, int *const buff_pos);

#endif //MIDDLEEND