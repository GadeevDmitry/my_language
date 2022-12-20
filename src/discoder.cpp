#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../lib/logs/log.h"
#include "../lib/read_write/read_write.h"
#include "../lib/algorithm/algorithm.h"

#include "discoder.h"
#include "terminal_colors.h"

//===========================================================================================================================
// MAIN
//===========================================================================================================================

int main(const int argc, const char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "you hould give two parameters: file with code in AST format to convert in source and name of source file\n");
        return 0;
    }
    int         buff_pos  = 0;
    int         buff_size = 0;
    const char *buff      = (const char *) read_file(argv[1], &buff_size);
    if         (buff      == nullptr)
    {
        fprintf(stderr, "can't open \"%s\"\n", argv[1]);
        return 0;
    }

    name_list var_store  = {}; name_list_ctor(&var_store);
    name_list func_store = {}; name_list_ctor(&func_store);
    AST_node       *tree = nullptr;

    if (!discoder_parse(&var_store, &func_store, &tree, buff, buff_size, &buff_pos))
    {
        log_free ((char *)buff);
        return 0;
    }
    log_free((char *)buff);

    //AST_tree_graphviz_dump(tree);

    FILE *source_stream = fopen(argv[2], "w");
    if   (source_stream == nullptr)
    {
        fprintf(stderr, "can't open \"%s\"\n", argv[2]);
        return 0;
    }

    if (!discoder_translate(tree, source_stream, &var_store, &func_store))
    {
        fprintf(stderr, TERMINAL_RED "translate failed\n" TERMINAL_CANCEL);
    }
    else fprintf(stderr, TERMINAL_GREEN "translate success\n" TERMINAL_CANCEL);

    name_list_dtor(&var_store);
    name_list_dtor(&func_store);
    AST_tree_dtor (tree);

    fclose(source_stream);
}

//===========================================================================================================================
// TRANSLATER
//===========================================================================================================================

#define fprintf_err(message) fprintf(stderr, TERMINAL_RED "ERROR: " TERMINAL_CANCEL "%s", message)

bool discoder_translate(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                        const name_list *const func_store, const int  tab_shift,
                                                                                                           const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);

    if (node == nullptr) return true;

    switch ($type)
    {
        case FICTIONAL: return translate_fictional(node, stream, var_store, func_store, tab_shift, independent_op);
        case NUMBER   : return translate_number   (node, stream, var_store, func_store, tab_shift, independent_op);
        case VARIABLE : return translate_variable (node, stream, var_store, func_store, tab_shift, independent_op);
        case OP_IF    : return translate_if       (node, stream, var_store, func_store, tab_shift, independent_op);
        case IF_ELSE  : fprintf_err("discoder translate: \"IF-ELSE\" node type with not \"IF\" node parent\n"); return false;
        case OP_WHILE : return translate_while    (node, stream, var_store, func_store, tab_shift, independent_op);
        case OPERATOR : return translate_operator (node, stream, var_store, func_store, tab_shift, independent_op);
        case VAR_DECL : return translate_var_decl (node, stream, var_store, func_store, tab_shift, independent_op);
        case FUNC_DECL: return translate_func_decl(node, stream, var_store, func_store, tab_shift, independent_op);
        case FUNC_CALL: return translate_func_call(node, stream, var_store, func_store, tab_shift, independent_op);
        case OP_RETURN: return translate_return   (node, stream, var_store, func_store, tab_shift, independent_op);
        default       : fprintf_err("discoder translate: undefined node type\n"); return false;
    }
    return false;
}

bool translate_fictional(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                         const name_list *const func_store, const int  tab_shift,
                                                                                                            const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == FICTIONAL);

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, independent_op)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, independent_op)) return false;

    return true;
}

bool translate_number(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                      const name_list *const func_store, const int  tab_shift,
                                                                                                         const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == NUMBER);

    if (independent_op)
    {
        fprintf_err("discoder translate: \"NUMBER\" node is independent operator\n");
        return false;
    }
    fprintf(stream, "%lf", $dbl_num);

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, independent_op)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, independent_op)) return false;

    return true;
}

bool translate_variable(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                        const name_list *const func_store, const int  tab_shift,
                                                                                                           const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == VARIABLE);

    if (independent_op)
    {
        fprintf_err("discoder translate: \"VARIABLE\" node is independent operator\n");
        return false;
    }
    if (var_store->size <= $var_index)
    {
        fprintf_err("discoder translate: index of variable is more than size of variable store\n");
        return false;
    }
    fprintf(stream, "%s", var_store->word[$var_index].name);

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, independent_op)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, independent_op)) return false;

    return true;
}

