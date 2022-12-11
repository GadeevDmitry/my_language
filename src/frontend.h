#ifndef FRONTEND
#define FRONTEND

//===========================================================================================================================
// DSL
//===========================================================================================================================

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
    "BARCELONA"         ,
    "MESSI"             ,
    "SUAREZ"            ,
    "NEYMAR"            ,
    "CHAMPIONS_LEAGUE"  ,
    "ARGENTINA"         ,
    "PORTUGAL"          ,
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
    "==", "!=", ">=", "<=", "&&", "||",
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

const char *LEXIS_GRAPHVIZ_HEADER = "digraph {\n"
                                    //"rankdir=LR\n"
                                    "splines=ortho\n"
                                    "node[shape=record, style=\"rounded, filled\", fontsize=8]\n";

//____________________________________________________________AST____________________________________________________________

enum AST_NODE_TYPE
{
    FICTIONAL   ,
    NUMBER      ,
    VARIABLE    ,
    OP_IF       ,
    IF_ELSE     ,
    OP_WHILE    ,
    OPERATOR    ,
    VAR_DECL    ,
    FUNC_CALL   ,
    FUNC_DECL   ,
    OP_RETURN   ,
};

enum OPERATOR_TYPE
{
    OP_ADD = 1      ,
    OP_SUB          ,
    OP_MUL          ,
    OP_DIV          ,
    OP_POW          ,

    OP_INPUT        ,
    OP_OUTPUT       ,

    OP_EQUAL        ,
    OP_ABOVE        ,
    OP_BELOW        ,
    OP_ABOVE_EQUAL  ,
    OP_BELOW_EQUAL  ,
    OP_NOT_EQUAL    ,
    OP_NOT          ,

    OP_OR           ,
    OP_AND          ,

    ASSIGNMENT      ,
};

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

//____________________________________________________________AST____________________________________________________________

struct AST_node
{
    AST_NODE_TYPE type;
    union
    {
        int         fictional;  // фиктивное значение                               .type = FICTIONAL, OP_IF, IF_ELSE, OP_WHILE, OP_RETURN
        int           int_num;  // значение числа                                   .type = NUMBER
        int         var_index;  // индекс имени переменной в списке имен переменных .type = VARIABLE, VAR_DECL
        int        func_index;  // индекс имени функции    в списке имен функций    .type = FUNC_CALL, FUNC_DECL
        OPERATOR_TYPE op_type;  // тип оператора                                    .type = OPERATOR
    }
    value;

    AST_node *left;             // указатель на левого сына
    AST_node *right;            // указатель на правого сына
    AST_node *prev;             // указатель на родителя
};

//===========================================================================================================================
// AST_NODE_CTOR_DTOR
//===========================================================================================================================

#define AST_NODE_CTOR_DECLARATION(ast_node_type, value_field_type)                                                          \
                                                                                                                            \
void AST_node_##ast_node_type##_ctor(AST_node *const node, const value_field_type value, AST_node *const left  = nullptr,   \
                                                                                         AST_node *const right = nullptr,   \
                                                                                         AST_node *const prev  = nullptr);

AST_NODE_CTOR_DECLARATION(FICTIONAL, int);
AST_NODE_CTOR_DECLARATION(NUMBER   , int);
AST_NODE_CTOR_DECLARATION(VARIABLE , int);
AST_NODE_CTOR_DECLARATION(OP_IF    , int);
AST_NODE_CTOR_DECLARATION(IF_ELSE  , int);
AST_NODE_CTOR_DECLARATION(OP_WHILE , int);
AST_NODE_CTOR_DECLARATION(OPERATOR , OPERATOR_TYPE);
AST_NODE_CTOR_DECLARATION(VAR_DECL , int);
AST_NODE_CTOR_DECLARATION(FUNC_CALL, int);
AST_NODE_CTOR_DECLARATION(FUNC_DECL, int);
AST_NODE_CTOR_DECLARATION(OP_RETURN, int);

#undef AST_NODE_DECLARATION

void AST_node_dtor (AST_node *const node);
void AST_tree_dtor (AST_node *const node);

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

void lexical_analyzer  (source *const code);

int  get_another_token (source *const code);
bool get_key_word_type (source *const code, const int token_beg, const int token_len, KEY_WORD_TYPE        *const type = nullptr);
bool key_double_char   (source *const code,                                           KEY_CHAR_DOUBLE_TYPE *const type = nullptr);
bool get_int_num       (source *const code, const int token_beg, const int token_len, int           *const num  = nullptr);
bool comment           (source *const code);

bool key_char          (const char to_check);
bool split_char        (const char to_check);

//===========================================================================================================================
// TOKEN_CTOR_DTOR
//===========================================================================================================================

void create_key_word_token        (source *const code, const int token_beg, const int token_len);
void create_key_double_char_token (source *const code);
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