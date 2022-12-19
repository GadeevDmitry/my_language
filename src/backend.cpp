#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../lib/logs/log.h"
#include "../lib/read_write/read_write.h"
#include "../lib/algorithm/algorithm.h"

#include "backend.h"
#include "terminal_colors.h"

#define fprintf_err(message) fprintf(stderr, TERMINAL_RED "ERROR: " TERMINAL_CANCEL "%s", message)

//===========================================================================================================================
// MAIN
//===========================================================================================================================

#define main_err_exit                                                                                                       \
        translator_dtor(&ast_asm);                                                                                          \
        AST_tree_dtor  ( tree   );                                                                                          \
        return 0;

int main(const int argc, const char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "you should give two parameters: ast format file and assembler file to translate in\n");
        return 0;
    }

    int buff_size    = 0;
    int buff_pos     = 0;
    const char *buff = (const char *) read_file(argv[1], &buff_size);

    translator ast_asm = {};
    AST_node     *tree = nullptr;
    int       main_num = backend_parse(&ast_asm, &tree, buff, buff_size, &buff_pos);
    log_free((char *) buff);

    if (main_num == -1)
    {
        fprintf_err("backend parse failed\n");
        return 0;
    }
    //>>>>>>>>>>>>
    AST_tree_graphviz_dump(tree);
    //<<<<<<<<<<<<
    FILE *stream = fopen(argv[2], "w");
    if   (stream == nullptr)
    {
        fprintf(stderr, "can't open \"%s\"", argv[2]);
        main_err_exit
    }
    int rex_begin = 0; // в регистре rex лежит отступ в RAM, rex_begin - отступ, равный количеству глобальных переменных
    if (!fill_global_scope(&ast_asm.mem_glob, tree, &rex_begin)) { main_err_exit }

    backend_header(stream, main_num, rex_begin);
    if (!translate_backend(&ast_asm, tree, stream))
    {
        fprintf_err("assembling failed\n");
    }
    else
    {
        int         pattern_size  = 0;
        const char *pattern_buff  = (const char *) read_file(NAME_PATTERN, &pattern_size);
        assert     (pattern_buff != nullptr);

        fwrite  (pattern_buff, sizeof(char), pattern_size, stream);
        fprintf (stderr, TERMINAL_GREEN "assembling success\n" TERMINAL_CANCEL);
        log_free((char *) pattern_buff);
    }
    fclose(stream);
    main_err_exit
}

//===========================================================================================================================
// TRANSLATE
//===========================================================================================================================

int TAG_CNT = 1; // счетчик меток в ассемблерном коде

