#ifndef FRONTEND
#define FRONTEND

#include "ast.h"
#include "../lib/stack/stack.h"

//===========================================================================================================================
// DSL
//===========================================================================================================================

//_____________________________________________________LEXICAL_ANALYSIS______________________________________________________

#define buff_data        code->buff.data
#define buff_size        code->buff.size
#define buff_line        code->buff.line
#define buff_pos         code->buff.pos

#define lexis_data       code->lexis.data
#define lexis_pos        code->lexis.pos
#define lexis_capacity   code->lexis.capacity

#define key_word_val     value.key_word
#define key_dbl_char_val value.key_dbl_char
#define key_char_val     value.key_char
#define int_num_val      value.int_num
#define token_len_val    value.token_len

#define $cur_token       lexis_data[*token_cnt]
#define $next_token      lexis_data[*token_cnt + 1]

#define $var_store      name_store->var_store
#define $func_store     name_store->func_store
#define $scope          name_store->scope

//===========================================================================================================================
// CONST
//===========================================================================================================================

//_____________________________________________________LEXICAL_ANALYSIS______________________________________________________

enum TOKEN_TYPE
{
    KEY_WORD        ,
    KEY_CHAR        ,
    KEY_CHAR_DOUBLE ,
    INT_NUM         ,
    UNDEF_TOKEN     ,
};
static const char *TOKEN_TYPE_NAMES[] =
{
    "KEY WORD"          ,
    "KEY CHAR"          ,
    "KEY CHAR DOUBLE"   ,
    "INT NUM"           ,
    "UNDEF"             ,
};

static const char *KEY_CHAR_NAMES     = ";," "(){}" "+-*/^" "!=><" "|&" "#";

static const char *KEY_WORD_NAMES[]   =
{
    "BARCELONA"         ,   // int
    "MESSI"             ,   // if
    "SUAREZ"            ,   // else
    "NEYMAR"            ,   // while
    "CHAMPIONS_LEAGUE"  ,   // return
    "CHECK_BEGIN"       ,   // input
    "CHECK_OVER"        ,   // output
};
enum KEY_WORD_TYPE
{
    INT     ,
    IF      ,
    ELSE    ,
    WHILE   ,
    RETURN  ,
    INPUT   ,
    OUTPUT  ,
};

static const char *KEY_CHAR_DOUBLE_NAMES[] = 
{
    "GOAL"              ,   // ==
    "NO_GOAL"           ,   // !=
    "GOAL_INSIDE"       ,   // >=
    "GOAL_OFFSIDE"      ,   // <=
    "GOAL_PLUS_ASSIST"  ,   // &&
    "GOAL_OR_ASSIST"    ,   // ||
};
enum KEY_CHAR_DOUBLE_TYPE
{
    EQUAL       ,
    NOT_EQUAL   ,
    ABOVE_EQUAL ,
    BELOW_EQUAL ,
    AND         ,
    OR          ,
};

static const char *LEXIS_GRAPHVIZ_HEADER = "digraph {\n"
                                         //"rankdir=LR\n"
                                           "splines=ortho\n"
                                           "node[shape=record, style=\"rounded, filled\", fontsize=8]\n";

//===========================================================================================================================
// STRUCT
//===========================================================================================================================

//_____________________________________________________LEXICAL_ANALYSIS______________________________________________________

struct token
{
    TOKEN_TYPE type;    // тип токена
    int token_beg;      // позиция в массиве с исходным текстом, где начинается токен
    int token_line;     // номер строки, где находится токен
    union
    {
        KEY_WORD_TYPE            key_word;  // .type = KEY_WORD
        KEY_CHAR_DOUBLE_TYPE key_dbl_char;  // .type = KEY_CHAR_DOUBLE
        char                     key_char;  // .type = KEY_CHAR
        int                       int_num;  // .type = INT_NUM
        int                     token_len;  // .type = UNDEF_TOKEN
    }
    value;
};

struct source
{
    struct
    {
        const char *data;   // содержимое исходника
        int         pos;    // текущая позиция в .data
        int         line;   // текущий номер мтроки в .data
        int         size;   // размер .data
    }
    buff;

    struct
    {
        token *data;        // массив токенов
        int    pos;         // индекс свободной ячейки для нового токена
        int    capacity;    // емкость массива токенов
    }
    lexis;
};

//____________________________________________________SYNTACTIC_ANALYSIS_____________________________________________________

struct var_info         // структура с информацией о переменной
{
    const char *name;   // имя переменной
    stack      scope;   // стек с номерами обласстей видимости, в которых обЪявлена переменная с именем .name
};

