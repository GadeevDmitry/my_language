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

#include "discoder.h"
#include "terminal_colors.h"

//===========================================================================================================================
// MAIN
//===========================================================================================================================

int main(const int argc, const char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "you hould give one parameter: code in AST format to convert in source\n");
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
        log_free((char *)buff);
        return 0;
    }

}

//===========================================================================================================================
// PARSE
//===========================================================================================================================

#define fprintf_err(message) fprintf(stderr, TERMINAL_RED "ERROR: " TERMINAL_CANCEL "%s", message)

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
    if (!get_int_buff(buff, buff_size, buff_pos, &var_num))
    {
        fprintf_err("expected number of variable names\n");
        parse_err_exit
    }
    if (var_num < 0)
    {
        fprintf_err("invalid number of variable names\n");
        parse_err_exit
    }
    if (!parse_variables(var_store, var_num, buff, buff_size, buff_pos)) { parse_err_exit }

    int func_num = 0;
    if (!get_int_buff(buff, buff_size, buff_pos, &func_num))
    {
        fprintf_err("expected number of function names\n");
        parse_err_exit
    }
    if (func_num < 0)
    {
        fprintf_err("invalid number of function names\n");
        parse_err_exit
    }
    if (!parse_functions(func_store, func_num, buff, buff_size, buff_pos)) { parse_err_exit }

    if ((*tree = AST_parse(buff, buff_size, buff_pos)) == nullptr) { parse_err_exit }

    return true;
}
#undef parse_err_exit

bool parse_variables(name_list *const var_store, const int var_num, const char *buff, const int buff_size, int *const buff_pos)
{
    assert(var_store != nullptr);
    assert(buff      != nullptr);
    assert(buff_pos  != nullptr);

    for (int i = 0; i < var_num; ++i)
    {
        const char *name_beg = nullptr;
        int         name_len = 0;

        if (get_ast_word(buff, buff_size, buff_pos, &name_beg, &name_len)) name_list_push(var_store, name_beg, name_len);
        else
        {
            fprintf_err("expected name of variable\n");
            return false;
        }
    }
    return true;
}

bool parse_functions(name_list *const func_store, const int func_num, const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff       != nullptr);
    assert(buff_pos   != nullptr);
    assert(func_store != nullptr);

    for (int i = 0; i < func_num; ++i)
    {
        const char *name_beg = nullptr;
        int         name_len = 0;

        if (get_ast_word(buff, buff_size, buff_pos, &name_beg, &name_len)) name_list_push(func_store, name_beg, name_len);
        else
        {
            fprintf_err("expected name of function\n");
            return false;
        }

        int arg_num = 0;
        if (!get_int_buff(buff, buff_size, buff_pos, &arg_num))
        {
            fprintf_err("expected number of function arguments\n");
            return false;
        }
        if (arg_num < 0)
        {
            fprintf_err("invalid number of arguments\n");
            return false;
        }
        if (!parse_arguments(arg_num, buff, buff_size, buff_pos)) return false;
    }
    return true;
}

bool parse_arguments(const int arg_num, const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    for (int i = 0; i < arg_num; ++i)
    {
        if (!get_ast_word(buff, buff_size, buff_pos))
        {
            fprintf_err("expected name of argument\n");
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