void backend_header(FILE *const stream, const int main_num, const int rex_begin)
{
    set_rex(rex_begin, stream);
    fprintf(stream, "call def_%d\n"
                    "\n"
                    "hlt    #header\n\n", main_num);
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_backend(translator *const ast_asm, const AST_node *const node, FILE *const stream)
{
    assert(ast_asm != nullptr);
    assert(stream  != nullptr);

    if (node  ==   nullptr) return true;
    if ($type == FICTIONAL)
    {
        if (!translate_backend(ast_asm, L, stream)) return false;
        if (!translate_backend(ast_asm, R, stream)) return false;

        return true;
    }
    if ($type ==  VAR_DECL) return true;
    if ($type == FUNC_DECL)
    {
        $relative = 0;
        translator_new_scope(ast_asm);

        fprintf(stream, "def_%d:\n", $func_index);

        if (!translate_func_args  (ast_asm, L, stream))       return false;
        if (!translate_distributor(ast_asm, R, stream, true)) return false;

        translator_del_scope(ast_asm);
        return true;
    }
    fprintf_err("there only function and variable declarations allowed in global scope\n");
    return false;
}

bool translate_func_args(translator *const ast_asm, const AST_node *const node, FILE *const stream)
{
    assert(ast_asm != nullptr);
    assert(stream  != nullptr);

    if (node == nullptr) return true;

    if ($type == FICTIONAL)
    {
        if (!translate_func_args(ast_asm, L, stream)) return false;
        if (!translate_func_args(ast_asm, R, stream)) return false;

        return true;
    }
    if ($type == VARIABLE)
    {
        if ($loc.size <= $var_index)
        {
            fprintf_err("variable index more than variable store size\n");
            return false;
        }
        stack_push($loc.ram+$var_index, &$relative);
        $relative += 1;
        return true;
    }
    fprintf_err("function arguments must be lvalues\n");
    return false;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_distributor(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(stream  != nullptr);

    if (node == nullptr) return true;

    switch ($type)
    {
        case FICTIONAL: return translate_fictional(ast_asm, node, stream, independent_op);
        case NUMBER   : return translate_number   (ast_asm, node, stream, independent_op);
        case VARIABLE : return translate_variable (ast_asm, node, stream, independent_op);
        case OP_IF    : return translate_if       (ast_asm, node, stream, independent_op);

        case IF_ELSE  : fprintf_err("\"IF_ELSE\" node has no \"IF\" parent\n");
                        return false;

        case OP_WHILE : return translate_while(ast_asm, node, stream, independent_op);
        case OPERATOR : return translate_operator(ast_asm, node, stream, independent_op);
        case VAR_DECL : return translate_var_decl(ast_asm, node, stream, independent_op);

        case FUNC_DECL: fprintf_err("\"FUNC_DECL\" into \"FUNC_DECL\" subtree\n");
                        return false;
        
        case FUNC_CALL: return translate_func_call(ast_asm, node, stream, independent_op);
        case OP_RETURN: return translate_return   (ast_asm, node, stream, independent_op);

        default       : assert(false && "default case in translate_distributor()");
                        return false;
    }
    return false;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_fictional(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == FICTIONAL);

    if (!translate_distributor(ast_asm, L, stream, independent_op)) return false;
    if (!translate_distributor(ast_asm, R, stream, independent_op)) return false;

    return true;
}

bool translate_var_decl(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independnt_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == VAR_DECL);

    if (!independnt_op)
    {
        fprintf_err("variable declaration must be independent operator\n");
        return false;
    }
    if ($loc.size <= $var_index)
    {
        fprintf_err("variable index more than variable store size\n");
        return false;
    }
    if (translator_redefined_var(ast_asm, $var_index))
    {
        fprintf_err("redefined variable\n");
        return false;
    }
    stack_push(&$loc.ram[$var_index], &$relative);
    $relative += 1;

    return true;
}

bool translate_number(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == NUMBER);

    if (independent_op)
    {
        fprintf_err("number can't be independent operator\n");
        return false;
    }
    fprintf(stream, "push %lf   #number\n", $dbl_num);

    return true;
}

bool translate_variable(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == VARIABLE);

    if (independent_op)
    {
        fprintf_err("variable can't be independent operator\n");
        return false;
    }
    if ($loc.size <= $var_index)
    {
        fprintf_err("variable index more than variable store size\n");
        return false;
    }
    if (translator_undefined_var(ast_asm, $var_index))
    {
        fprintf_err("undefined variable\n");
        return false;        
    }
    if (!stack_empty($loc.ram+$var_index)) fprintf(stream, "push [rex+%d]\n", *(int *) stack_front($loc.ram+$var_index));
    else                                   fprintf(stream, "push [%d]\n", $glob.ram[$var_index]);

    return true;
}

bool translate_if(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == OP_IF);

    if (!independent_op)
    {
        fprintf_err("IF must be independent operator\n");
        return false;
    }
    if (L == nullptr)
    {
        fprintf_err("\"condition\" subtree of \"IF\" node is nullptr\n");
        return false;
    }
    fprintf(stream, "\n"
                    "#OPERATOR IF condition\n");

    
    translate_distributor(ast_asm, L, stream, false);       //IF condition

    const int tag_else   = TAG_CNT++;
    const int tag_if_end = TAG_CNT++;

    fprintf(stream, "\n"
                    "push 0    #OPERATOR IF begin\n"
                    "je tag_%d #jump to case ELSE if zero condition\n", tag_else);
 
    translator_new_scope (ast_asm);
    translate_distributor(ast_asm, R->left, stream, true);  //case IF operators
    translator_del_scope (ast_asm);

    fprintf(stream, "\n"
                    "jmp tag_%d #case IF end\n"
                    "tag_%d:    #case ELSE tag\n", tag_if_end, tag_else);

    translator_new_scope (ast_asm);
    translate_distributor(ast_asm, R->right, stream, true); //case ELSE operators
    translator_del_scope (ast_asm);

    fprintf(stream, "\n"
                    "tag_%d:    #OPERATOR IF end\n", tag_if_end);

    return true;
}

