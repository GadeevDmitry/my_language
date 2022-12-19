#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../lib/logs/log.h"
#include "../lib/read_write/read_write.h"
#include "../lib/algorithm/algorithm.h"
#include "../lib/graphviz_dump/graphviz_dump.h"

#include "frontend.h"
#include "terminal_colors.h"

/*
//===========================================================================================================================
// general: return none
//===========================================================================================================================

general = {var_declaration | function_declaration}+   //"CAMP_NOU" - имя главной функции

//===========================================================================================================================
// var_declaration: return none
//===========================================================================================================================

var_declaration = "BARCELONA" var_name ';'
                            //var_name - имя новой переменной
                            //имя      - последовательность не ключевых символов, не являющаяся ключевым словом
                            //имя не может начинаться с цифры

//===========================================================================================================================
// function_declaration: return none
// function_call:        return value returned by the function
// op_function_call:     return none
//===========================================================================================================================

function_declaration = "BARCELONA" func_name '(' args ')' '{' operators '}'
                                // func_name - имя новой функции

function_call        = func '(' param ')'
                    // func - имя существующей функции

op_function_call     = function_call ';'
args                 = {var_name {',' var_name}*}?
param                = {rvalue   {',' rvalue  }*}?
    
//===========================================================================================================================
// operators: return none
//===========================================================================================================================

operators   = {var_declaration | op_assignment | op_input | op_output | if | while | op_function_call | op_return}*

//===========================================================================================================================
// op_assignment: return none
//===========================================================================================================================

op_assignment = lvalue '=' rvalue ';'

//===========================================================================================================================
// op_input:  return none
// op_output: return none
//===========================================================================================================================

op_input  = "CHECK_BEGIN" lvalue ';'
op_output = "CHECK_OVER"  rvalue ';'

//===========================================================================================================================
// if: return none
//===========================================================================================================================

if   = "MESSI" '(' rvalue ')' '{' operators '}' else?
else = "SUAREZ" '{' operators '}'

//===========================================================================================================================
// while: return none
//===========================================================================================================================

while = "NEYMAR" '(' rvalue ')' '{' operators '}'

//===========================================================================================================================
// op_return: return none
//===========================================================================================================================

op_return = "CHAMPIONS_LEAGUE" rvalue ';'

//===========================================================================================================================
// rvalue: return value of expretion
//===========================================================================================================================

rvalue       =  assignment? op_or
assignment   = (lvalue '=')+
op_or        = op_and     {'||'        op_and    }*
op_and       = op_equal   {'&&'        op_equal  }*
op_equal     = op_compare {[== !=]     op_compare}?
op_compare   = op_add_sub {[< > <= >=] op_add_sub}?
op_add_sub   = op_mul_div {[+-]        op_mul_div}*
op_mul_div   = op_pow     {[/*]        op_pow    }*
op_pow       = op_not     {'^'         op_not    }?
op_not       = {!}? operand
operand      = '('rvalue ')' | function_call | rvalue_token

rvalue_token = number | lvalue
number       = double

//===========================================================================================================================
// lvalue: return value of variable
//===========================================================================================================================

       //var - имя существующей переменной
lvalue = var
*/

//===========================================================================================================================
// MAIN
//===========================================================================================================================

int main(const int argc, const char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "you should give one parameter: source code\n");
        return 0;
    }
    source *code = new_source(argv[1]);
    if     (code == nullptr) return 0;

    lexical_analyzer   (code);
    lexis_graphviz_dump(code);

    dictionary name_store = {};
    dictionary_ctor(&name_store);
    AST_node        *root = parse_general(code, &name_store);
    //>>>>>>>>>>
    var_name_list_text_dump (&name_store.var_store);
    func_name_list_text_dump(&name_store.func_store);
    AST_tree_graphviz_dump  (root);
    //<<<<<<<<<<
    if (root == nullptr)
    {
        fprintf(stderr, TERMINAL_RED "compile failed\n" TERMINAL_CANCEL);
    }
    else
    {
        fprintf(stderr, TERMINAL_GREEN "compile success\n" TERMINAL_CANCEL);
        frontend_convert(&name_store, root, argv[1]);
    }
    source_dtor(code);
}

//===========================================================================================================================
// CONVERT
//===========================================================================================================================

void frontend_convert(dictionary *const name_store, AST_node *const tree, const char *source_file)
{
    assert(name_store  != nullptr);
    assert(tree        != nullptr);
    assert(source_file != nullptr);

    char    frontend_file[strlen(source_file) + 10] = {};
    sprintf(frontend_file, "%s.front", source_file);

    FILE *stream = fopen(frontend_file, "w");
    if   (stream == nullptr)
    {
        fprintf(stderr, "can't open \"%s\" to convert the AST\n", frontend_file);

        dictionary_dtor(name_store);
        AST_tree_dtor  (tree);
        return;
    }

    var_name_list_convert (              &$var_store, stream);
    func_name_list_convert(&$func_store, &$var_store, stream);

    AST_convert(tree, stream);

    dictionary_dtor(name_store);
    AST_tree_dtor  (tree);
    fclose         (stream);
}

void var_name_list_convert(const var_name_list *const var_store, FILE *const stream)
{
    assert(var_store != nullptr);
    assert(stream    != nullptr);

    fprintf(stream, "%d\n", var_store->size);

    for (int i = 0; i < var_store->size; ++i)
    {
        fprintf(stream, "%s\n", var_store->var[i].name);
    }
    fprintf(stream, "\n");
}

void func_name_list_convert(const func_name_list *const func_store, const var_name_list *const var_store, FILE *const stream)
{
    assert(func_store != nullptr);
    assert(var_store  != nullptr);
    assert(stream     != nullptr);

    fprintf(stream, "%d\n", func_store->size);

    for (int i = 0; i < func_store->size; ++i)
    {
        fprintf(stream, "%s\n", func_store->func[i].name);
    }
    fprintf(stream, "\n");
}

//===========================================================================================================================
// VAR_NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void var_name_list_ctor(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    var_store->size     = 0;
    var_store->capacity = 4;    //default capacity
    var_store->var      = (var_info *) log_calloc(4, sizeof(var_info));
}

void var_name_list_dtor(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    for (int i = 0; i < var_store->size; ++i) var_info_dtor(var_store->var+i);
    log_free(var_store->var);

    var_store->var      = nullptr;
    var_store->size     = 0;
    var_store->capacity = 0;
}