struct var_name_list    // список имен переменных
{
    var_info *var;      // массив структур с переменными
    int      size;      // размер массива
    int  capacity;      // емкость массива
};
//---------------------------------------------------------------------------------------------------------------------------

struct arg_list         // структура с аргументами функции
{
    int *name_index;    // массив индексов аргументов в списке имен переменных
    int        size;    // размер массива
    int    capacity;    // емкость массива
};

struct func_info        // структура с информацией о функции
{
    const char *name;   // имя функции
    arg_list    args;   // структура с аргументами данной функции
};

struct func_name_list   // список имен функций
{
    func_info *func;    // массив структур с функциями
    int        size;    // размер массива
    int    capacity;    // емкость массива
};
//---------------------------------------------------------------------------------------------------------------------------

struct dictionary       // структура, объединяющая списки имен аргументов и функций
{
    var_name_list   var_store;
    func_name_list func_store;

    int scope;
};

//____________________________________________________SYNTACTIC_ANALYSIS_____________________________________________________

//===========================================================================================================================
// VAR_NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void var_name_list_ctor (var_name_list *const var_store);
void var_name_list_dtor (var_name_list *const var_store);

void var_info_ctor (var_info *const var, const source *const code, const token *const cur_token, const int scope = -1);
void var_info_dtor (var_info *const var);

//===========================================================================================================================
// VAR_NAME_LIST USER
//===========================================================================================================================

// Добавляет область видимости scope в переменную cur_token (если переменная с данным именем не встречалась ранее, то она создается).
// Возвращает индекс имени в списке имен var_store
int  var_name_list_add_var       (var_name_list *const var_store, const source *const code, const token *const cur_token, const int scope);

// Удаляет область видимости scope из всех переменных
void var_name_list_clear_var     (var_name_list *const var_store,                                                         const int scope);

// Возвращает индекс переменной cur_token в спимке имен var_store, если переменная определена в любой области видимости, и -1 иначе
int  var_name_list_defined_var(var_name_list *const var_store, const source *const code, const token *const cur_token);

// Возвращает индекс переменной cur_token в списке имен var_store, если она объявлена в текущей области видимости scope, и -1 иначе
int  var_name_list_redefined_var (var_name_list *const var_store, const source *const code, const token *const cur_token, const int scope);

//===========================================================================================================================
// VAR_NAME_LIST CLOSED
//===========================================================================================================================

bool same_var              (const source *const code, var_info *const var, const token *const cur_token);

void var_info_push_scope   (var_info *const var, const int scope);
void var_info_pop_scope    (var_info *const var, const int scope);

int  var_name_list_new_var (var_name_list *const var_store, const source *const code, const token *const cur_token, const int scope);
void var_name_list_realloc (var_name_list *const var_store);

//===========================================================================================================================
// FUNC_NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void func_name_list_ctor (func_name_list *const func_store);
void func_name_list_dtor (func_name_list *const func_store);

void func_info_ctor (func_info *const func, const source *const code, const token *const cur_token);
void func_info_dtor (func_info *const func);

void arg_list_ctor (arg_list *const arg_store);
void arg_list_dtor (arg_list *const arg_store);

//===========================================================================================================================
// FUNC_NAME_LIST USER
//===========================================================================================================================

// Добавляет функцию cur_token в список имен func_store
// Возвращает индекс имени в списке имен
int func_name_list_add_func     (func_name_list *const func_store, const source *const code, const token *const cur_token);

//Возвращает индекс функции cur_token в списке имен func_store, если она объявлена, и -1 иначе
int func_name_list_defined_func (func_name_list *const func_store, const source *const code, const token *const cur_token);

//===========================================================================================================================
// FUNC_NAME_LIST CLOSED
//===========================================================================================================================

bool same_func              (const source *const code, func_info *const func, const token *const cur_token);
void func_name_list_realloc (func_name_list *const func_store);

//===========================================================================================================================
// DICTIONARY_CTOR_DTOR
//===========================================================================================================================

void dictionary_ctor (dictionary *const name_store);
void dictionary_dtor (dictionary *const name_store);

//===========================================================================================================================
// TRANSLATOR
//===========================================================================================================================

AST_node *parse_general(const source *const code);

bool parse_var_decl         (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         subtree);
bool parse_func_decl        (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         subtree);
bool parse_func_args        (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         arg_tree);