bool translate_while(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == OP_WHILE);

    if (!independent_op)
    {
        fprintf_err("WHILE must be independent operator\n");
        return false;
    }
    if (L == nullptr)
    {
        fprintf_err("\"conditon\" subtree of \"WHILE\" node is nullptr\n");
        return false;
    }

    const int tag_while_condition = TAG_CNT++;
    const int tag_while_end       = TAG_CNT++;

    fprintf(stream, "tag_%d:    #OPERATOR WHILE condition\n", tag_while_condition);

    translate_distributor(ast_asm, L, stream, false);   //WHILE condition

    fprintf(stream, "\n"
                    "push 0\n"
                    "je tag_%d  #jump to the end of cycle if zero condition\n", tag_while_end);
    
    translator_new_scope (ast_asm);
    translate_distributor(ast_asm, R, stream, true);    //WHILE operators
    translator_del_scope (ast_asm);

    fprintf(stream, "\n"
                    "jmp tag_%d #jump to WHILE condition\n"
                    "tag_%d:    #OPERATOR WHILE end\n", tag_while_condition, tag_while_end);

    return true;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_operator(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert($type   == OPERATOR);

    switch ($op_type)
    {
        case OP_ADD         :
        case OP_SUB         :
        case OP_MUL         :
        case OP_DIV         :
        case OP_POW         :

        case OP_EQUAL       :
        case OP_ABOVE       :
        case OP_BELOW       :
        case OP_ABOVE_EQUAL :
        case OP_BELOW_EQUAL :
        case OP_NOT_EQUAL   :

        case OP_OR          :
        case OP_AND         : return translate_closed_binary_operator(ast_asm, node, stream, independent_op);

        case OP_INPUT       :
        case OP_OUTPUT      : return translate_opened_unary_operator (ast_asm, node, stream, independent_op);

        case OP_NOT         : return translate_closed_unary_operator (ast_asm, node, stream, independent_op);
        
        case ASSIGNMENT     : return translate_assignment            (ast_asm, node, stream, independent_op);
        default : assert(false && "default case in translate_operator()\n");
                  return false;
    }
    return false;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_closed_binary_operator(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == OPERATOR);

    if (independent_op)
    {
        fprintf(stderr, "\"%s\" can't be independent operator\n", OPERATOR_NAMES[$op_type]);
        return false;
    }
    if (L == nullptr || R == nullptr)
    {
        fprintf(stderr, "\"%s\" has must have two operands\n", OPERATOR_NAMES[$op_type]);
        return false;
    }
    if (!translate_distributor(ast_asm, L, stream, false)) return false;
    if (!translate_distributor(ast_asm, R, stream, false)) return false;

    switch ($op_type)
    {
        case OP_ADD         : fprintf(stream, "add\n"); break;
        case OP_SUB         : fprintf(stream, "sub\n"); break;
        case OP_MUL         : fprintf(stream, "mul\n"); break;
        case OP_DIV         : fprintf(stream, "div\n"); break;
        case OP_POW         : fprintf(stream, "pow\n"); break;

        case OP_EQUAL       : fprintf(stream, "call def_operator_eq\n" ); break;
        case OP_ABOVE       : fprintf(stream, "call def_operator_a\n"  ); break;
        case OP_BELOW       : fprintf(stream, "call def_operator_b\n"  ); break;
        case OP_ABOVE_EQUAL : fprintf(stream, "call def_operator_ae\n" ); break;
        case OP_BELOW_EQUAL : fprintf(stream, "call def_operator_be\n" ); break;
        case OP_NOT_EQUAL   : fprintf(stream, "call def_operator_neq\n"); break;

        case OP_OR          : fprintf(stream, "call def_operator_or\n" ); break;
        case OP_AND         : fprintf(stream, "call def_operator_and\n"); break;

        default             : assert(false && "default case in translate_closed_binary_operator()\n"); return false;
    }
    return true;
}
//---------------------------------------------------------------------------------------------------------------------------
bool translate_opened_unary_operator(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == OPERATOR);

    if (!independent_op)
    {
        fprintf(stderr, "\"%s\" must be independent operator\n", OPERATOR_NAMES[$op_type]);
        return false;
    }
    if (L == nullptr || R != nullptr)
    {
        fprintf(stderr, "\"%s\" must have one operand\n", OPERATOR_NAMES[$op_type]);
        return false;
    }
    switch ($op_type)
    {
        case OP_INPUT : return translate_operator_input (ast_asm, node, stream);
        case OP_OUTPUT: return translate_operator_output(ast_asm, node, stream);
        default       : assert(false && "default case in translate_opened_unary_operator()\n"); return false;
    }
    return false;
}