bool translate_if(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                  const name_list *const func_store, const int  tab_shift,
                                                                                                     const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OP_IF);

    if (!independent_op)
    {
        fprintf_err("discoder translate: \"IF\" node is not independent operator\n");
        return false;
    }
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "MESSI (");

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;

    if (R->type != IF_ELSE)
    {
        fprintf_err("discoder translate: right subtree of IF node is not IF_ELSE node\n");
        return false;
    }

    fprintf    (stream,     ")\n");
    fprintf_tab(stream, tab_shift);
    fprintf    (stream,     "{\n");

    if (!discoder_translate(R->left, stream, var_store, func_store, tab_shift + 1, true)) return false;

    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "}\n");

    if (R->right != nullptr)
    {
        fprintf_tab(stream, tab_shift);
        fprintf    (stream, "SUAREZ\n");
        fprintf_tab(stream, tab_shift);
        fprintf    (stream, "{\n");

        if (!discoder_translate(R->right, stream, var_store, func_store, tab_shift + 1, true)) return false;

        fprintf_tab(stream, tab_shift);
        fprintf    (stream, "}\n");
    }
    return true;
}

bool translate_while(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                     const name_list *const func_store, const int  tab_shift,
                                                                                                        const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OP_WHILE);

    if (!independent_op)
    {
        fprintf_err("discoder translate: \"WHILE\" node is not independent operator\n");
        return false;
    }
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "NEYMAR(");

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;

    fprintf    (stream, ")\n");
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "{\n");

    if (!discoder_translate(R, stream, var_store, func_store, tab_shift + 1, true)) return false;

    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "}\n");

    return true;
}

bool translate_operator(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                        const name_list *const func_store, const int  tab_shift,
                                                                                                           const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OPERATOR);

    switch ($op_type)
    {
        case OP_ADD        :
        case OP_SUB        :
        case OP_MUL        :
        case OP_DIV        :
        case OP_POW        :

        case OP_EQUAL      :
        case OP_ABOVE      :
        case OP_BELOW      :
        case OP_ABOVE_EQUAL:
        case OP_BELOW_EQUAL:
        case OP_NOT_EQUAL  :

        case OP_OR         :
        case OP_AND        : return translate_closed_binary_operator(node, stream, var_store, func_store, tab_shift, independent_op);

        case OP_INPUT      :
        case OP_OUTPUT     : return translate_opened_unary_operator (node, stream, var_store, func_store, tab_shift, independent_op);

        case OP_NOT        :
        case OP_SIN        :
        case OP_COS        :
        case OP_DIFF       :
        case OP_LOG        :
        case OP_SQRT       : return translate_closed_unary_operator (node, stream, var_store, func_store, tab_shift, independent_op);

        case ASSIGNMENT    : return  translate_assignment           (node, stream, var_store, func_store, tab_shift, independent_op);

        default            : break;
    }

    return false;
}

bool translate_assignment(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                          const name_list *const func_store, const int  tab_shift,
                                                                                                             const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OPERATOR);
    assert($op_type   == ASSIGNMENT);

    if (independent_op) fprintf_tab(stream, tab_shift);

        if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;
        fprintf(stream, " = ");
        if (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;

        if (independent_op) fprintf(stream, ";\n");

        return true;
}

bool translate_opened_unary_operator(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                     const name_list *const func_store, const int  tab_shift,
                                                                                                                        const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OPERATOR);

    if (!independent_op)
    {
        fprintf(stderr, "discoder translate: \"%s\" must be independent operator\n", AST_OPERATOR_TYPE_NAMES[$op_type]);
        return false;
    }
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "%s ", AST_OPERATOR_TYPE_NAMES[$op_type]);

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;

    fprintf(stream, ";\n");
    return true;
}

bool translate_closed_unary_operator(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                     const name_list *const func_store, const int  tab_shift,
                                                                                                                        const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OPERATOR);

    if (independent_op)
    {
        fprintf(stderr, "discoder translate: \"%s\" can't be independent operator\n", AST_OPERATOR_TYPE_NAMES[$op_type]);
        return false;
    }
    fprintf(stream, "%s", AST_OPERATOR_TYPE_NAMES[$op_type]);

    if ($op_type != OP_NOT) fprintf(stream, "(");

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;

    if ($op_type != OP_NOT) fprintf(stream, ")");

    return true;
}

