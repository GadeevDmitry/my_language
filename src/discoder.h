#ifndef DISCODER
#define DISCODER

#include "ast.h"

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
// PARSE
//===========================================================================================================================

bool discoder_parse  (name_list *const  var_store,
                      name_list *const func_store,
                      AST_node **const       tree, const char *buff, const int buff_size, int *const buff_pos);
bool parse_variables (name_list *const  var_store,
                      const        int    var_num, const char *buff, const int buff_size, int *const buff_pos);
bool parse_functions (name_list *const func_store,
                      const        int   func_num, const char *buff, const int buff_size, int *const buff_pos);
bool parse_arguments (const        int    arg_num, const char *buff, const int buff_size, int *const buff_pos);

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