bool translate_assignment(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);

    if (L == nullptr || R == nullptr)
    {
        fprintf_err("\"assignment\" must have two operands\n");
        return false;
    }

    fprintf(stream, "\n"
                    "#OPERATOR assignment begin\n");

    if (!translate_distributor (ast_asm, R, stream, false)) return false;
    if (!translate_pop_variable(ast_asm, L, stream))        return false;

    if (!independent_op) translate_variable(ast_asm, L, stream, false);

    fprintf(stream, "#OPERATOR assignment end\n");
    return true;
}

bool translate_operator_input(translator *const ast_asm, const AST_node *const node, FILE *const stream)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);

    fprintf(stream, "\n"
                    "#OPERATOR input begin\n" 
                    "in\n");

    if (!translate_pop_variable(ast_asm, L, stream)) return false;

    fprintf(stream, "#OPERATOR input end\n");
    return true;
}

bool translate_pop_variable(translator *const ast_asm, const AST_node *const node, FILE *const stream)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);

    if ($type != VARIABLE)
    {
        fprintf_err("\"input or assignment\" operand must be lvalue\n");
        return false;
    }
    if (translator_undefined_var(ast_asm, $var_index))
    {
        fprintf_err("undefined var\n");
        return false;
    }
    if (!stack_empty($loc.ram+$var_index)) fprintf(stream, "pop [rex+%d]\n", *(int *) stack_front($loc.ram+$var_index));
    else                                   fprintf(stream, "pop [%d]\n",     $glob.ram[$var_index]);

    return true;
}

bool translate_operator_output(translator *const ast_asm, const AST_node *const node, FILE *const stream)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);

    fprintf(stream, "\n"
                    "#OPERATOR output begin\n");

    if (!translate_distributor(ast_asm, L, stream, false)) return false;

    fprintf(stream, "out\n"
                    "pop void\n"
                    "#OPERATOR output end\n");
    return true;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_closed_unary_operator(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == OPERATOR);

    if (independent_op)
    {
        fprintf(stderr, "\"%s\" can't be independent operator\n", OPERATOR_NAMES[$op_type]);
        return false;
    }
    if (L == nullptr || R != nullptr)
    {
        fprintf(stderr, "\"%s\" must have one operand\n", OPERATOR_NAMES[$op_type]);
        return false;
    }
    if (!translate_distributor(ast_asm, L, stream, false)) return false;

    switch ($op_type)
    {
        case OP_NOT: fprintf(stream, "call def_operator_not\n"); break;
        default    : assert(false && "default case in translate closed_unary_operator"); return false;
    }

    return true;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_func_call(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);
    assert($type   == FUNC_CALL);

    fprintf(stream, "\n"
                    "#OPERATOR func_call begin\n");

    int param_cnt = 0;
    if (!translate_func_param(ast_asm, L, stream, &param_cnt)) return false;
    fprintf(stream, "\n");

    for (int i = param_cnt - 1; i >= 0; --i) fprintf(stream, "pop [rex+%d]\n", i+$relative);

    add_rex($relative, stream);
    fprintf(stream, "call def_%d\n", $func_index);
    sub_rex($relative, stream);

    if (independent_op) fprintf(stream, "pop void\n");

    fprintf(stream, "#OPERATOR func_call end\n");
    return true;
}