bool translate_closed_binary_operator(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                      const name_list *const func_store, const int tab_shift,
                                                                                                                         const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OPERATOR);

    if (independent_op)
    {
        fprintf(stderr, "discoder translate: \"%s\" can't be independent operator\n", AST_OPERATOR_TYPE_NAMES[$op_type]);
        return false;
    }

    if (L != nullptr && L->type == OPERATOR && OP_PRIORITY[L->value.op_type] < OP_PRIORITY[$op_type])
    {
        fprintf(stream, "(");
        if  (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;
        fprintf(stream, ")");
    }
    else if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;

    fprintf(stream, " %s ", AST_OPERATOR_TYPE_NAMES[$op_type]);

    if (R != nullptr && R->type == OPERATOR && OP_PRIORITY[R->value.op_type] < OP_PRIORITY[$op_type])
    {
        fprintf(stream, "(");
        if  (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;
        fprintf(stream, ")");
    }
    else if (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;

    return true;

}
bool translate_var_decl(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                        const name_list *const func_store, const int  tab_shift,
                                                                                                           const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == VAR_DECL);

    if (!independent_op)
    {
        fprintf_err("discoder translate: \"VAR_DECL\" is not independent operator\n");
        return false;
    }
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "BARCELONA ");

    if (var_store->size <= $var_index)
    {
        fprintf_err("discoder translate: index of variable is more than size of variable store\n");
        return false;
    }
    fprintf(stream, "%s", var_store->word[$var_index].name);

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;

    fprintf(stream, ";\n");
    return true;
}

bool translate_func_decl(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                         const name_list *const func_store, const int  tab_shift,
                                                                                                            const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == FUNC_DECL);

    if (!independent_op)
    {
        fprintf_err("discoder translate: \"FUNC_DECL\" is not independent operator\n");
    }
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "BARCELONA ");

    if ($func_index >= func_store->size)
    {
        fprintf_err("discoder translate: index of function is more than size of function store\n");
        return false;
    }
    fprintf(stream, "%s(", func_store->word[$func_index].name);

    int arg_cnt = 0;
    if (!translate_func_args_params(L, stream, var_store, func_store, tab_shift, &arg_cnt)) return false;

    fprintf    (stream, ")\n");
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "{\n");

    if (!discoder_translate(R, stream, var_store, func_store, tab_shift + 1, true)) return false;

    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "}\n");

    return true;
}

bool translate_func_call(const AST_node *const node, FILE *const stream, const name_list *const  var_store,
                                                                         const name_list *const func_store, const int  tab_shift,
                                                                                                            const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == FUNC_CALL);

    if (independent_op) fprintf_tab(stream, tab_shift);
    if (func_store->size <= $func_index)
    {
        fprintf_err("discoder translate: index of function is more than size of function store\n");
        return false;
    }
    fprintf(stream, "%s(", func_store->word[$func_index].name);

    int param_cnt = 0;
    if (!translate_func_args_params(L, stream, var_store, func_store, tab_shift, &param_cnt)) return false;
    if (!translate_func_args_params(R, stream, var_store, func_store, tab_shift, &param_cnt)) return false;

    fprintf(stream, ")");
    if (independent_op) fprintf(stream, ";\n");

    return true;
}

bool translate_func_args_params(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                                const name_list *const func_store, const int  tab_shift,
                                                                                                                   int *const arg_param_cnt)
{
    assert(stream        != nullptr);
    assert(var_store     != nullptr);
    assert(func_store    != nullptr);
    assert(arg_param_cnt != nullptr);

    if (node == nullptr) return true;

    if ($type == FICTIONAL)
    {
        if (!translate_func_args_params(L, stream, var_store, func_store, tab_shift, arg_param_cnt)) return false;
        if (!translate_func_args_params(R, stream, var_store, func_store, tab_shift, arg_param_cnt)) return false;

        return true;
    }
    if (*arg_param_cnt != 0) fprintf(stream, ", ");

    if (!discoder_translate(node, stream, var_store, func_store, tab_shift, false)) return false;
    *arg_param_cnt += 1;
    return true;
}

