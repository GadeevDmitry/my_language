#ifndef AST
#define AST

//===========================================================================================================================
// DSL
//===========================================================================================================================

#define $type       node->type

#define $fictional  node->value.fictional
#define $dbl_num    node->value.dbl_num
#define $var_index  node->value.var_index
#define $func_index node->value.func_index
#define $op_type    node->value.op_type

#define L node->left
#define R node->right
#define P node->prev

//===========================================================================================================================
// CONST
//===========================================================================================================================

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
    FUNC_DECL   ,
    FUNC_CALL   ,
    OP_RETURN   ,
};

enum OPERATOR_TYPE
{
    OP_ADD = 1      ,
    OP_SUB          ,
    OP_MUL          ,
    OP_DIV          ,
    OP_SQRT         ,

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
    OP_POW          ,
    OP_SIN          ,
    OP_COS          ,
    OP_DIFF         ,
    OP_LOG          ,
};

static const char *OPERATOR_NAMES[] =
{
    "nothing"       ,
    "add"           ,
    "sub"           ,
    "mul"           ,
    "div"           ,
    "sqrt"          ,
    "input"         ,
    "output"        ,
    "equal"         ,
    "above"         ,
    "below"         ,
    "above_equal"   ,
    "below_equal"   ,
    "not_equal"     ,
    "not"           ,
    "or"            ,
    "and"           ,
    "assignment"    ,
    "pow"           ,
    "sin"           ,
    "cos"           ,
    "diff"          ,
    "ln"            ,
};

static const char *MAIN_FUNCTION = "CAMP_NOU";

//===========================================================================================================================
// STRUCT
//===========================================================================================================================

struct AST_node
{
    AST_NODE_TYPE type;
    union
    {
        int         fictional;  // фиктивное значение                               .type = FICTIONAL, OP_IF, IF_ELSE, OP_WHILE, OP_RETURN
        double        dbl_num;  // значение числа                                   .type = NUMBER
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

#define AST_NEW_NODE_DECLARATION(ast_node_type, value_field_type)                                                           \
                                                                                                                            \
AST_node *new_##ast_node_type##_AST_node(const value_field_type value, AST_node *const left  = nullptr,                     \
                                                                       AST_node *const right = nullptr,                     \
                                                                       AST_node *const prev  = nullptr);

AST_NODE_CTOR_DECLARATION(FICTIONAL,    int);
AST_NODE_CTOR_DECLARATION(NUMBER   , double);
AST_NODE_CTOR_DECLARATION(VARIABLE ,    int);
AST_NODE_CTOR_DECLARATION(OP_IF    ,    int);
AST_NODE_CTOR_DECLARATION(IF_ELSE  ,    int);
AST_NODE_CTOR_DECLARATION(OP_WHILE ,    int);
AST_NODE_CTOR_DECLARATION(OPERATOR , OPERATOR_TYPE);
AST_NODE_CTOR_DECLARATION(VAR_DECL ,    int);
AST_NODE_CTOR_DECLARATION(FUNC_CALL,    int);
AST_NODE_CTOR_DECLARATION(FUNC_DECL,    int);
AST_NODE_CTOR_DECLARATION(OP_RETURN,    int);

AST_NEW_NODE_DECLARATION(FICTIONAL,     int);
AST_NEW_NODE_DECLARATION(NUMBER   ,  double);
AST_NEW_NODE_DECLARATION(VARIABLE ,     int);
AST_NEW_NODE_DECLARATION(OP_IF    ,     int);
AST_NEW_NODE_DECLARATION(IF_ELSE  ,     int);
AST_NEW_NODE_DECLARATION(OP_WHILE ,     int);
AST_NEW_NODE_DECLARATION(OPERATOR , OPERATOR_TYPE);
AST_NEW_NODE_DECLARATION(VAR_DECL ,     int);
AST_NEW_NODE_DECLARATION(FUNC_CALL,     int);
AST_NEW_NODE_DECLARATION(FUNC_DECL,     int);
AST_NEW_NODE_DECLARATION(OP_RETURN,     int);

#undef AST_NODE_DECLARATION
#undef AST_NEW_NODE_DECLARATION

void AST_node_dtor (AST_node *const node);
void AST_tree_dtor (AST_node *const node);

//===========================================================================================================================
// PARSE_CONVERT
//===========================================================================================================================

AST_node *AST_parse   (const char *buff, const int buff_size, int *const buff_pos);
void      AST_convert (const AST_node *const node, FILE *const stream, const int tab_shift = 0);

bool get_buff_int     (const char *buff, const int buff_size, int *const buff_pos, int *const    int_num);
bool get_buff_dbl     (const char *buff, const int buff_size, int *const buff_pos, double *const dbl_num);
bool get_ast_word     (const char *buff, const int buff_size, int *const buff_pos, const char **name_beg = nullptr,
                                                                                   int *const   name_len = nullptr);
void skip_ast_spaces  (const char *buff, const int buff_size, int *const buff_pos);
void fprintf_tab      (FILE *const stream, const int tab_shift);

//===========================================================================================================================
// DUMP
//===========================================================================================================================

void AST_tree_graphviz_dump(const AST_node *const root);

#endif //AST