bool translate_func_param(translator *const ast_asm, const AST_node *const node, FILE *const stream, int *const param_cnt)
{
    assert(ast_asm != nullptr);
    assert(stream  != nullptr);

    if (node == nullptr) return true;

    if ($type == FICTIONAL)
    {
        if (!translate_func_param(ast_asm, L, stream, param_cnt)) return false;
        if (!translate_func_param(ast_asm, R, stream, param_cnt)) return false;

        return true;
    }
    *param_cnt += 1;
    fprintf(stream, "#%d parameter\n", *param_cnt);

    if (!translate_distributor(ast_asm, node, stream, false)) return false;
    return true;
}
//---------------------------------------------------------------------------------------------------------------------------

bool translate_return(translator *const ast_asm, const AST_node *const node, FILE *const stream, const bool independent_op)
{
    assert(ast_asm != nullptr);
    assert(node    != nullptr);
    assert(stream  != nullptr);

    if (!independent_op)
    {
        fprintf_err("\"return\" must be independent operator\n");
        return false;
    }
    if (!translate_distributor(ast_asm, L, stream, false)) return false;
    if (!translate_distributor(ast_asm, R, stream, false)) return false;

    fprintf(stream, "ret\n");

    return true;
}
//---------------------------------------------------------------------------------------------------------------------------

bool fill_global_scope(global *const mem_glob, const AST_node *const node, int *const rex)
{
    assert(mem_glob != nullptr);
    assert(rex      != nullptr);

    if (node == nullptr) return true;

    if ($type == FICTIONAL)
    {
        if (!fill_global_scope(mem_glob, L, rex)) return false;
        if (!fill_global_scope(mem_glob, R, rex)) return false;

        return true;
    }
    if ($type == VAR_DECL)
    {
        if (mem_glob->size <= $var_index)
        {
            fprintf_err("backend: index of variable is more than size of variable store\n");
            return false;
        }
        mem_glob->ram[$var_index] = *rex;
        *rex += 1;
    }
    return true;
}

//===========================================================================================================================
// EXTRA
//===========================================================================================================================

void set_rex(const int rex_val, FILE *const stream)
{
    assert(stream != nullptr);

    fprintf(stream, "\n"
                    "#REX SET BEGIN\n"
                    "push %d\n"
                    "pop rex\n"
                    "#REX SET END\n", rex_val);
}

void add_rex(const int rex_add, FILE *const stream)
{
    assert(stream != nullptr);

    fprintf(stream, "\n"
                    "#REX ADD BEGIN\n"
                    "push rex\n"
                    "push %d\n"
                    "add\n"
                    "pop rex\n"
                    "#REX ADD END\n", rex_add);
}

void sub_rex(const int rex_sub, FILE *const stream)
{
    assert(stream != nullptr);

    fprintf(stream, "\n"
                    "#REX SUB BEGIN\n"
                    "push rex\n"
                    "push %d\n"
                    "sub\n"
                    "pop rex\n"
                    "#REX SUB END\n", rex_sub);
}
//---------------------------------------------------------------------------------------------------------------------------

bool translator_redefined_var(translator *const ast_asm, const int var_index)
{
    assert(ast_asm != nullptr);
    assert(!stack_empty(&$scope));

    if (stack_empty($loc.ram+var_index)) return false;

    const int cur_scope = *(int *) stack_front(&$scope           );
    const int var_scope = *(int *) stack_front($loc.ram+var_index);

    return var_scope >= cur_scope;
}

bool translator_undefined_var(translator *const ast_asm, const int var_index)
{
    assert(ast_asm != nullptr);

    if (!stack_empty($loc.ram+var_index)) return false;

    return $glob.ram[var_index] == -1;
}

void translator_new_scope(translator *const ast_asm)
{
    assert(ast_asm != nullptr);

    stack_push(&$scope, &$relative);
}