bool translate_return(const AST_node *const node, FILE *const stream, const name_list *const var_store,
                                                                      const name_list *const func_store, const int  tab_shift,
                                                                                                         const bool independent_op)
{
    assert(stream     != nullptr);
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(node       != nullptr);
    assert($type      == OP_RETURN);

    if (!independent_op)
    {
        fprintf_err("discoder translate: \"RETURN\" node is not independent operator\n");
        return false;
    }
    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "CHAMPIONS_LEAGUE ");

    if (!discoder_translate(L, stream, var_store, func_store, tab_shift, false)) return false;
    if (!discoder_translate(R, stream, var_store, func_store, tab_shift, false)) return false;

    fprintf(stream, ";\n");
    return true;
}

//===========================================================================================================================
// PARSE
//===========================================================================================================================

#define parse_err_exit                                                                                                      \
        AST_tree_dtor (*tree);                                                                                              \
        name_list_dtor(var_store);                                                                                          \
        name_list_dtor(func_store);                                                                                         \
        return false;

bool discoder_parse(name_list *const  var_store,
                    name_list *const func_store,
                    AST_node **const       tree, const char *buff, const int buff_size, int *const buff_pos)
{
    assert(var_store  != nullptr);
    assert(func_store != nullptr);
    assert(tree       != nullptr);
    assert(buff       != nullptr);
    assert(buff_pos   != nullptr);

    int var_num = 0;
    if (!get_buff_int(buff, buff_size, buff_pos, &var_num))
    {
        fprintf_err("discoder_parse: expected number of variable names\n");
        parse_err_exit
    }
    if (var_num < 0)
    {
        fprintf_err("discoder_parse: invalid number of variable names\n");
        parse_err_exit
    }
    if (!parse_name_list(var_store, var_num, buff, buff_size, buff_pos)) { parse_err_exit }

    int func_num = 0;
    if (!get_buff_int(buff, buff_size, buff_pos, &func_num))
    {
        fprintf_err("discoder_parse: expected number of function names\n");
        parse_err_exit
    }
    if (func_num < 0)
    {
        fprintf_err("discoder_parse: invalid number of function names\n");
        parse_err_exit
    }
    if (!parse_name_list(func_store, func_num, buff, buff_size, buff_pos)) { parse_err_exit }

    if ((*tree = AST_parse(buff, buff_size, buff_pos)) == nullptr) { parse_err_exit }

    return true;
}
#undef parse_err_exit

bool parse_name_list(name_list *const name_store, const int name_num, const char *buff, const int buff_size, int *const buff_pos)
{
    assert(name_store != nullptr);
    assert(buff       != nullptr);
    assert(buff_pos   != nullptr);

    for (int i = 0; i < name_num; ++i)
    {
        const char *name_beg = nullptr;
        int         name_len = 0;

        if (get_ast_word(buff, buff_size, buff_pos, &name_beg, &name_len)) name_list_push(name_store, name_beg, name_len);
        else
        {
            fprintf_err("discoder_parse: expected name\n");
            return false;
        }
    }
    return true;
}
#undef fprintf_err

//===========================================================================================================================
// NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void name_list_ctor(name_list *const name_store)
{
    assert(name_store != nullptr);

    name_store->size     = 0;
    name_store->capacity = 4; //default capacity
    name_store->word     = (name_info *) log_calloc(4, sizeof(name_info));
}

void name_list_dtor(name_list *const name_store)
{
    assert(name_store != nullptr);

    for (int i = 0; i < name_store->size; ++i) name_info_dtor(name_store->word+i);
    log_free(name_store->word);
}

void name_info_ctor(name_info *const word, const char *name_beg, const int name_len)
{
    assert(word != nullptr);

    word->name     = strndup(name_beg, (size_t) name_len);
    word->name_len = name_len;
}

void name_info_dtor(name_info *const word)
{
    assert(word != nullptr);

    free((char *)word->name);

    word->name     = nullptr;
    word->name_len = 0;
}

//===========================================================================================================================
// NAME_LIST USER
//===========================================================================================================================

void name_list_push(name_list *const name_store, const char *name_beg, const int name_len)
{
    assert(name_store != nullptr);
    assert(name_beg   != nullptr);

    name_list_realloc(name_store);
    name_info_ctor   (name_store->word+name_store->size, name_beg, name_len);

    name_store->size++;
}

//===========================================================================================================================
// NAME_LIST CLOSED
//===========================================================================================================================

void name_list_realloc(name_list *const name_store)
{
    assert(name_store != nullptr);

    if (name_store->size == name_store->capacity)
    {
        name_store->capacity *= 2;
        name_store->word      = (name_info *) log_realloc(name_store->word, (size_t) name_store->capacity * sizeof(name_info));
    }
}