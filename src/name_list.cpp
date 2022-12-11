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

#include "name_list.h"

//____________________________________________________________VAR____________________________________________________________

//===========================================================================================================================
// CTOR_DTOR
//===========================================================================================================================

void var_name_list_ctor(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    var_store->size     = 0;
    var_store->capacity = 4; //default capacity
    var_store->var      = (var_name *) log_calloc(4, sizeof(var_name));
}

void var_name_list_dtor(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    for (int i = 0; i < var_store->size; ++i) var_name_dtor(var_store->var+i);

    log_free(var_store->var);

    var_store->var      = nullptr;
    var_store->size     = 0;
    var_store->capacity = 0;
}

static void var_name_ctor(var_name *const ctored_var_name, const char *name_beg, const int name_len, const int new_address)
{
    assert(ctored_var_name != nullptr);
    assert(name_beg        != nullptr);

    ctored_var_name->name = (const char *) strndup(name_beg, (size_t) name_len);

    stack_ctor(&ctored_var_name->address, sizeof(int));
    stack_push(&ctored_var_name->address, &new_address);
}

static void var_name_dtor(var_name *const dtored_var_name)
{
    assert(dtored_var_name != nullptr);

    log_free  ((char *) dtored_var_name->name);
    stack_dtor(        &dtored_var_name->address);
}

//===========================================================================================================================
// EXTRA USER FUNCTION
//===========================================================================================================================

//добавляет имя и адрес переменной в список имен, возвращает индекс имени
int var_name_list_new_address(var_name_list *const var_store, const char *name_beg, const int name_len, const int new_address)
{
    assert(var_store != nullptr);
    assert(name_beg  != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (same_name(var_store->var+i, name_beg, name_len))
        {
            var_name_new_address(var_store->var+i, new_address);
            return i;
        }
    }
    return var_name_list_new_name(var_store, name_beg, name_len, new_address);
}

//проверяет имя переменной на существование в любой области видимости (необходимо для выявления необъявленной переменной)
bool defined_var_name(const var_name_list *const var_store, const char *name_beg, const int name_len)
{
    assert(var_store != nullptr);
    assert(name_beg  != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (same_name(var_store->var+i, name_beg, name_len) && !stack_empty(&var_store->address(i))) return true;
    }
    return false;
}

//проверяет имя переменной на существование в текущей области видимости (необходимо для выявления переопределённой переменной)
bool redefined_var_name(const var_name_list *const var_store, const char *name_beg, const int name_len, const int scope_beg_address)
{
    assert(var_store != nullptr);
    assert(name_beg  != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (same_name(var_store->var+i, name_beg, name_len))
        {
            if (       !stack_empty(&var_store->address(i)) &&
                *(int *)stack_front(&var_store->address(i)) >= scope_beg_address) return true;
        }   
    }
    return false;
}

//удаляет переменные из текущей области видимости
void remove_local_var(const var_name_list *const var_store, const int scope_beg_address)
{
    assert(var_store != nullptr);

    for (int i = 0; i < var_store->size; ++i)
    {
        if (       !stack_empty(&var_store->address(i)) &&
            *(int *)stack_front(&var_store->address(i)) >= scope_beg_address) stack_pop(&var_store->address(i));
    }
}

//===========================================================================================================================
// EXTRA STATIC FUNCTION
//===========================================================================================================================

static bool same_name(const var_name *const cur_var_name, const char *name_beg, const int name_len)
{
    assert(cur_var_name != nullptr);
    assert(name_beg     != nullptr);

    return cur_var_name->name[name_len] == '\0' && !strncmp(cur_var_name->name, name_beg, (size_t) name_len);
}

static void var_name_new_address(var_name *const cur_var_name, const int new_address)
{
    assert(cur_var_name != nullptr);

    stack_push(&cur_var_name->address, &new_address);
}

static void var_name_list_realloc(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    if (var_store->size == var_store->capacity)
    {
        var_store->capacity *= 2;
        var_store->var       = (var_name *) log_realloc(var_store->var, (size_t) var_store->capacity * sizeof(var_name));
    }
}

//возвращает индекс имени в списке имен
static int var_name_list_new_name(var_name_list *const var_store, const char *name_beg, const int name_len, const int new_address)
{
    assert(var_store != nullptr);
    assert(name_beg  != nullptr);

    var_name_list_realloc(var_store);
    var_name_ctor        (var_store->var+var_store->size, name_beg, name_len, new_address);

    return var_store->size++;
}

//_________________________________________________________FUNCTION__________________________________________________________

//===========================================================================================================================
// CTOR_DTOR
//===========================================================================================================================

void func_name_list_ctor(func_name_list *const func_store)
{
    assert(func_store != nullptr);

    func_store->size     = 0;
    func_store->capacity = 4; //default capacity
    func_store->func     = (func_name *) log_calloc(4, sizeof(func_name));
}

void func_name_list_dtor(func_name_list *const func_store)
{
    assert(func_store != nullptr);

    for (int i = 0; i < func_store->size; ++i) func_name_dtor(func_store->func+i);

    log_free(func_store->func);

    func_store->func     = nullptr;
    func_store->size     = 0;
    func_store->capacity = 0;
}

void func_name_dtor(func_name *const dtored_func_name)
{
    assert(dtored_func_name != nullptr);

    dtored_func_name->arg_num = 0;

    log_free     ((char *) dtored_func_name->name);
    arg_list_dtor(        &dtored_func_name->arg);
}

void arg_list_dtor(arg_list *const dtored_arg_list)
{
    assert(dtored_arg_list != nullptr);

    for (int i = 0; i < dtored_arg_list->size; ++i) log_free((char *) dtored_arg_list->name[i]);
    
    dtored_arg_list->capacity = 0;
    log_free(dtored_arg_list->name);
}