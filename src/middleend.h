#ifndef MIDDLEEND
#define MIDDLEEND

#include "ast.h"

//===========================================================================================================================
// OPTIMIZE
//===========================================================================================================================

void optimize_ast  (AST_node *const node);

void optimize_add  (AST_node *const node);
void optimize_sub  (AST_node *const node);
void optimize_mul  (AST_node *const node);
void optimize_div  (AST_node *const node);
void optimize_pow  (AST_node *const node);
void optimize_and  (AST_node *const node);
void optimize_or   (AST_node *const node);
void optimize_sqrt (AST_node *const node);
void optimize_sin  (AST_node *const node);
void optimize_not  (AST_node *const node);

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