void var_info_ctor(var_info *const var, const source *const code, const token *const cur_token, const int scope)
{
    assert(var       != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    assert(cur_token->type == UNDEF_TOKEN);

    const char *name_beg = buff_data + cur_token->token_beg;
    const int   name_len =             cur_token->token_len_val;

    var->name     = strndup(name_beg, (size_t) name_len);
    var->name_len = name_len;
    stack_ctor(&var->scope, sizeof(int));

    if (scope != -1) stack_push(&var->scope, &scope);
}

void var_info_dtor(var_info *const var)
{
    assert(var != nullptr);

    free      ((char *)var->name); //не используем log_free, так как выделяли память с помощью strndup, а не log_calloc
    stack_dtor(&var->scope);

    var->name_len = 0;
}

//===========================================================================================================================
// VAR_NAME_LIST USER
//===========================================================================================================================

int var_name_list_add_var(var_name_list *const var_store, const source *const code, const token *const cur_token, const int scope)
{
    assert(var_store != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (same_var(code, var_store->var+i, cur_token))
        {
            var_info_push_scope(var_store->var+i, scope);
            return i;
        }
    }
    return var_name_list_new_var(var_store, code, cur_token, scope);
}

void var_name_list_clear_var(var_name_list *const var_store, const int scope)
{
    assert(var_store != nullptr);

    for (int i = 0; i < var_store->size; ++i) var_info_pop_scope(var_store->var+i, scope);
}

int var_name_list_defined_var(var_name_list *const var_store, const source *const code, const token *const cur_token)
{
    assert(var_store != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (same_var(code, var_store->var+i, cur_token) && !stack_empty(&var_store->var[i].scope)) return i;
    }
    return -1;
}

int var_name_list_redefined_var(var_name_list *const var_store, const source *const code, const token *const cur_token, const int scope)
{
    assert(var_store != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (same_var(code, var_store->var+i, cur_token))
        {
            stack *cur_stk = &var_store->var[i].scope;

            if (!stack_empty(cur_stk) && scope == *(int *)stack_front(cur_stk)) return  i;
            else                                                                return -1;
        }
    }
    return -1;
}

//===========================================================================================================================
// VAR_NAME_LIST CLOSED
//===========================================================================================================================

bool same_var(const source *const code, var_info *const var, const token *const cur_token)
{
    assert(var       != nullptr);
    assert(cur_token != nullptr);

    assert(cur_token->type == UNDEF_TOKEN);

    const char *name_beg = buff_data+cur_token->token_beg;
    const int   name_len = cur_token->token_len_val;

    return var->name_len == name_len && !strncmp(var->name, name_beg, (size_t) name_len);
}

void var_info_push_scope(var_info *const var, const int scope)
{
    assert(var != nullptr);

    stack_push(&var->scope, &scope);
}

void var_info_pop_scope(var_info *const var, const int scope)
{
    assert(var != nullptr);

    if (stack_empty(&var->scope)) return;

    int last_scope = *(int *) stack_front(&var->scope);
    if (last_scope == scope)  stack_pop  (&var->scope);
}

int var_name_list_new_var(var_name_list *const var_store, const source *const code, const token *const cur_token, const int scope)
{
    assert(var_store != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    var_name_list_realloc(var_store);
    var_info_ctor        (var_store->var+var_store->size, code, cur_token, scope);

    return var_store->size++;
}

void var_name_list_realloc(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    if (var_store->size == var_store->capacity)
    {
        var_store->capacity *= 2;
        var_store->var       = (var_info *) log_realloc(var_store->var, (size_t) var_store->capacity * sizeof(var_info));
    }
}

//===========================================================================================================================
// FUNC_NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void func_name_list_ctor(func_name_list *const func_store)
{
    assert(func_store != nullptr);

    func_store->size     = 0;
    func_store->capacity = 4;   //default capacity
    func_store->func     = (func_info *) log_calloc(4, sizeof(func_info));
}

void func_name_list_dtor(func_name_list *const func_store)
{
    for (int i = 0; i < func_store->size; ++i) func_info_dtor(func_store->func+i);
    log_free(func_store->func);

    func_store->func     = nullptr;
    func_store->size     = 0;
    func_store->capacity = 0;
}

void func_info_ctor(func_info *const func, const source *const code, const token *const cur_token)
{
    assert(func      != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    assert(cur_token->type == UNDEF_TOKEN);

    const char *name_beg = buff_data + cur_token->token_beg;
    const int   name_len =             cur_token->token_len_val;

    func->name     = strndup(name_beg, (size_t) name_len);
    func->name_len = name_len;
    func->arg_num  = 0;
    arg_list_ctor(&func->args);
}

void func_info_dtor(func_info *const func)
{
    assert(func != nullptr);

    free         ((char *) func->name); //не используем log_free, так как выделяли память с помощью strndup, а не log_calloc
    arg_list_dtor(&func->args);

    func->name_len = 0;
    func->arg_num  = 0;
}

void arg_list_ctor(arg_list *const arg_store)
{
    assert(arg_store != nullptr);

    arg_store->size       = 0;
    arg_store->capacity   = 4;
    arg_store->name_index = (int *) log_calloc(4, sizeof(int));
}

void arg_list_dtor(arg_list *const arg_store)
{
    assert(arg_store != nullptr);

    log_free(arg_store->name_index);

    arg_store->name_index = nullptr;
    arg_store->capacity   = 0;
    arg_store->size       = 0;
}

//===========================================================================================================================
// FUNC_NAME_LIST USER
//===========================================================================================================================

int func_name_list_add_func(func_name_list *const func_store, const source *const code, const token *const cur_token)
{
    assert(func_store != nullptr);
    assert(code       != nullptr);
    assert(cur_token  != nullptr);

    func_name_list_realloc(func_store);
    func_info_ctor        (func_store->func+func_store->size, code, cur_token);

    return func_store->size++;
}

int func_name_list_defined_func(func_name_list *const func_store, const source *const code, const token *const cur_token)
{
    assert(func_store != nullptr);
    assert(code       != nullptr);
    assert(cur_token  != nullptr);

    for (int i = 0; i < func_store->size; ++i)
    {
        if (same_func(code, func_store->func+i, cur_token)) return i;
    }
    return -1;
}

void func_name_list_add_args(func_name_list *const func_store, const int func_index, AST_node *const node)
{
    assert(func_store != nullptr);

    assert(func_index >= 0);
    assert(func_index < func_store->size);

    func_name_add_args(func_store->func+func_index, node);
}

int func_name_list_get_arg_num(const func_name_list *const func_store, const int func_index)
{
    assert(func_store != nullptr);

    assert(func_index >= 0);
    assert(func_index < func_store->size);

    return func_store->func[func_index].arg_num;
}

bool func_name_list_check_main_func(const func_name_list *const func_store)
{
    assert(func_store != nullptr);

    int main_func_len = strlen(MAIN_FUNCTION);

    for (int i = 0; i < func_store->size; ++i)
    {
        const func_info *const cur_func = func_store->func+i;

        if (cur_func->name_len == main_func_len && cur_func->arg_num == 0 &&
            !strncmp(cur_func->name, MAIN_FUNCTION, main_func_len))
        {
            return true;
        }
    }
    return false;
}

//===========================================================================================================================
// FUNC_NAME_LIST CLOSED
//===========================================================================================================================

bool same_func(const source *const code, func_info *const func, const token *const cur_token)
{
    assert(code      != nullptr);
    assert(func      != nullptr);
    assert(cur_token != nullptr);

    assert(cur_token->type == UNDEF_TOKEN);

    const char *name_beg = buff_data + cur_token->token_beg;
    const int   name_len = cur_token->token_len_val;

    return func->name_len == name_len && !strncmp(func->name, name_beg, (size_t) name_len);
}

void func_name_list_realloc(func_name_list *const func_store)
{
    assert(func_store != nullptr);

    if (func_store->size == func_store->capacity)
    {
        func_store->capacity *= 2;
        func_store->func      = (func_info *) log_realloc(func_store->func, (size_t) func_store->capacity * sizeof(func_info));
    }
}

void func_name_add_args(func_info *const func, const AST_node *const node)
{
    assert(func != nullptr);

    if (node == nullptr) return;

    if ($type == FICTIONAL) { func_name_add_args(func, L); func_name_add_args(func, R); return; }
    if ($type ==  VARIABLE)
    {
        arg_list_push_arg(&func->args, $var_index);
        func->arg_num++;
        return;
    }
    assert(false && "bad AST-node function argument type");
}

void arg_list_push_arg(arg_list *const arg_store, const int arg_index)
{
    assert(arg_store != nullptr);

    arg_list_realloc(arg_store);
    arg_store->name_index[arg_store->size++] = arg_index;
}

void arg_list_realloc(arg_list *const arg_store)
{
    assert(arg_store != nullptr);

    if (arg_store->size == arg_store->capacity)
    {
        arg_store->capacity  *= 2;
        arg_store->name_index = (int *) log_realloc(arg_store->name_index, (size_t) arg_store->capacity * sizeof(int));
    }
}

//===========================================================================================================================
// DICTIONARY_CTOR_DTOR
//===========================================================================================================================

void dictionary_ctor(dictionary *const name_store)
{
    assert(name_store != nullptr);

    var_name_list_ctor (&$var_store);
    func_name_list_ctor(&$func_store);

    $scope = 0;
}

void dictionary_dtor(dictionary *const name_store)
{
    assert(name_store != nullptr);

    var_name_list_dtor (&$var_store);
    func_name_list_dtor(&$func_store);

    $scope = -1;
}

//===========================================================================================================================
// TRANSLATOR
//===========================================================================================================================

#define fprintf_err(line, message) fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL message, line)

bool MEET_RETURN = false;   // true, если встретился оператор return в области видимости 1
                            // нужен для отслеживания наличия возвращаемого значения в функции

//---------------------------------------------------------------------------------------------------------------------------
#define general_err_exit                                                                                                    \
        AST_tree_dtor  (root);                                                                                              \
        dictionary_dtor(name_store);                                                                                        \
        return nullptr;

AST_node *parse_general(const source *const code, dictionary *const name_store)
{
    assert(code       != nullptr);
    assert(name_store != nullptr);

    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
    var_name_list_text_dump (&$var_store);
    func_name_list_text_dump(&$func_store);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    AST_node   *root = new_FICTIONAL_AST_node(0);
    int    token_cnt = 0;
    while (token_cnt < lexis_pos)
    {
        AST_node *subtree = nullptr;

        if (!parse_var_decl(name_store, code, &token_cnt, &subtree))  { general_err_exit }
        if (subtree != nullptr)                                       { fictional_merge_tree(root, subtree); continue; }

        if (!parse_func_decl(name_store, code, &token_cnt, &subtree)) { general_err_exit }
        if (subtree != nullptr)                                       { fictional_merge_tree(root, subtree); continue; }

        fprintf_err(lexis_data[token_cnt].token_line, "undefined function or variable declaration\n");
        general_err_exit;
    }
    if (!func_name_list_check_main_func(&$func_store))
    {
        fprintf(stderr, TERMINAL_RED "ERROR: " TERMINAL_CANCEL "there is no main function %s()\n", MAIN_FUNCTION);
        general_err_exit
    }
    return root;
}
#undef general_err_exit
//---------------------------------------------------------------------------------------------------------------------------

bool parse_var_decl(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const subtree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(subtree    != nullptr);
    assert(*subtree   == nullptr);

    const int old_token_cnt = *token_cnt;

    if (!token_dbl($cur_token)) return true;
    *token_cnt += 1;

    if (!(token_undef($cur_token) && token_char($next_token, ';'))) { *token_cnt = old_token_cnt; return true; }

    if (var_name_list_redefined_var(&$var_store, code, &$cur_token, $scope) != -1)
    {
        fprintf_err($cur_token.token_line, "redefined variable\n");
        *token_cnt = old_token_cnt;
        return false;
    }
    *subtree = new_VAR_DECL_AST_node(var_name_list_add_var(&$var_store, code, &$cur_token, $scope));
    *token_cnt += 2;
    return true;
}
//---------------------------------------------------------------------------------------------------------------------------
#define func_decl_err_exit                                                                                                  \
        AST_tree_dtor(*func_decl_tree);                                                                                     \
        *func_decl_tree = nullptr;                                                                                          \
        *token_cnt      = old_token_cnt;                                                                                    \
        return false;

bool parse_func_decl(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const func_decl_tree)
{
    assert(name_store      != nullptr);
    assert(code            != nullptr);
    assert(token_cnt       != nullptr);
    assert(func_decl_tree  != nullptr);
    assert(*func_decl_tree == nullptr);

    const int old_token_cnt = *token_cnt;

    if (!token_dbl($cur_token)) return true;
    *token_cnt += 1;

    if (!(token_undef($cur_token) && token_char($next_token, '('))) { *token_cnt = old_token_cnt; return true; }

    if (func_name_list_defined_func(&$func_store, code, &$cur_token) != -1)
    {
        fprintf_err($cur_token.token_line, "redefined function\n");
        func_decl_err_exit
    }
    const int func_index = func_name_list_add_func(&$func_store, code, &$cur_token);
    *func_decl_tree      = new_FUNC_DECL_AST_node (func_index);
    *token_cnt          += 2;

    AST_node *subtree = nullptr;

    if (!parse_func_args(name_store, code, token_cnt, &subtree)) { func_decl_err_exit }

    func_name_list_add_args(&$func_store, func_index, subtree);
    (*func_decl_tree)->left = subtree;
    if (subtree != nullptr) subtree->prev = *func_decl_tree;
    subtree = nullptr;

    if (!token_char($cur_token, ')'))
    {
        fprintf_err($cur_token.token_line, "expected ')' after function header\n");
        func_decl_err_exit
    }
    *token_cnt += 1;

    if (!token_char($cur_token, '{'))
    {
        fprintf_err($cur_token.token_line, "expected '{' before function operators\n");
        func_decl_err_exit
    }
    *token_cnt += 1;

    if (!parse_operators(name_store, code, token_cnt, &subtree)) { func_decl_err_exit }

    (*func_decl_tree)->right = subtree;
    if (subtree != nullptr)    subtree->prev = *func_decl_tree;
    subtree = nullptr;

    if (!token_char($cur_token, '}'))
    {
        fprintf_err($cur_token.token_line, "expected '}' after function operators\n");
        func_decl_err_exit
    }
    if (!MEET_RETURN)
    {
        fprintf_err($cur_token.token_line, "expected return value at the end of function\n");
        func_decl_err_exit
    }
    MEET_RETURN = false;
    *token_cnt += 1;
    var_name_list_clear_var(&$var_store, $scope);
    $scope      = 0;
    return true;
}
#undef  func_decl_err_exit
#define func_args_err_exit                                                                                                  \
        AST_tree_dtor(*arg_tree);                                                                                           \
        *arg_tree  = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_func_args(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const arg_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(arg_tree   != nullptr);
    assert(*arg_tree  == nullptr);

    const int old_token_cnt = *token_cnt;
    $scope = 1;

    if (!token_undef($cur_token)) return true;

    *arg_tree         = new_FICTIONAL_AST_node(0);
    AST_node *subtree = new_VARIABLE_AST_node (var_name_list_add_var(&$var_store, code, &$cur_token, $scope));

    fictional_merge_tree(*arg_tree, subtree);
    *token_cnt += 1;

    while (token_char($cur_token, ','))
    {
        *token_cnt += 1;
        if (!token_undef($cur_token))
        {
            fprintf_err($cur_token.token_line, "undefined name of argument\n");
            func_args_err_exit
        }
        if (var_name_list_redefined_var(&$var_store, code, &$cur_token, $scope) != -1)
        {
            fprintf_err($cur_token.token_line, "redefined name of argument\n");
            func_args_err_exit
        }
        subtree = new_VARIABLE_AST_node(var_name_list_add_var(&$var_store, code, &$cur_token, $scope));
        fictional_merge_tree(*arg_tree, subtree);
        *token_cnt += 1;
    }
    return true;
}
#undef func_args_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define operators_err_exit                                                                                                  \
        AST_tree_dtor(*op_tree);                                                                                            \
        *op_tree = nullptr;                                                                                                 \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_operators(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const op_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(op_tree    != nullptr);
    assert(*op_tree   == nullptr);

    const int old_token_cnt = *token_cnt;
    *op_tree = new_FICTIONAL_AST_node(0);

    while (*token_cnt < lexis_pos)
    {
        AST_node *subtree = nullptr;

        #define call_parser(parser_name)                                                                                    \
            if (!parser_name(name_store, code, token_cnt, &subtree)) { operators_err_exit }                                 \
            if (subtree != nullptr)                                  { fictional_merge_tree(*op_tree, subtree); continue; }

        call_parser(parse_var_decl)
        call_parser(parse_op_assignment)
        call_parser(parse_op_input)
        call_parser(parse_op_output)
        call_parser(parse_if)
        call_parser(parse_while)
        call_parser(parse_op_func_call)
        call_parser(parse_op_return)

        break;

        #undef call_parser
    }
    if ((*op_tree)->left == nullptr && (*op_tree)->right == nullptr)
    {
        AST_tree_dtor(*op_tree);
        *op_tree   = nullptr;
        *token_cnt = old_token_cnt;
    }
    return true;
}
//---------------------------------------------------------------------------------------------------------------------------
#define op_assignment_err_exit                                                                                              \
        AST_tree_dtor(*op_assign_tree);                                                                                     \
        *op_assign_tree = nullptr;                                                                                          \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_assignment(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const op_assign_tree)
{
    assert(name_store      != nullptr);
    assert(code            != nullptr);
    assert(token_cnt       != nullptr);
    assert(op_assign_tree  != nullptr);
    assert(*op_assign_tree == nullptr);

    if (!token_char($next_token, '=')) return true;

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;

    if (!parse_lvalue(name_store, code, token_cnt, &subtree)) { op_assignment_err_exit }
    if (subtree == nullptr)
    {
        fprintf_err($cur_token.token_line, "expexted lvalue before '='\n");
        op_assignment_err_exit
    }
    *op_assign_tree = new_OPERATOR_AST_node(ASSIGNMENT, subtree);
    subtree         = nullptr;
    *token_cnt     += 1;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { op_assignment_err_exit }

    (*op_assign_tree)->right = subtree;
    subtree          ->prev  = *op_assign_tree;

    if (!token_char($cur_token, ';'))
    {
        fprintf_err($cur_token.token_line, "expected ';' after assignment\n");
        op_assignment_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef op_assignment_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define input_err_exit                                                                                                      \
        AST_tree_dtor(*in_tree);                                                                                            \
        *in_tree   = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_input(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const in_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(in_tree    != nullptr);
    assert(*in_tree   == nullptr);

    if (!token_input($cur_token)) return true;

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;
    *token_cnt             += 1;

    if (!parse_lvalue(name_store, code, token_cnt, &subtree)) { input_err_exit }
    if (subtree == nullptr)
    {
        fprintf_err($cur_token.token_line, "undefined lvalue");
        input_err_exit
    }
    *in_tree = new_OPERATOR_AST_node(OP_INPUT, subtree);

    if (!token_char($cur_token, ';'))
    {
        fprintf_err($cur_token.token_line, "expected ';' after input operator\n");
        input_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef input_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define output_err_exit                                                                                                     \
        AST_tree_dtor(*out_tree);                                                                                           \
        *out_tree  = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_output(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const out_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(out_tree   != nullptr);
    assert(*out_tree  == nullptr);

    if (!token_output($cur_token)) return true;

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;
    *token_cnt             += 1;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { output_err_exit }
    *out_tree = new_OPERATOR_AST_node(OP_OUTPUT, subtree);

    if (!token_char($cur_token, ';'))
    {
        fprintf_err($cur_token.token_line, "expected ';' after output operator\n");
        output_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef output_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define if_err_exit                                                                                                         \
        AST_tree_dtor(*if_tree);                                                                                            \
        *if_tree = nullptr;                                                                                                 \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_if(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const if_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(if_tree    != nullptr);
    assert(*if_tree   == nullptr);

    if (!token_if($cur_token)) return true;

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;
    *token_cnt             += 1;

    if (!token_char($cur_token, '('))
    {
        fprintf_err($cur_token.token_line, "expected '(' before condition\n");
        if_err_exit
    }
    *token_cnt += 1;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { if_err_exit }

   *if_tree = new_IF_ELSE_AST_node(0);
   *if_tree = new_OP_IF_AST_node  (0, subtree, *if_tree);
    subtree = nullptr;

    if (!token_char($cur_token, ')'))
    {
        fprintf_err($cur_token.token_line, "expected ')' after condition\n");
        if_err_exit
    }
    *token_cnt += 1;
    if (!token_char($cur_token, '{'))
    {
        fprintf_err($cur_token.token_line, "expected '{' before \"if\" operators\n");
        if_err_exit
    }
    *token_cnt += 1;
    $scope++;

    if (!parse_operators(name_store, code, token_cnt, &subtree)) { if_err_exit }

    (*if_tree)->right->left = subtree;
    if (subtree != nullptr)   subtree->prev = (*if_tree)->right;
    subtree = nullptr;

    if (!token_char($cur_token, '}'))
    {
        fprintf_err($cur_token.token_line, "expected '}' after \"if\" operators\n");
        if_err_exit
    }
    *token_cnt += 1;
    var_name_list_clear_var(&$var_store, $scope);
    $scope--;

    if (!parse_else(name_store, code, token_cnt, &subtree)) { if_err_exit }

    (*if_tree)->right->right = subtree;
    if (subtree != nullptr)    subtree->prev = (*if_tree)->right;

    return true;
}
#undef    if_err_exit
#define else_err_exit                                                                                                       \
        AST_tree_dtor(*else_tree);                                                                                          \
        *else_tree = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_else(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const else_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(else_tree  != nullptr);
    assert(*else_tree == nullptr);

    if (!token_else($cur_token)) return true;

    const int old_token_cnt = *token_cnt;
    *token_cnt             += 1;

    if (!token_char($cur_token, '{'))
    {
        fprintf_err($cur_token.token_line, "expected '{' after else\n");
        else_err_exit
    }
    *token_cnt += 1;
    $scope++;

    if (!parse_operators(name_store, code, token_cnt, else_tree)) { else_err_exit }

    if (!token_char($cur_token, '}'))
    {
        fprintf_err($cur_token.token_line, "expected '}' after \"else\" operators\n");
        else_err_exit;
    }
    *token_cnt += 1;
    var_name_list_clear_var(&$var_store, $scope);
    $scope--;

    return true;
}
#undef else_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define while_err_exit                                                                                                      \
        AST_tree_dtor(*while_tree);                                                                                         \
        *while_tree = nullptr;                                                                                              \
        *token_cnt  = old_token_cnt;                                                                                        \
        return false;

bool parse_while(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const while_tree)
{
    assert(name_store  != nullptr);
    assert(code        != nullptr);
    assert(token_cnt   != nullptr);
    assert(while_tree  != nullptr);
    assert(*while_tree == nullptr);

    if (!token_while($cur_token)) return true;

    const int old_token_cnt = *token_cnt;
    AST_node *subtree       = nullptr;
    *token_cnt             += 1;

    if (!token_char($cur_token, '('))
    {
        fprintf_err($cur_token.token_line, "expexted '(' before condition\n");
        while_err_exit
    }
    *token_cnt += 1;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { while_err_exit }

    *while_tree = new_OP_WHILE_AST_node(0, subtree);
    subtree     = nullptr;

    if (!token_char($cur_token, ')'))
    {
        fprintf_err($cur_token.token_line, "expected ')' after condition\n");
        while_err_exit
    }
    *token_cnt += 1;
    if (!token_char($cur_token, '{'))
    {
        fprintf_err($cur_token.token_line, "expexted '{' before \"while\" operators\n");
        while_err_exit
    }
    *token_cnt += 1;
    $scope++;

    if (!parse_operators(name_store, code, token_cnt, &subtree)) { while_err_exit }

    (*while_tree)->right =  subtree;
    if (subtree != nullptr) subtree->prev = *while_tree;
    subtree = nullptr;

    if (!token_char($cur_token, '}'))
    {
        fprintf_err($cur_token.token_line, "expected '}' after \"while\" operators\n");
        while_err_exit
    }
    *token_cnt += 1;
    var_name_list_clear_var(&$var_store, $scope);
    $scope--;

    return true;
}
#undef while_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define op_func_call_err_exit                                                                                               \
        AST_tree_dtor(*subtree);                                                                                            \
        *subtree   = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_func_call(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const subtree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(subtree    != nullptr);
    assert(*subtree   == nullptr);

    if (!(token_undef($cur_token) && token_char($next_token, '('))) return true;

    const int old_token_cnt = *token_cnt;

    if (!parse_func_call(name_store, code, token_cnt, subtree)) { op_func_call_err_exit }
    if (!token_char($cur_token, ';'))
    {
        fprintf_err($cur_token.token_line, "expected ';' after function call\n");
        op_func_call_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef op_func_call_err_exit
#define   func_call_err_exit                                                                                                \
        AST_tree_dtor(*subtree);                                                                                            \
        *subtree   = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_func_call(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const subtree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(subtree    != nullptr);
    assert(*subtree   == nullptr);

    const int old_token_cnt = *token_cnt;

    if (!(token_undef($cur_token) && token_char($next_token, '('))) return true;

    const int func_index = func_name_list_defined_func(&$func_store, code, &$cur_token);
    if (      func_index == -1)
    {
        fprintf_err($cur_token.token_line, "undefined function\n");
        func_call_err_exit
    }
    *subtree    = new_FUNC_CALL_AST_node(func_index);
    *token_cnt += 2;
    
    int param_num = 0;
    if (token_char($cur_token, ')'))
    {
        if (param_num != func_name_list_get_arg_num(&$func_store, func_index))
        {
            fprintf_err($cur_token.token_line, "bad number of parameters in function call\n");
            func_call_err_exit
        }
        *token_cnt += 1;
        return true;
    }

    AST_node *param_tree = nullptr;
    if (!parse_func_call_param(name_store, code, token_cnt, &param_tree)) { func_call_err_exit }

    param_num = get_subtree_num(param_tree);
    (*subtree)->left = param_tree;
    param_tree->prev = *subtree;

    if (!token_char($cur_token, ')'))
    {
        fprintf_err($cur_token.token_line, "expected ')' after function parameters\n");
        func_call_err_exit
    }
    if (param_num != func_name_list_get_arg_num(&$func_store, func_index))
    {
        fprintf_err($cur_token.token_line, "bad number of parameters in function call\n");
        func_call_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef  func_call_err_exit
#define func_call_param_err_exit                                                                                            \
        AST_tree_dtor(*param_tree);                                                                                         \
        *param_tree = nullptr;                                                                                              \
        *token_cnt  = old_token_cnt;                                                                                        \
        return false;

bool parse_func_call_param(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const param_tree)
{
    assert(name_store  != nullptr);
    assert(code        != nullptr);
    assert(token_cnt   != nullptr);
    assert(param_tree  != nullptr);
    assert(*param_tree == nullptr);

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;

    *param_tree = new_FICTIONAL_AST_node(0);

    if (!parse_rvalue(name_store, code, token_cnt, &subtree))
    {
        fprintf_err($cur_token.token_line, "bad parameter\n");
        func_call_param_err_exit
    }
    fictional_merge_tree(*param_tree, subtree);

    while (token_char($cur_token, ','))
    {
        *token_cnt += 1;
        subtree     = nullptr;

        if (!parse_rvalue(name_store, code, token_cnt, &subtree))
        {
            fprintf_err($cur_token.token_line, "bad parameter\n");
            func_call_param_err_exit
        }
        fictional_merge_tree(*param_tree, subtree);
    }
    return true;
}
#undef func_call_param_err_exit
//---------------------------------------------------------------------------------------------------------------------------
#define return_err_exit                                                                                                     \
        AST_tree_dtor(*ret_tree);                                                                                           \
        *ret_tree   = nullptr;                                                                                              \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_return(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const ret_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(ret_tree   != nullptr);
    assert(*ret_tree  == nullptr);

    if (!token_return($cur_token)) return true;

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;
    *token_cnt             += 1;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { return_err_exit }
    *ret_tree = new_OP_RETURN_AST_node(0, subtree);

    if (!token_char($cur_token, ';'))
    {
        fprintf_err($cur_token.token_line, "expected ';' after return operatopr\n");
        return_err_exit
    }
    *token_cnt += 1;
    if ($scope == 1) MEET_RETURN = true;
    return true;
}
#undef  return_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define rvalue_err_exit                                                                                                     \
        log_error("error in parse_rvalue: old_token_cnt=%d\n", old_token_cnt);                                            \
        AST_tree_dtor(*rvalue_tree);                                                                                        \
        *rvalue_tree = nullptr;                                                                                             \
        *token_cnt   = old_token_cnt;                                                                                       \
        return false;

bool parse_rvalue(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const rvalue_tree)
{
    assert(name_store   != nullptr);
    assert(code         != nullptr);
    assert(token_cnt    != nullptr);
    assert(rvalue_tree  != nullptr);
    assert(*rvalue_tree == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_rvalue: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;

    if (!parse_assignment(name_store, code, token_cnt, rvalue_tree)) return false;
    if (!parse_op_or     (name_store, code, token_cnt, &subtree))
    {
        fprintf_err($cur_token.token_line, "bad rvalue\n");
        rvalue_err_exit
    }
    if  (*rvalue_tree != nullptr) assignment_merge_tree(*rvalue_tree, subtree);
    else *rvalue_tree  = subtree;
    return true;
}
#undef rvalue_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define assignment_err_exit                                                                                                 \
        log_error("error in parse_assignment: old_token_cnt=%d\n", old_token_cnt);                                        \
        AST_tree_dtor(*assign_tree);                                                                                        \
        *assign_tree = nullptr;                                                                                             \
        *token_cnt   = old_token_cnt;                                                                                       \
        return false;

bool parse_assignment(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const assign_tree)
{
    assert(name_store   != nullptr);
    assert(code         != nullptr);
    assert(token_cnt    != nullptr);
    assert(assign_tree  != nullptr);
    assert(*assign_tree == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_assignment: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    if (!token_char($next_token, '=')) return true;

    const int old_token_cnt = *token_cnt;
    AST_node       *subtree = nullptr;

    *assign_tree = new_OPERATOR_AST_node(ASSIGNMENT);

    if (parse_lvalue(name_store, code, token_cnt, &subtree) && subtree != nullptr)
    {
        *token_cnt          += 1;
        (*assign_tree)->left = subtree;
        subtree       ->prev = *assign_tree;
        subtree              = nullptr;
    }
    else
    {
        fprintf_err($cur_token.token_line, "expected lvalue before '='\n");
        assignment_err_exit
    }

    if (parse_assignment(name_store, code, token_cnt, &subtree))
    {
        (*assign_tree)->right = subtree;
        if (subtree != nullptr) subtree->prev  = *assign_tree;
        subtree               = nullptr;
    }
    else { assignment_err_exit }
    return true;
}
#undef assignment_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define or_err_exit                                                                                                         \
        log_error("error in parse_op_or: old_token_cnt=%d\n", old_token_cnt);                                                \
        AST_tree_dtor(*or_tree);                                                                                            \
        *or_tree   = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;

bool parse_op_or(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const or_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(or_tree    != nullptr);
    assert(*or_tree   == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_or: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_and(name_store, code, token_cnt, or_tree)) return false;
    
    while (token_or($cur_token))
    {
        AST_node *subtree = nullptr;
        *token_cnt       += 1;
        
        if (!parse_op_and(name_store, code, token_cnt, &subtree)) { or_err_exit }
        *or_tree = new_OPERATOR_AST_node(OP_OR, *or_tree, subtree);
    }
    return true;
}
#undef or_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define and_err_exit                                                                                                        \
        log_error("error in parse_op_and: old_token_cnt=%d\n", old_token_cnt);                                               \
        AST_tree_dtor(*and_tree);                                                                                           \
        *and_tree = nullptr;                                                                                                \
        *token_cnt = old_token_cnt;

bool parse_op_and(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const and_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(and_tree   != nullptr);
    assert(*and_tree  == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_and: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_equal(name_store, code, token_cnt, and_tree)) return false;

    while (token_and($cur_token))
    {
        AST_node *subtree = nullptr;
        *token_cnt       += 1;

        if (!parse_op_equal(name_store, code, token_cnt, &subtree)) { and_err_exit }
        *and_tree = new_OPERATOR_AST_node(OP_AND, *and_tree, subtree);
    }
    return true;
}
#undef and_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define equal_err_exit                                                                                                      \
        log_error("error in parse_op_equal: old_token_cnt=%d\n", old_token_cnt);                                             \
        AST_tree_dtor(*equal_tree);                                                                                         \
        *equal_tree = nullptr;                                                                                              \
        *token_cnt  = old_token_cnt;                                                                                        \
        return false;

bool parse_op_equal(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const equal_tree)
{
    assert(name_store  != nullptr);
    assert(code        != nullptr);
    assert(token_cnt   != nullptr);
    assert(equal_tree  != nullptr);
    assert(*equal_tree == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_equal: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_compare(name_store, code, token_cnt, equal_tree)) return false;

    if (token_eq($cur_token))
    {
        OPERATOR_TYPE ast_token_type = token_to_ast_op_type($cur_token);
        AST_node            *subtree = nullptr;
        *token_cnt                  += 1;

        if (!parse_op_compare(name_store, code, token_cnt, &subtree)) { equal_err_exit }
        *equal_tree = new_OPERATOR_AST_node(ast_token_type, *equal_tree, subtree);
    }
    return true;
}
#undef equal_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define compare_err_exit                                                                                                    \
        log_error("error in parse_op_compare: old_token_cnt=%d\n", old_token_cnt);                                           \
        AST_tree_dtor(*cmp_tree);                                                                                           \
        *cmp_tree  = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_compare(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const cmp_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(cmp_tree   != nullptr);
    assert(*cmp_tree  == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_compare: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_add_sub(name_store, code, token_cnt, cmp_tree)) return false;

    if (token_cmp($cur_token))
    {
        OPERATOR_TYPE ast_token_type = token_to_ast_op_type($cur_token);
        AST_node            *subtree = nullptr;
        *token_cnt                  += 1;

        if (!parse_op_add_sub(name_store, code, token_cnt, &subtree)) { compare_err_exit }
        *cmp_tree = new_OPERATOR_AST_node(ast_token_type, *cmp_tree, subtree);
    }
    return true;
}
#undef compare_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define add_sub_err_exit                                                                                                    \
        log_error("error in parse_op_add_sub: old_token_cnt=%d\n", old_token_cnt);                                           \
        AST_tree_dtor(*add_sub_tree);                                                                                       \
        *add_sub_tree = nullptr;                                                                                            \
        *token_cnt    = old_token_cnt;                                                                                      \
        return false;

bool parse_op_add_sub(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const add_sub_tree)
{
    assert(name_store    != nullptr);
    assert(code          != nullptr);
    assert(token_cnt     != nullptr);
    assert(add_sub_tree  != nullptr);
    assert(*add_sub_tree == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_value_op_add_sub: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_mul_div(name_store, code, token_cnt, add_sub_tree)) return false;

    while (token_add_sub($cur_token))
    {
        OPERATOR_TYPE ast_token_type = token_to_ast_op_type($cur_token);
        AST_node            *subtree = nullptr;
        *token_cnt                  += 1;

        if (!parse_op_mul_div(name_store, code, token_cnt, &subtree)) { add_sub_err_exit }
        *add_sub_tree = new_OPERATOR_AST_node(ast_token_type, *add_sub_tree, subtree);
    }
    return true;
}
#undef add_sub_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define mul_div_err_exit                                                                                                    \
        log_error("error in parse_op_mul_div: old_token_cnt=%d\n", old_token_cnt);                                        \
        AST_tree_dtor(*mul_div_tree);                                                                                       \
        *mul_div_tree = nullptr;                                                                                            \
        *token_cnt    = old_token_cnt;                                                                                      \
        return false;

bool parse_op_mul_div(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const mul_div_tree)
{
    assert(name_store    != nullptr);
    assert(code          != nullptr);
    assert(token_cnt     != nullptr);
    assert(mul_div_tree  != nullptr);
    assert(*mul_div_tree == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_mul_div: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_pow(name_store, code, token_cnt, mul_div_tree)) return false;

    while (token_mul_div($cur_token))
    {
        OPERATOR_TYPE ast_token_type = token_to_ast_op_type($cur_token);
        AST_node            *subtree = nullptr;
        *token_cnt                  += 1;

        if (!parse_op_pow(name_store, code, token_cnt, &subtree)) { mul_div_err_exit }
        *mul_div_tree = new_OPERATOR_AST_node(ast_token_type, *mul_div_tree, subtree);
    }
    return true;
}
#undef mul_div_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define pow_err_exit                                                                                                        \
        log_error("error in parse_op_pow: old_token_cnt=%d\n", old_token_cnt);                                            \
        AST_tree_dtor(*pow_tree);                                                                                           \
        *pow_tree  = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_pow(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const pow_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(pow_tree   != nullptr);
    assert(*pow_tree  == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_pow: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (!parse_op_not(name_store, code, token_cnt, pow_tree)) return false;

    if (token_pow($cur_token))
    {
        AST_node *subtree = nullptr;
        *token_cnt       += 1;

        if (!parse_op_not(name_store, code, token_cnt, &subtree)) { pow_err_exit }
        *pow_tree = new_OPERATOR_AST_node(OP_POW, *pow_tree, subtree);
    }
    return true;
}
#undef pow_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define not_err_exit                                                                                                        \
        log_error("error in parse_op_not: old_token_cnt=%d\n", old_token_cnt);                                            \
        AST_tree_dtor(*not_tree);                                                                                           \
        *not_tree  = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_op_not(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const not_tree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(not_tree   != nullptr);
    assert(*not_tree  == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_op_not: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;
    AST_node *subtree       = nullptr;

    if (token_char($cur_token, '!'))
    {
        *not_tree   = new_OPERATOR_AST_node(OP_NOT);
        *token_cnt += 1;
    }
    if (!parse_operand(name_store, code, token_cnt, &subtree)) { not_err_exit }
    if (*not_tree == nullptr)
    {
        *not_tree = subtree;
        return true;
    }
    (*not_tree)->left =   subtree;
    subtree    ->prev = *not_tree;
    return true;
}
#undef not_err_exit
//--------------------------------------------------------------------------------------------------------------------------
#define operand_err_exit                                                                                                    \
        log_error("error in parse_operand: old_token_cnt=%d\n", old_token_cnt);                                             \
        AST_tree_dtor(*operand);                                                                                            \
        *operand   = nullptr;                                                                                               \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_operand(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const operand)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(operand    != nullptr);
    assert(*operand   == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_operand: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    const int old_token_cnt = *token_cnt;

    if (token_char($cur_token, '('))
    {
        *token_cnt += 1;
        if (!parse_rvalue(name_store, code, token_cnt, operand)) { operand_err_exit }
        if (!token_char($cur_token, ')'))
        {
            fprintf_err($cur_token.token_line, "expected ')' after operand\n");
            operand_err_exit
        }
        *token_cnt += 1;
        return true;
    }
    if (!parse_unary_op    (name_store, code, token_cnt, operand)) { operand_err_exit }
    if (*operand != nullptr)                                         return true;
    if (!parse_rvalue_token(name_store, code, token_cnt, operand)) { operand_err_exit }
    if (*operand != nullptr)                                         return true;
    if (parse_func_call(name_store, code, token_cnt, operand) &&
        *operand != nullptr)                                         return true;
    
    fprintf_err($cur_token.token_line, "bad operand\n");
    operand_err_exit
}
#undef operand_err_exit
//--------------------------------------------------------------------------------------------------------------------------
bool parse_unary_op(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const unary_op)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(unary_op   != nullptr);
    assert(*unary_op  == nullptr);

    if (!parse_sqrt(name_store, code, token_cnt, unary_op)) return false;
    if (*unary_op != nullptr)                               return  true;

    if (!parse_sin (name_store, code, token_cnt, unary_op)) return false;
    return true;
}
//--------------------------------------------------------------------------------------------------------------------------
#define sqrt_err_exit                                                                                                       \
        AST_tree_dtor(*sqrt_op);                                                                                            \
        *sqrt_op = nullptr;                                                                                                 \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_sqrt(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const sqrt_op)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(sqrt_op    != nullptr);
    assert(*sqrt_op   == nullptr);

    const int old_token_cnt = *token_cnt;

    if (!token_sqrt($cur_token)) return true;

    *token_cnt += 1;
    if (!token_char($cur_token, '('))
    {
        fprintf_err($cur_token.token_line, "expected '(' before sqrt operand\n");
        sqrt_err_exit
    }
    *token_cnt += 1;
    AST_node *subtree = nullptr;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { sqrt_err_exit }

    *sqrt_op = new_OPERATOR_AST_node(OP_SQRT, subtree);
    subtree  = nullptr;

    if (!token_char($cur_token, ')'))
    {
        fprintf_err($cur_token.token_line, "expected ')' after sqrt operand\n");
        sqrt_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef sqrt_err_exit

#define sin_err_exit                                                                                                        \
        AST_tree_dtor(*sin_op);                                                                                             \
        *sin_op = nullptr;                                                                                                  \
        *token_cnt = old_token_cnt;                                                                                         \
        return false;

bool parse_sin(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const sin_op)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(sin_op     != nullptr);
    assert(*sin_op    == nullptr);

    const int old_token_cnt = *token_cnt;

    if (!token_sin($cur_token)) return true;

    *token_cnt += 1;
    if (!token_char($cur_token, '('))
    {
        fprintf_err($cur_token.token_line, "expected '(' before sin operand\n");
        sin_err_exit
    }
    *token_cnt += 1;
    AST_node *subtree = nullptr;

    if (!parse_rvalue(name_store, code, token_cnt, &subtree)) { sin_err_exit }

    *sin_op = new_OPERATOR_AST_node(OP_SIN, subtree);
    subtree = nullptr;

    if (!token_char($cur_token, ')'))
    {
        fprintf_err($cur_token.token_line, "expected ')' after sin operand\n");
        sin_err_exit
    }
    *token_cnt += 1;
    return true;
}
#undef sin_err_exit
//--------------------------------------------------------------------------------------------------------------------------
bool parse_rvalue_token(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const subtree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(subtree    != nullptr);
    assert(*subtree   == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_rvalue_token: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    if ($cur_token.type == DBL_NUM)
    {
        *subtree    = new_NUMBER_AST_node($cur_token.dbl_num_val);
        *token_cnt += 1;
        return true;
    }
    if (token_undef($cur_token) && !token_char($next_token, '('))
    {
        if (parse_lvalue(name_store, code, token_cnt, subtree)) return true;
        return false;
    }
    return true;
}
//--------------------------------------------------------------------------------------------------------------------------
bool parse_lvalue(dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const subtree)
{
    assert(name_store != nullptr);
    assert(code       != nullptr);
    assert(token_cnt  != nullptr);
    assert(subtree    != nullptr);
    assert(*subtree   == nullptr);
    //>>>>>>>>>>>>>>>>>>>>>>>>>>
    log_message("start parse_lvalue: old_token_cnt=%d\n", *token_cnt);
    //<<<<<<<<<<<<<<<<<<<<<<<<<<
    if (!token_undef($cur_token)) return true;

    int  var_index = -1;
    if ((var_index = var_name_list_defined_var(&$var_store, code, &$cur_token)) != -1)
    {
        *subtree = new_VARIABLE_AST_node(var_index);
        *token_cnt += 1;
        return true;
    }
    fprintf_err($cur_token.token_line, "undefined variable\n");
    return false;
}

OPERATOR_TYPE token_to_ast_op_type(const token cur_token)
{
    if (token_char  (cur_token, '+')) return OP_ADD;
    if (token_char  (cur_token, '-')) return OP_SUB;
    if (token_char  (cur_token, '*')) return OP_MUL;
    if (token_char  (cur_token, '/')) return OP_DIV;
    if (token_char  (cur_token, '^')) return OP_POW;
    if (token_char  (cur_token, '>')) return OP_ABOVE;
    if (token_char  (cur_token, '<')) return OP_BELOW;
    if (token_char  (cur_token, '!')) return OP_NOT;

    if (token_e     (cur_token))      return OP_EQUAL;
    if (token_ne    (cur_token))      return OP_NOT_EQUAL;
    if (token_ae    (cur_token))      return OP_ABOVE_EQUAL;
    if (token_be    (cur_token))      return OP_BELOW_EQUAL;
    if (token_input (cur_token))      return OP_INPUT;
    if (token_output(cur_token))      return OP_OUTPUT;
    if (token_sqrt  (cur_token))      return OP_SQRT;

    assert(false && "default case in token_to_ast_op_type");
    return OP_ADD;
}

//===========================================================================================================================
// BOOL TOKEN_...
//===========================================================================================================================

bool token_cmp    (const token cur_token) { return token_ae(cur_token) || token_be(cur_token) || token_char(cur_token, '>') || token_char(cur_token, '<'); }
bool token_eq     (const token cur_token) { return token_e (cur_token) || token_ne(cur_token); }

bool token_mul_div(const token cur_token) { return token_char(cur_token, '*') || token_char(cur_token, '/'); }
bool token_add_sub(const token cur_token) { return token_char(cur_token, '+') || token_char(cur_token, '-'); }
bool token_pow    (const token cur_token) { return token_char(cur_token, '^'); }

bool token_dbl    (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == DOUBLE; }
bool token_if     (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == IF    ; }
bool token_else   (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == ELSE  ; }
bool token_while  (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == WHILE ; }
bool token_return (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == RETURN; }
bool token_input  (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == INPUT ; }
bool token_output (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == OUTPUT; }
bool token_sqrt   (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == SQRT  ; }
bool token_sin    (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == SIN   ; }

bool token_e      (const token cur_token) { return cur_token.type == KEY_CHAR_DOUBLE && cur_token.key_dbl_char_val == EQUAL      ; }
bool token_ne     (const token cur_token) { return cur_token.type == KEY_CHAR_DOUBLE && cur_token.key_dbl_char_val == NOT_EQUAL  ; }
bool token_ae     (const token cur_token) { return cur_token.type == KEY_CHAR_DOUBLE && cur_token.key_dbl_char_val == ABOVE_EQUAL; }
bool token_be     (const token cur_token) { return cur_token.type == KEY_CHAR_DOUBLE && cur_token.key_dbl_char_val == BELOW_EQUAL; }
bool token_and    (const token cur_token) { return cur_token.type == KEY_CHAR_DOUBLE && cur_token.key_dbl_char_val == AND        ; }
bool token_or     (const token cur_token) { return cur_token.type == KEY_CHAR_DOUBLE && cur_token.key_dbl_char_val == OR         ; }

bool token_undef  (const token cur_token) { return cur_token.type == UNDEF_TOKEN; }

bool token_char   (const token cur_token, const char cmp) { return cur_token.type == KEY_CHAR && cur_token.key_char_val == cmp; }

//===========================================================================================================================
// AST_NODE DESCENT
//===========================================================================================================================

void fictional_merge_tree(AST_node *const main_tree, AST_node *const subtree)
{
    assert(main_tree != nullptr);
    assert(subtree   != nullptr);

    assert(main_tree->type == FICTIONAL);
    assert(subtree  ->type != FICTIONAL);

    if (main_tree->left == nullptr)
    {
        main_tree->left = subtree;
        subtree  ->prev = main_tree;
        return;
    }
    if (main_tree->right == nullptr)
    {
        main_tree->right = new_FICTIONAL_AST_node(0, subtree, nullptr, main_tree);
        subtree  ->prev  = main_tree->right;
        return;
    }
    fictional_merge_tree(main_tree->right, subtree);
}

void assignment_merge_tree(AST_node *const main_tree, AST_node *const subtree)
{
    assert(main_tree != nullptr);
    assert(subtree   != nullptr);

    assert(main_tree->type == OPERATOR && main_tree->value.op_type == ASSIGNMENT);
    
    if (main_tree->right == nullptr)
    {
        main_tree->right = subtree;
        subtree  ->prev  = main_tree;
        return;
    }
    assignment_merge_tree(main_tree->right, subtree);
}

int get_subtree_num(const AST_node *const node)
{
    if (node == nullptr) return 0;

    if ($type == FICTIONAL) return get_subtree_num(L) + get_subtree_num(R);
    return 1;
}

//===========================================================================================================================
// SOURCE_CTOR_DTOR
//===========================================================================================================================

source *new_source(const char *src_file)
{
    assert(src_file != nullptr);

    source *new_code = (source *) log_calloc(1, sizeof(source));
    
    if (new_buff (src_file, new_code) == false) { source_dtor(new_code); return nullptr; }
    new_lexis(new_code);

    return new_code;
}

bool new_buff(const char *src_file, source *const code)
{
    assert(src_file != nullptr);
    assert(code     != nullptr);

    buff_data = (const char *) read_file(src_file, &buff_size);
    buff_line = 1;
    buff_pos  = 0;

    if (buff_data == nullptr)
    {
        fprintf(stderr, "can't open sorce file \"%s\"\n", src_file);
        return false;
    }
    return true;
}

void new_lexis(source *const code)
{
    assert(code != nullptr);

    lexis_data     = (token *) log_calloc((size_t) buff_size, sizeof(token));
    lexis_pos      = 0;
    lexis_capacity = buff_size;
}

void source_dtor(source *const code) //only if parameter "code" was created by malloc or calloc
{
    assert(code != nullptr);

    log_free((char *)buff_data);
    log_free(       lexis_data);
    log_free(code);
}

//===========================================================================================================================
// LEXICAL_ANALYSIS
//===========================================================================================================================

void lexical_analyzer(source *const code)
{
    assert(code != nullptr);

    skip_source_spaces(code);

    while (buff_pos < buff_size && buff_data[buff_pos] != '\0')
    {
        if (comment(code)) continue;
        if (key_char(buff_data[buff_pos])) create_key_char_token(code);
        else
        {
            int token_beg = buff_pos;
            int token_len = get_another_token(code);

            if      (get_dbl_num        (code, token_beg, token_len)) create_dbl_num_token        (code, token_beg, token_len);
            else if (get_key_double_char(code, token_beg, token_len)) create_key_double_char_token(code, token_beg, token_len);
            else if (get_key_word_type  (code, token_beg, token_len)) create_key_word_token       (code, token_beg, token_len);
            else                                                      create_undef_token          (code, token_beg, token_len);
        }
        skip_source_spaces(code);
    }
}

int get_another_token(source *const code)
{
    assert(code != nullptr);

    int token_len = 0;
    while (buff_pos < buff_size && !split_char(buff_data[buff_pos]))
    {
        ++buff_pos;
        ++token_len;
    }
    return token_len;
}
                                                                                                //default type = nullptr
bool get_key_word_type(source *const code, const int token_beg, const int token_len, KEY_WORD_TYPE *const type)
{
    assert(code != nullptr);

    for (size_t i = 0; i * sizeof(char *) < sizeof(KEY_WORD_NAMES); ++i)
    {
        if (!strncmp(KEY_WORD_NAMES[i], buff_data + token_beg, (size_t)token_len))
        {
            if (type != nullptr) *type = (KEY_WORD_TYPE) i;
            return true;
        }
    }
    return false;
}
                                                                                                        // default type = nullptr
bool get_key_double_char(source *const code, const int token_beg, const int token_len, KEY_CHAR_DOUBLE_TYPE *const type)
{
    assert(code != nullptr);

    for (size_t i = 0; i * sizeof(char *) < sizeof(KEY_CHAR_DOUBLE_NAMES); ++i)
    {
        if (!strncmp(KEY_CHAR_DOUBLE_NAMES[i], buff_data + token_beg, (size_t)token_len))
        {
            if (type != nullptr) *type = (KEY_CHAR_DOUBLE_TYPE) i;
            return true;
        }
    }
    return false;
}
                                                                                   //default num = nullptr
bool get_dbl_num(source *const code, const int token_beg, const int token_len, double *const num)
{
    assert(code != nullptr);

    double dbl_num = 0;
    int    num_len = 0;

    if (sscanf(buff_data + token_beg, "%lf%n", &dbl_num, &num_len) != 1) return false;
    if (num_len != token_len)                                            return false;

    if (num != nullptr) *num = dbl_num;
    return true;
}

bool comment(source *const code)
{
    assert(code != nullptr);

    if (buff_data[buff_pos] == '#')
    {
        skip_source_line(code);
        return true;
    }
    return false;
}

bool key_char(const char to_check)
{
    for (int i = 0; KEY_CHAR_NAMES[i] != '\0'; ++i)
    {
        if (KEY_CHAR_NAMES[i] == to_check) return true;
    }
    return false;
}

bool split_char(const char to_check)
{
    if (to_check == '\0')   return true;
    if (isspace (to_check)) return true;
    if (key_char(to_check)) return true;

    return false;
}

//===========================================================================================================================
// TOKEN_CTOR_DTOR
//===========================================================================================================================

void create_key_word_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_WORD;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;

    get_key_word_type(code, token_beg, token_len, &lexis_data[lexis_pos].key_word_val);
    ++lexis_pos;
}

void create_key_char_token(source *const code)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_CHAR;
    lexis_data[lexis_pos].token_beg     = buff_pos;
    lexis_data[lexis_pos].token_line    = buff_line;
    lexis_data[lexis_pos].key_char_val  = buff_data[buff_pos++];

    ++lexis_pos;
}

void create_key_double_char_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_CHAR_DOUBLE;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;

    get_key_double_char(code, token_beg, token_len, &lexis_data[lexis_pos].key_dbl_char_val);
    ++lexis_pos;
}

void create_dbl_num_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = DBL_NUM;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;

    get_dbl_num(code, token_beg, token_len, &lexis_data[lexis_pos].dbl_num_val);
    ++lexis_pos;
}

void create_undef_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = UNDEF_TOKEN;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;
    lexis_data[lexis_pos].token_len_val = token_len;

    ++lexis_pos;
}

//===========================================================================================================================
// SKIP_SOURCE
//===========================================================================================================================

void skip_source_line(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && buff_data[buff_pos] != '\n') ++buff_pos;
    
    if (buff_pos < buff_size)
    {
        ++buff_pos;
        ++buff_line;
    }
    skip_source_spaces(code);
}

void skip_source_spaces(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && isspace(buff_data[buff_pos]))
    {
        if (buff_data[buff_pos] == '\n') ++buff_line;
        ++buff_pos;
    }
}

//===========================================================================================================================
// DUMP
//===========================================================================================================================

void var_name_list_text_dump(var_name_list *const var_store)
{
    if (var_store == nullptr)
    {
        log_warning("var_store to dump is nullptr\n");
        return;
    }

    log_message("\n"
                "var_store\n"
                "{\n"
                "    size     = %d\n"
                "    capacity = %d\n"
                "    var      = %p\n"
                "    {\n",
                var_store->size,
                var_store->capacity,
                var_store->var);

    for (int i = 0; i < var_store->size; ++i)
    {
        const var_info *const cur_var = var_store->var+i;

        log_message("        %d\n"
                    "        {\n"
                    "            name     = \"%s\"\n"
                    "            name_len = %d\n"
                    "        }\n",
                    i,
                    cur_var->name,
                    cur_var->name_len);
    }
    log_message("    }\n"
                "}\n");
}

void func_name_list_text_dump(func_name_list *const func_store)
{
    if (func_store == nullptr)
    {
        log_warning("func_store to dump is nullptr\n");
        return;
    }

    log_message("\n"
                "func_store\n"
                "{\n"
                "    size     = %d\n"
                "    capacity = %d\n"
                "    func     = %p\n"
                "    {\n",
                func_store->size,
                func_store->capacity,
                func_store->func);

    for (int i = 0; i < func_store->size; ++i)
    {
        const func_info *const cur_func = func_store->func+i;

        log_message("        %d\n"
                    "        {\n"
                    "            name     = \"%s\"\n"
                    "            name_len = %d\n"
                    "            arg_num  = %d\n"
                    "            args     = %p\n"
                    "            {\n",
                    i,
                    cur_func->name,
                    cur_func->name_len,
                    cur_func->arg_num,
                    cur_func->args);
        
        const arg_list *const cur_args = &cur_func->args;

        log_message("                size       = %d\n"
                    "                capacity   = %d\n"
                    "                name_index = %p\n"
                    "                {\n"
                    "                    ",
                    cur_args->size,
                    cur_args->capacity,
                    cur_args->name_index);

        for (int j = 0; j < cur_args->size; ++j) log_message("%d ", cur_args->name_index[j]);
        log_message("\n"
                    "                }\n"
                    "            }\n"
                    "        }\n");
    }
    log_message("    }\n"
                "}\n");
}

void source_text_dump(source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr\n");
        return;
    }

    log_message("\n"
                "code\n"
                "{\n"
                "    buff\n"
                "    {\n"
                "        data = %p\n"
                "        pos  = %d\n"
                "        line = %d\n"
                "        size = %d\n"
                "    }\n"
                "    lexis\n"
                "    {\n"
                "        data     = %p\n"
                "        pos      = %d\n"
                "        capacity = %d\n"
                "    }\n"
                "}\n\n",
                buff_data, buff_pos, buff_line, buff_size, lexis_data, lexis_pos, lexis_capacity);
}

void lexis_graphviz_dump(source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr(%d)\n", __LINE__);
        return;
    }
    static int cur = 0;

    char dump_txt[GRAPHVIZ_SIZE_FILE] = "";
    char dump_png[GRAPHVIZ_SIZE_FILE] = "";

    sprintf(dump_txt, "dump_txt/lexis%d.txt", cur);
    sprintf(dump_png, "dump_png/lexis%d.png", cur);

    FILE *stream_txt =  fopen(dump_txt, "w");
    if   (stream_txt == nullptr)
    {
        log_error("can't open dump file(%d)\n", __LINE__);
        return;
    }
    ++cur;

    setvbuf(stream_txt, nullptr, _IONBF, 0);
    fprintf(stream_txt, "%s", LEXIS_GRAPHVIZ_HEADER);

    do_lexis_graphviz_dump(code, stream_txt);

    fprintf(stream_txt, "}\n");
    system_graphviz_dump(dump_txt, dump_png);

    fclose(stream_txt);
}


void do_lexis_graphviz_dump(source *const code, FILE *const stream)
{
    assert(code   != nullptr);
    assert(stream != nullptr);

    for (int i = 0; i < lexis_pos; ++i)
    {
        graphviz_dump_token(lexis_data+i, stream, i);
    }
    for (int i = 0; i < lexis_pos - 1; ++i)
    {
        graphviz_dump_edge(i, i+1, stream);
    }
}

void graphviz_dump_edge(const int token_num_from, const int token_num_to, FILE *const stream)
{
    assert(stream != nullptr);

    fprintf(stream, "node%d->node%d[weight=0, style=\"invis\"]\n", token_num_from, token_num_to);
}

void graphviz_dump_token(const token *const cur_token, FILE *const stream, const int token_num)
{
    assert(cur_token != nullptr);
    assert(stream    != nullptr);

    GRAPHVIZ_COLOR     color = BLACK;
    GRAPHVIZ_COLOR fillcolor = BLACK;

    switch (cur_token->type)
    {
        case KEY_WORD:          fillcolor = LIGHT_BLUE;
                                    color =  DARK_BLUE;
                                break;
        case KEY_CHAR_DOUBLE:   fillcolor = LIGHT_GREEN;
                                    color =  DARK_GREEN;
                                break;
        case KEY_CHAR:          fillcolor = LIGHT_PINK;
                                    color =   DARK_RED;
                                break;
        case DBL_NUM :          fillcolor = LIGHT_GREY;
                                    color =      BLACK;
                                break;
        case UNDEF_TOKEN:       fillcolor = LIGHT_ORANGE;
                                    color =  DARK_ORANGE;
                                break;
        default:                break;
    }
    graphviz_describe_token(cur_token, stream, color, fillcolor, token_num);
}

void graphviz_describe_token(const token *const cur_token, FILE *const stream, const GRAPHVIZ_COLOR     color,
                                                                               const GRAPHVIZ_COLOR fillcolor, const int token_num)
{
    assert(cur_token != nullptr);
    assert(stream    != nullptr);

    char token_value[GRAPHVIZ_SIZE_CMD] = {};
    get_token_value_message(cur_token, token_value);

    fprintf(stream, "node%d[color=\"%s\", fillcolor=\"%s\", label=\"{token number %d\\n | type: %s\\n | token_beg: %d\\n | token_line: %d\\n | %s}\"]\n",
                         token_num,
                                    GRAPHVIZ_COLOR_NAMES[color],
                                                      GRAPHVIZ_COLOR_NAMES[fillcolor],
                                                                                  token_num,
                                                                                                TOKEN_TYPE_NAMES[cur_token->type],
                                                                                                                   cur_token->token_beg,
                                                                                                                                       cur_token->token_line,
                                                                                                                                               token_value);
}

void get_token_value_message(const token *const cur_token, char *const token_value)
{
    assert(cur_token   != nullptr);
    assert(token_value != nullptr);

    switch(cur_token->type)
    {
        case KEY_WORD   :       sprintf(token_value, "word: %s", KEY_WORD_NAMES[cur_token->key_word_val]);
                                break;
        case KEY_CHAR_DOUBLE:   sprintf(token_value, "dbl_char: %s", KEY_CHAR_DOUBLE_NAMES[cur_token->key_dbl_char_val]);
                                break;
        case KEY_CHAR   :       if      (cur_token->key_char_val == '{') sprintf(token_value, "char: opened fig");
                                else if (cur_token->key_char_val == '}') sprintf(token_value, "char: closed fig");
                                else                                     sprintf(token_value, "char: \\\'%c\\\'", cur_token->key_char_val);
                                break;
        case DBL_NUM    :       sprintf(token_value, "num: %lf", cur_token->dbl_num_val);
                                break;
        case UNDEF_TOKEN:       sprintf(token_value, "len: %d", cur_token->token_len_val);
                                break;
        default:                break;
    }
}

void system_graphviz_dump(char *const dump_txt, char *const dump_png)
{
    assert(dump_txt != nullptr);
    assert(dump_png != nullptr);

    dump_txt[GRAPHVIZ_SIZE_FILE - 1] = '\0';
    dump_png[GRAPHVIZ_SIZE_FILE - 1] = '\0';

    char cmd[GRAPHVIZ_SIZE_CMD] = "";

    sprintf       (cmd, "dot %s -T png -o %s", dump_txt, dump_png);
    system        (cmd);
    log_message   ("<img src=%s>\n", dump_png);
}