void translator_del_scope(translator *const ast_asm)
{
    assert(ast_asm != nullptr);
    assert(!stack_empty(&$scope));

    for (int i = 0; i < $loc.size; ++i)
    {
        if (translator_redefined_var(ast_asm, i)) stack_pop($loc.ram+i);
    }
    $relative = *(int *) stack_front(&$scope);
    stack_pop(&$scope);
}
//===========================================================================================================================
// PARSE
//===========================================================================================================================

int backend_parse(translator *const ast_asm,
                  AST_node  **const    tree, const char *buff, const int buff_size, int *const buff_pos)
{
    assert(ast_asm  != nullptr);
    assert(tree     != nullptr);

    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    int var_num = var_name_list_parse(buff, buff_size, buff_pos);
    if (var_num == -1) return -1;

    int main_num = func_name_list_parse(buff, buff_size, buff_pos);
    if (main_num == -1) return -1;
    
    *tree = AST_parse(buff, buff_size, buff_pos);
    if (tree == nullptr) return -1;

    translator_ctor(ast_asm, var_num);
    return main_num;
}

int var_name_list_parse(const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    int var_num = 0;
    if (!get_buff_int(buff, buff_size, buff_pos, &var_num))
    {
        fprintf_err("backend parse: expected number of variable names\n");
        return -1;
    }
    if (var_num < 0)
    {
        fprintf_err("backend parse: invalid number of variable names\n");
        return -1;
    }
    for (int i = 0; i < var_num; ++i)
    {
        if (!get_ast_word(buff, buff_size, buff_pos))
        {
            fprintf_err("backend parse: expected variable name\n");
            return -1;
        }
    }
    return var_num;
}

int func_name_list_parse(const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    int func_num = 0;
    if (!get_buff_int(buff, buff_size, buff_pos, &func_num))
    {
        fprintf_err("backend parse: expected number of functions\n");
        return -1;
    }
    if (func_num < 0)
    {
        fprintf_err("backend parse: invalid number of functions\n");
        return -1;
    }

    int main_num = -1;
    int main_len = (int) strlen(MAIN_FUNCTION);

    for (int i = 0; i < func_num; ++i)
    {
        const char *name_beg = nullptr;
        int         name_len = 0;

        if (!get_ast_word(buff, buff_size, buff_pos, &name_beg, &name_len))
        {
            fprintf_err("backend parse: expected function name\n");
            return -1;
        }
        if (name_len == main_len && !strncmp(MAIN_FUNCTION, name_beg, name_len)) main_num = i;
    }
    return main_num;
}

//===========================================================================================================================
// CTOR_DTOR
//===========================================================================================================================

void translator_ctor(translator *const ast_asm, const int var_num)
{
    assert(ast_asm != nullptr);

    global_ctor(&$glob , var_num);
    local_ctor (&$loc  , var_num);
    stack_ctor (&$scope, sizeof(int));

    $relative = 0;
}

void translator_dtor(translator *const ast_asm)
{
    assert(ast_asm != nullptr);

    global_dtor(&$glob );
    local_dtor (&$loc  );
    stack_dtor (&$scope);

    $relative = 0;
}
//---------------------------------------------------------------------------------------------------------------------------

void global_ctor(global *const mem, const int size)
{
    assert(mem != nullptr);

    mem->ram  = (int *) log_calloc((size_t) size, sizeof(int));
    mem->size = size;
}

void global_dtor(global *const mem)
{
    log_free(mem->ram);

    mem->ram  = nullptr;
    mem->size = 0;
}
//---------------------------------------------------------------------------------------------------------------------------

void local_ctor(local *const mem, const int size)
{
    assert(mem != nullptr);

    mem->ram  = (stack *) log_calloc((size_t) size, sizeof(stack));
    mem->size = size;

    for (int i = 0; i < size; ++i) stack_ctor(mem->ram+i, sizeof(int));
}

void local_dtor(local *const mem)
{
    assert(mem != nullptr);

    for (int i = 0; i < mem->size; ++i) stack_dtor(mem->ram+i);

    log_free(mem->ram);

    mem->ram  = nullptr;
    mem->size = 0;
}