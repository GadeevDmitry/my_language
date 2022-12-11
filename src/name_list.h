#ifndef NAME_LIST
#define NAME_LIST

#include "../lib/stack/stack.h"

//===========================================================================================================================
// DSL
//===========================================================================================================================

#define address(i) var[i].address
#define    name(i) var[i].name

//===========================================================================================================================
// STRUCT
//===========================================================================================================================

//____________________________________________________________VAR____________________________________________________________

struct var_name         // структура, содержащая информацию о переменной
{
    const char *name;   // имя переменной
    stack    address;   // стек адресов переменной
};

struct var_name_list    // список переменных
{
    var_name *var;      // массив структур с переменными
    int      size;      // размер .var
    int  capacity;      // емкость .var
};

//_________________________________________________________FUNCTION__________________________________________________________

struct arg_list         // структура, содержащая имена переменных
{
    const char **name;  // массив имён аргументов
    int          size;  // размер .name
    int      capacity;  // емкость .name
};

struct func_name        // структура, содержащая информацию о функции
{
    const char *name;   // имя функции
    int      arg_num;   // количество аргументов функции
    arg_list arg;       // структура, с именами аргументов
};

struct func_name_list   // список функций
{
    func_name *func;    // массив структур с функциями
    int        size;    // размер .func
    int    capacity;    // емкость .func
};

//____________________________________________________________VAR____________________________________________________________

//===========================================================================================================================
// CTOR_DTOR
//===========================================================================================================================

void        var_name_list_ctor (var_name_list *const var_store);
void        var_name_list_dtor (var_name_list *const var_store);

static void var_name_ctor      (var_name *const ctored_var_name, const char *name_beg, const int name_len, const int new_address);
static void var_name_dtor      (var_name *const dtored_var_name);

//===========================================================================================================================
// EXTRA USER FUNCTION
//===========================================================================================================================

int  var_name_list_new_address (      var_name_list *const var_store, const char *name_beg, const int name_len, const int new_address);
bool defined_var_name          (const var_name_list *const var_store, const char *name_beg, const int name_len);
bool redefined_var_name        (const var_name_list *const var_store, const char *name_beg, const int name_len, const int scope_beg_address);
void remove_local_var          (const var_name_list *const var_store, const int scope_beg_address);

//===========================================================================================================================
// EXTRA STATIC FUNCTION
//===========================================================================================================================

static bool same_name              (const var_name *const cur_var_name, const char *name_beg, const int name_len);
static void var_name_new_address   (      var_name *const cur_var_name, const int new_address);
static void var_name_list_realloc  (var_name_list *const var_store);
static int  var_name_list_new_name (var_name_list *const var_store, const char *name_beg, const int name_len, const int new_address);

//_________________________________________________________FUNCTION__________________________________________________________

//===========================================================================================================================
// CTOR_DTOR
//===========================================================================================================================

void func_name_list_ctor (func_name_list *const func_store);
void func_name_list_dtor (func_name_list *const func_store);

void func_name_dtor      (func_name *const dtored_func_name);
void arg_list_dtor       (arg_list  *const dtored_arg_list);

#endif //NAME_LIST