bool parse_operators        (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          op_tree);
bool parse_op_assignment    (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const   op_assign_tree);
bool parse_op_input         (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          in_tree);
bool parse_op_output        (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         out_tree);

bool parse_if               (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          if_tree);
bool parse_else             (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const        else_tree);
bool parse_while            (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const       while_tree);

bool parse_op_func_call     (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          subtree);
bool parse_func_call        (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          subtree);
bool parse_func_call_param  (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const       param_tree);

bool parse_op_return        (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          subtree);

bool parse_rvalue           (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const      rvalue_tree);
bool parse_assignment       (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const      assign_tree);
bool parse_op_or            (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          or_tree);
bool parse_op_and           (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         and_tree);
bool parse_op_equal         (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const       equal_tree);
bool parse_op_compare       (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         cmp_tree);
bool parse_op_add_sub       (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const     add_sub_tree);
bool parse_op_mul_div       (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const     mul_div_tree);
bool parse_op_pow           (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         pow_tree);
bool parse_op_not           (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const         not_tree);
bool parse_operand          (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          operand);
bool parse_rvalue_token     (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          subtree);
bool parse_lvalue           (dictionary *const name_store, const source *const code, int *const token_cnt, AST_node **const          subtree);

OPERATOR_TYPE token_to_ast_op_type(const token cur_token);

//===========================================================================================================================
// BOOL TOKEN_...
//===========================================================================================================================

bool token_cmp    (const token cur_token);
bool token_eq     (const token cur_token);
bool token_mul_div(const token cur_token);
bool token_add_sub(const token cur_token);
bool token_pow    (const token cur_token);

bool token_int    (const token cur_token);
bool token_if     (const token cur_token);
bool token_else   (const token cur_token);
bool token_while  (const token cur_token);
bool token_return (const token cur_token);
bool token_input  (const token cur_token);
bool token_output (const token cur_token);

bool token_e      (const token cur_token);
bool token_ne     (const token cur_token);
bool token_ae     (const token cur_token);
bool token_be     (const token cur_token);
bool token_and    (const token cur_token);
bool token_or     (const token cur_token);

bool token_undef  (const token cur_token);

bool token_char   (const token cur_token, const char cmp);

//_____________________________________________________LEXICAL_ANALYSIS______________________________________________________

//===========================================================================================================================
// SOURCE_CTOR_DTOR
//===========================================================================================================================

source *new_source  (const char *src_file);
bool    new_buff    (const char *src_file, source *const code);
void    new_lexis   (source *const code);
void    source_dtor (source *const code);

//===========================================================================================================================
// LEXICAL_ANALYSIS
//===========================================================================================================================

void lexical_analyzer    (source *const code);

int  get_another_token   (source *const code);
bool get_key_word_type   (source *const code, const int token_beg, const int token_len, KEY_WORD_TYPE        *const type = nullptr);
bool get_key_double_char (source *const code, const int token_beg, const int token_len, KEY_CHAR_DOUBLE_TYPE *const type = nullptr);
bool get_int_num         (source *const code, const int token_beg, const int token_len, int                  *const num  = nullptr);
bool comment             (source *const code);

bool key_char            (const char to_check);
bool split_char          (const char to_check);

//===========================================================================================================================
// TOKEN_CTOR_DTOR
//===========================================================================================================================

void create_key_word_token        (source *const code, const int token_beg, const int token_len);
void create_key_double_char_token (source *const code, const int token_beg, const int token_len);
void create_key_char_token        (source *const code);
void create_int_num_token         (source *const code, const int token_beg, const int token_len);
void create_undef_token           (source *const code, const int token_beg, const int token_len);

//===========================================================================================================================
// SKIP_SOURCE
//===========================================================================================================================

void skip_source_line   (source *const code);
void skip_source_spaces (source *const code);

//===========================================================================================================================
// DUMP
//===========================================================================================================================

void source_text_dump        (source *const code);

void lexis_graphviz_dump     (source *const code);
void do_lexis_graphviz_dump  (source *const code, FILE *const stream);

void graphviz_dump_edge      (const int token_num_from, const int token_num_to, FILE *const stream);

void graphviz_dump_token     (const token *const cur_token, FILE *const stream, const int token_num);
void graphviz_describe_token (const token *const cur_token, FILE *const stream, const GRAPHVIZ_COLOR     color,
                                                                                const GRAPHVIZ_COLOR fillcolor,
                                                                                const int token_num);
void get_token_value_message (const token *const cur_token, char *const token_value);

void system_graphviz_dump    (char *const dump_txt, char *const dump_png);

#endif //FRONTEND