#ifndef DISCODER
#define DISCODER

#include "ast.h"

//===========================================================================================================================
// CONST
//===========================================================================================================================

static const char *AST_OPERATOR_TYPE_NAMES[] =
{
    "-"                 ,
    "+"                 , // ADD
    "-"                 , // SUB
    "*"                 , // MUL
    "/"                 , // DIV
    "KEEPER"            , // OP_SQRT

    "CHECK_BEGIN"       , // INPUT
    "CHECK_OVER"        , // OUTPUT

    "GOAL"              , // OP_EQUAL
    ">"                 , // OP_ABOVE
    "<"                 , // OP_BELOW
    "GOAL_INSIDE"       , // OP_ABOVE_EQUAL
    "GOAL_OFFSIDE"      , // OP_BELOW_EQUAL
    "NO_GOAL"           , // OP_NOT_EQUAL
    "!"                 , // OP_NOT

    "GOAL_OR_ASSIST"    , // OP_OR
    "GOAL_PLUS_ASSIST"  , // OP_AND

    "="                 , // ASSIGNMENT
    "^"                 , // POW
    "LEFT_CORNER"       , // SIN
};

static const int OP_PRIORITY[] =
{
    0   ,
    5   ,   // ADD
    5   ,   // SUB
    6   ,   // MUL
    6   ,   // DIV
    9   ,   // SQRT

    0   ,   // INPUT
    0   ,   // OUTPUT

    3   ,   // OP_EQUAL
    4   ,   // OP_ABOVE
    4   ,   // OP_BELOW
    4   ,   // OP_ABOVE_EQUAL
    4   ,   // OP_BELOW_EQUAL
    3   ,   // OP_NOT_EQUAL
    8   ,   // OP_NOT

    1   ,   // OP_OR
    2   ,   // OP_AND

    0   ,   // ASSIGNMENT
    7   ,   // POW
    9   ,   // SIN
};

//===========================================================================================================================
// STRUCT
//===========================================================================================================================

struct name_info        // структура с информацией об имени
{
    const char *name;   // имя
    int     name_len;   // длина имени
};

struct name_list        // список имен
{
    name_info *word;    // массив структур с именами
    int        size;    // размер массива
    int    capacity;    // емкость массива
};

//===========================================================================================================================
// TRANSLATER
//===========================================================================================================================

bool discoder_translate (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                         const name_list *const func_store, const int  tab_shift = 0,
                                                                                                            const bool independent_op = true );

bool translate_fictional        (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_number           (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_variable         (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_if               (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_while            (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                   const bool independent_op);
//--------------------------------------------------------------------------------------------------------------------------
bool translate_operator               (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                       const name_list *const func_store, const int  tab_shift,
                                                                                                                          const bool independent_op);
bool translate_assignment             (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                       const name_list *const func_store, const int  tab_shift,
                                                                                                                          const bool independent_op);
bool translate_opened_unary_operator  (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                       const name_list *const func_store, const int  tab_shift,
                                                                                                                          const bool independent_op);
bool translate_closed_unary_operator  (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                       const name_list *const func_store, const int  tab_shift,
                                                                                                                          const bool independent_op);
bool translate_closed_binary_operator (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                       const name_list *const func_store, const int tab_shift,
                                                                                                                          const bool independent_op);
//--------------------------------------------------------------------------------------------------------------------------
bool translate_var_decl         (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_func_decl        (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_func_call        (const AST_node *const node, FILE *const stream, const name_list *const  var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);
bool translate_func_args_params (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    int *const arg_param_cnt );
bool translate_return           (const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                 const name_list *const func_store, const int  tab_shift,
                                                                                                                    const bool independent_op);

//===========================================================================================================================
// PARSE
//===========================================================================================================================

bool discoder_parse  (name_list *const  var_store,
                      name_list *const func_store,
                      AST_node **const       tree,                     const char *buff, const int buff_size, int *const buff_pos);
bool parse_name_list (name_list *const name_store, const int name_num, const char *buff, const int buff_size, int *const buff_pos);

//===========================================================================================================================
// NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void name_list_ctor (name_list *const name_store);
void name_list_dtor (name_list *const name_store);
void name_info_ctor (name_info *const word, const char *name_beg, const int name_len);
void name_info_dtor (name_info *const word);

//===========================================================================================================================
// NAME_LIST USER
//===========================================================================================================================

void name_list_push    (name_list *const name_store, const char *name_beg, const int name_len);

//===========================================================================================================================
// NAME_LIST CLOSED
//===========================================================================================================================

void name_list_realloc (name_list *const name_store);

#endif //DISCODER