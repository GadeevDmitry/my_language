#ifndef BACKEND
#define BACKEND

#include "ast.h"
#include "../lib/stack/stack.h"

//===========================================================================================================================
// CONST
//===========================================================================================================================

const char *NAME_PATTERN = "src/backend_pattern.txt";

//===========================================================================================================================
// DSL
//===========================================================================================================================

#define $glob       ast_asm->mem_glob
#define $loc        ast_asm->mem_loc
#define $scope      ast_asm->scope
#define $relative   ast_asm->relative

//===========================================================================================================================
// STRUCT
//===========================================================================================================================

struct global   // структура с глобальными переменными
{
    int *ram;   // массив адресов, ram[i] - адрес глобальной переменной номер i
                // если переменной i нет в глобальной области видимости, ram[i] = -1
    int size;   // размер .ram
};
//---------------------------------------------------------------------------------------------------------------------------

struct local    // структура с локальными переменными
{
    stack *ram; // массив стеков адресов, ram[i] - стек адресов переменной номер i в локальных областях видимости
    int   size; // размер .ram
};
//---------------------------------------------------------------------------------------------------------------------------

struct translator       // структура транслятор
{
    global mem_glob;    // адреса глобальных переменных
    local  mem_loc;     // адреса локальных переменных
    stack  scope;       // стек относительных адресов начал областей видимости
    int    relative;    // текущий относительный адрес
};

//===========================================================================================================================
// TRANSLATE
//===========================================================================================================================

void backend_header                     (FILE *const stream, const int main_num, const int rex_begin);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_backend                  (translator *const ast_asm, const AST_node *const node, FILE *const stream);
bool translate_func_args                (translator *const ast_asm, const AST_node *const node, FILE *const stream);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_distributor              (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_fictional                (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
bool translate_var_decl                 (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independnt_op);
bool translate_number                   (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
bool translate_variable                 (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
bool translate_if                       (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
bool translate_while                    (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_operator                 (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_closed_binary_operator   (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_opened_unary_operator    (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);

bool translate_assignment               (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
bool translate_operator_input           (translator *const ast_asm, const AST_node *const node, FILE *const stream);
bool translate_pop_variable             (translator *const ast_asm, const AST_node *const node, FILE *const stream);
bool translate_operator_output          (translator *const ast_asm, const AST_node *const node, FILE *const stream);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_closed_unary_operator    (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_func_call                (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
bool translate_func_param               (translator *const ast_asm, const AST_node *const node, FILE *const stream, int *const param_cnt);
//---------------------------------------------------------------------------------------------------------------------------
bool translate_return                   (translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op);
//---------------------------------------------------------------------------------------------------------------------------
bool fill_global_scope                  (global *const mem_glob, const AST_node *const node, int *const rex);

//===========================================================================================================================
// EXTRA
//===========================================================================================================================

void set_rex(const int rex_val, FILE *const stream);
void add_rex(const int rex_add, FILE *const stream);
void sub_rex(const int rex_sub, FILE *const stream);
//---------------------------------------------------------------------------------------------------------------------------
bool translator_redefined_var (translator *const ast_asm, const int var_index);
bool translator_undefined_var (translator *const ast_asm, const int var_index);
void translator_new_scope     (translator *const ast_asm);
void translator_del_scope     (translator *const ast_asm);

//===========================================================================================================================
// PARSE
//===========================================================================================================================

// возвращает номер главной функции и -1 в случае ошибки
int backend_parse         (translator *const ast_asm,
                           AST_node  **const    tree, const char *buff, const int buff_size, int *const buff_pos);

// считывает имена переменных, возвращает их количество и -1 в случае ошибки
int  var_name_list_parse  (const char *buff, const int buff_size, int *const buff_pos);

// считывает имена функций, возвращает номер главной функции и -1 в случае ошибки
int  func_name_list_parse (const char *buff, const int buff_size, int *const buff_pos);

//===========================================================================================================================
// CTOR_DTOR
//===========================================================================================================================

void translator_ctor (translator *const ast_asm, const int var_num);
void translator_dtor (translator *const ast_asm);
//---------------------------------------------------------------------------------------------------------------------------
void global_ctor     (global *const mem, const int size);
void global_dtor     (global *const mem);
//---------------------------------------------------------------------------------------------------------------------------
void local_ctor      (local *const mem, const int size);
void local_dtor      (local *const mem);

#endif //BACKEND