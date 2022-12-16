#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../lib/logs/log.h"
#include "../lib/read_write/read_write.h"
#include "../lib/algorithm/algorithm.h"
#include "../lib/stack/stack.h"
#include "../lib/graphviz_dump/graphviz_dump.h"

#include "ast.h"
#include "terminal_colors.h"

//===========================================================================================================================
// STATIC CONST
//===========================================================================================================================

static const char *AST_NODE_TYPE_NAMES[] =
{
    "FICTIONAL" ,
    "NUMBER"    ,
    "VARIABLE"  ,
    "OP_IF"     ,
    "IF_ELSE"   ,
    "OP_WHILE"  ,
    "OPERATOR"  ,
    "VAR_DECL"  ,
    "FUNC_DECL" ,
    "FUNC_CALL" ,
    "OP_RETURN" ,
};
static const char *AST_GRAPHVIZ_HEADER = "digraph {\n"
                                         "splines=ortho\n"
                                         "node[style=\"rounded, filled\", fontsize=8]\n";

//===========================================================================================================================
// STATIC FUNCTION
//===========================================================================================================================

static bool AST_parse_dfs          (const char *buff, const int buff_size, int *const buff_pos, AST_node **const    node);
static bool get_buff_char          (const char *buff, const int buff_size, int *const buff_pos, const char c);

static void do_ast_graphviz_dump   (const AST_node *const node, FILE *const stream, int *const node_num);
static void graphviz_dump_node     (const AST_node *const node, FILE *const stream,  const int node_num);
static void graphviz_dump_edge     (const int        node_from, const int  node_to,  FILE *const stream);

static void graphviz_describe_node (const AST_node *const node, FILE *const stream, const GRAPHVIZ_COLOR     color,
                                                                                    const GRAPHVIZ_COLOR fillcolor, const int node_num);
static void get_node_value_message (const AST_node *const     node, char *const node_value);
static void system_graphviz_dump   (char           *const dump_txt, char *const   dump_png);

//===========================================================================================================================
// AST_NODE_CTOR_DTOR
//===========================================================================================================================

#define AST_NODE_CTOR(union_value_field, value_field_type, ast_node_type)                                                   \
                                                                                                                            \
void AST_node_##ast_node_type##_ctor(AST_node *const node, const value_field_type value, AST_node *const left ,             \
                                                                                         AST_node *const right,             \
                                                                                         AST_node *const prev )             \
{                                                                                                                           \
    assert(node != nullptr);                                                                                                \
                                                                                                                            \
    $type             = ast_node_type;                                                                                      \
    union_value_field = value;                                                                                              \
                                                                                                                            \
    L = left;  if (left  != nullptr) { left ->prev = node; }                                                                \
    R = right; if (right != nullptr) { right->prev = node; }                                                                \
    P = prev;                                                                                                               \
}

#define AST_NEW_NODE(union_value_field, value_field_type, ast_node_type)                                                    \
                                                                                                                            \
AST_node *new_##ast_node_type##_AST_node(const value_field_type value, AST_node *const left ,                               \
                                                                       AST_node *const right,                               \
                                                                       AST_node *const prev )                               \
{                                                                                                                           \
    AST_node *node = (AST_node *) log_calloc(1, sizeof(AST_node));                                                          \
                                                                                                                            \
    $type             = ast_node_type;                                                                                      \
    union_value_field = value;                                                                                              \
                                                                                                                            \
    L = left;  if (left  != nullptr) { left ->prev = node; }                                                                \
    R = right; if (right != nullptr) { right->prev = node; }                                                                \
    P = prev;                                                                                                               \
                                                                                                                            \
    return node;                                                                                                            \
}

AST_NODE_CTOR($fictional ,           int, FICTIONAL)
AST_NODE_CTOR($int_num   ,           int, NUMBER   )
AST_NODE_CTOR($var_index ,           int, VARIABLE )
AST_NODE_CTOR($fictional ,           int, OP_IF    )
AST_NODE_CTOR($fictional ,           int, IF_ELSE  )
AST_NODE_CTOR($fictional ,           int, OP_WHILE )
AST_NODE_CTOR($op_type   , OPERATOR_TYPE, OPERATOR )
AST_NODE_CTOR($var_index ,           int, VAR_DECL )
AST_NODE_CTOR($func_index,           int, FUNC_CALL)
AST_NODE_CTOR($func_index,           int, FUNC_DECL)
AST_NODE_CTOR($fictional ,           int, OP_RETURN)

AST_NEW_NODE($fictional ,           int, FICTIONAL)
AST_NEW_NODE($int_num   ,           int, NUMBER   )
AST_NEW_NODE($var_index ,           int, VARIABLE )
AST_NEW_NODE($fictional ,           int, OP_IF    )
AST_NEW_NODE($fictional ,           int, IF_ELSE  )
AST_NEW_NODE($fictional ,           int, OP_WHILE )
AST_NEW_NODE($op_type   , OPERATOR_TYPE, OPERATOR )
AST_NEW_NODE($var_index ,           int, VAR_DECL )
AST_NEW_NODE($func_index,           int, FUNC_CALL)
AST_NEW_NODE($func_index,           int, FUNC_DECL)
AST_NEW_NODE($fictional ,           int, OP_RETURN)

#undef AST_NODE_CTOR
#undef AST_NEW_NODE

void AST_node_dtor(AST_node *const node)
{
    log_free(node);
}

void AST_tree_dtor(AST_node *const node)
{
    if (node == nullptr) return;

    AST_tree_dtor(node->left);
    AST_tree_dtor(node->right);
    AST_node_dtor(node);
}

//===========================================================================================================================
// PARSE_CONVERT
//===========================================================================================================================

                                                                // default tab_shift = 0
void AST_convert(const AST_node *const node, FILE *const stream, const int tab_shift)
{
    assert(node   != nullptr);
    assert(stream != nullptr);

    int node_value = 0;
    switch ($type)
    {
        case FICTIONAL:
        case OP_IF    :
        case IF_ELSE  :
        case OP_WHILE :
        case OP_RETURN: node_value = 0;
                        break;
        case NUMBER   : node_value = $int_num;
                        break;
        case VARIABLE :
        case VAR_DECL : node_value = $var_index;
                        break;
        case FUNC_CALL:
        case FUNC_DECL: node_value = $func_index;
                        break;
        case OPERATOR:  node_value = $op_type;
                        break;
        default       : break;
    }

    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "{ %d %d\n", $type, node_value);

    if (L != nullptr) AST_convert(L, stream, tab_shift + 1);
    if (R != nullptr) AST_convert(R, stream, tab_shift + 1);

    fprintf_tab(stream, tab_shift);
    fprintf    (stream, "}\n");
}

void fprintf_tab(FILE *const stream, const int tab_shift)
{
    assert(stream != nullptr);

    for(int i = 0; i < tab_shift; ++i)
    {
        putc('\t', stream);
    }
}

AST_node *AST_parse(const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    AST_node *tree = nullptr;
    if (!AST_parse_dfs(buff, buff_size, buff_pos, &tree)) return nullptr;
    return tree;
}

#define fprintf_err(message) fprintf(stderr, TERMINAL_RED "ERROR: " TERMINAL_CANCEL "%s", message)

#define AST_parse_dfs_err_exit                                                                                              \
        AST_tree_dtor(*node);                                                                                               \
        *node = nullptr;                                                                                                    \
        return false;

static bool AST_parse_dfs(const char *buff, const int buff_size, int *const buff_pos, AST_node **const node)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);
    assert(node     != nullptr);
    assert(*node    == nullptr);

    if (!get_buff_char(buff, buff_size, buff_pos, '{'))
    {
        fprintf_err("expected '{'\n");
        fprintf(stderr, "buff_pos = %d\n", *buff_pos);
        AST_parse_dfs_err_exit
    }

    int node_type = 0;
    int node_val  = 0;

    if (!get_buff_int(buff, buff_size, buff_pos, &node_type))
    {
        fprintf_err("expected node type\n");
        AST_parse_dfs_err_exit
    }
    if (!get_buff_int(buff, buff_size, buff_pos, &node_val))
    {
        fprintf_err("expected node value\n");
        AST_parse_dfs_err_exit
    }

    switch (node_type)
    {
        case FICTIONAL: *node = new_FICTIONAL_AST_node(node_val); break;
        case NUMBER   : *node = new_NUMBER_AST_node   (node_val); break;
        case VARIABLE : *node = new_VARIABLE_AST_node (node_val); break;
        case OP_IF    : *node = new_OP_IF_AST_node    (node_val); break;
        case IF_ELSE  : *node = new_IF_ELSE_AST_node  (node_val); break;
        case OP_WHILE : *node = new_OP_WHILE_AST_node (node_val); break;
        case OPERATOR : *node = new_OPERATOR_AST_node ((OPERATOR_TYPE) node_val); break;
        case VAR_DECL : *node = new_VAR_DECL_AST_node (node_val); break;
        case FUNC_DECL: *node = new_FUNC_DECL_AST_node(node_val); break;
        case FUNC_CALL: *node = new_FUNC_CALL_AST_node(node_val); break;
        case OP_RETURN: *node = new_OP_RETURN_AST_node(node_val); break;
        default       : fprintf_err("invalid node type\n");
                        AST_parse_dfs_err_exit
    }
    if (get_buff_char(buff, buff_size, buff_pos, '}')) return true;

    AST_node *left  = nullptr;
    AST_node *right = nullptr;

    if (!AST_parse_dfs(buff, buff_size, buff_pos, &left)) { AST_parse_dfs_err_exit }
    if ( get_buff_char(buff, buff_size, buff_pos, '}'))
    {
        if ((*node)->type == FUNC_DECL) //в этом случае у функции нет аргументов
        {
            (*node)->right =  left;
            left   ->prev  = *node;
        }
        else
        {
            (*node)->left =  left;
            left   ->prev = *node;
        }
        return true;
    }
    if (!AST_parse_dfs(buff, buff_size, buff_pos, &right))
    {
        AST_tree_dtor(left);
        AST_parse_dfs_err_exit
    }
    if (!get_buff_char(buff, buff_size, buff_pos, '}'))
    {
        AST_tree_dtor(left);
        fprintf_err("expected '}'\n");
        AST_parse_dfs_err_exit
    }
    (*node)->left  =  left;
    (*node)->right = right;

    left ->prev = *node;
    right->prev = *node;

    return true;
}
#undef AST_parse_dfs_err_exit
#undef fprintf_err

bool get_buff_int(const char *buff, const int buff_size, int *const buff_pos, int *const int_num)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);
    assert(int_num  != nullptr);

    skip_ast_spaces(buff, buff_size, buff_pos);

    *int_num    = 0;
    int num_len = 0;

    if (*buff_pos >= buff_size)                                   return false;
    if (sscanf(buff + *buff_pos, "%d%n", int_num, &num_len) != 1) return false;

    *buff_pos += num_len;
    return true;
}

static bool get_buff_char(const char *buff, const int buff_size, int *const buff_pos, const char c)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    skip_ast_spaces(buff, buff_size, buff_pos);

    if (*buff_pos == buff_size) return false;
    if (buff[*buff_pos] == c)
    {
        *buff_pos += 1;
        return true;
    }
    return false;
}
                                                                                // default _name_beg = nullptr,  _name_len = nullptr
bool get_ast_word(const char *buff, const int buff_size, int *const buff_pos, const char **_name_beg, int *const _name_len)
{
    assert(buff      != nullptr);
    assert(buff_pos  != nullptr);

    skip_ast_spaces(buff, buff_size, buff_pos);
    if (*buff_pos == buff_size) return false;

    const char *name_beg = buff + *buff_pos;
    int         name_len = 0;

    while (*buff_pos < buff_size && buff[*buff_pos] != '\0'
                                 && buff[*buff_pos] != '['
                                 && !isspace(buff[*buff_pos])) *buff_pos += 1;

    name_len = (buff + *buff_pos) - name_beg;

    if (_name_beg != nullptr) *_name_beg = name_beg;
    if (_name_len != nullptr) *_name_len = name_len;

    return true;
}

void skip_ast_spaces(const char *buff, const int buff_size, int *const buff_pos)
{
    assert(buff     != nullptr);
    assert(buff_pos != nullptr);

    skip_spaces(buff, buff_size, buff_pos);

    if    (buff_size == *buff_pos) return;
    while (buff[*buff_pos] == '[') //начало комментария
    {
        *buff_pos += 1;

        while (*buff_pos < buff_size &&       *buff_pos  != ']') *buff_pos += 1;
        if    (*buff_pos < buff_size &&  buff[*buff_pos] == ']') *buff_pos += 1;

        skip_spaces(buff, buff_size, buff_pos);
    }
}

//===========================================================================================================================
// DUMP
//===========================================================================================================================

void AST_tree_graphviz_dump(const AST_node *const root)
{
    if (root == nullptr)
    {
        log_warning("node to dump is nullptr(%d)\n", __LINE__);
        return;
    }

    static int cur = 0;

    char dump_txt[GRAPHVIZ_SIZE_FILE] = "";
    char dump_png[GRAPHVIZ_SIZE_FILE] = "";

    sprintf(dump_txt, "dump_txt/ast%d.txt", cur);
    sprintf(dump_png, "dump_png/ast%d.png", cur);

    FILE *stream_txt =  fopen(dump_txt, "w");
    if   (stream_txt == nullptr)
    {
        log_error("can't open dump file(%d)\n", __LINE__);
        return;
    }
    ++cur;

    setvbuf(stream_txt, nullptr, _IONBF, 0);
    fprintf(stream_txt, "%s", AST_GRAPHVIZ_HEADER);

    int node_num = 0;
    do_ast_graphviz_dump(root, stream_txt, &node_num);

    fprintf(stream_txt, "}\n");
    system_graphviz_dump(dump_txt, dump_png);

    fclose(stream_txt);
}

static void do_ast_graphviz_dump(const AST_node *const node, FILE *const stream, int *const node_num)
{
    assert(node     != nullptr);
    assert(stream   != nullptr);
    assert(node_num != nullptr);

    const int node_cur = *node_num;

    graphviz_dump_node(node, stream, *node_num);
    *node_num += 1;

    const int node_l = *node_num;
    if (L != nullptr) { do_ast_graphviz_dump(L, stream, node_num); graphviz_dump_edge  (node_cur, node_l, stream); }

    const int node_r = *node_num;
    if (R != nullptr) { do_ast_graphviz_dump(R, stream, node_num); graphviz_dump_edge  (node_cur, node_r, stream); }
}

static void graphviz_dump_edge(const int node_from, const int node_to, FILE *const stream)
{
    assert(stream != nullptr);

    fprintf(stream, "node%d->node%d[color=\"black\"]\n", node_from, node_to);
}

static void graphviz_dump_node(const AST_node *const node, FILE *const stream, const int node_num)
{
    assert(node   != nullptr);
    assert(stream != nullptr);

    GRAPHVIZ_COLOR     color = BLACK;
    GRAPHVIZ_COLOR fillcolor = BLACK;

    switch ($type)
    {
        case FICTIONAL: fillcolor = LIGHT_GREY;
                            color =      BLACK;
                        break;
        case NUMBER:
        case VARIABLE:  fillcolor = LIGHT_BLUE;
                            color =  DARK_BLUE;
                        break;
        case OP_IF:
        case IF_ELSE:
        case OP_WHILE:
        case OP_RETURN: fillcolor = LIGHT_GREEN;
                            color =  DARK_GREEN;
                        break;
        case OPERATOR:
        case FUNC_CALL: fillcolor = LIGHT_ORANGE;
                            color =  DARK_ORANGE;
                        break;
        case VAR_DECL:
        case FUNC_DECL: fillcolor = LIGHT_PINK;
                            color =  DARK_PINK;
                        break;
        default:        break;
    }
    graphviz_describe_node(node, stream, color, fillcolor, node_num);
}

static void graphviz_describe_node(const AST_node *const node, FILE *const stream, const GRAPHVIZ_COLOR     color,
                                                                                   const GRAPHVIZ_COLOR fillcolor, const int node_num)
{
    assert(node   != nullptr);
    assert(stream != nullptr);

    char node_value[GRAPHVIZ_SIZE_CMD] = {};
    get_node_value_message(node, node_value);

    fprintf(stream, "node%d[color=\"%s\", fillcolor=\"%s\", label=\"{type: %s | %s }\"]\n",
                         node_num,
                                    GRAPHVIZ_COLOR_NAMES[color],
                                                      GRAPHVIZ_COLOR_NAMES[fillcolor],
                                                                           AST_NODE_TYPE_NAMES[$type],
                                                                                node_value);
}

static void get_node_value_message(const AST_node *const node, char *const node_value)
{
    assert(node       != nullptr);
    assert(node_value != nullptr);

    switch ($type)
    {
        case FICTIONAL:
        case OP_IF:
        case IF_ELSE:
        case OP_WHILE:
        case OP_RETURN: sprintf(node_value, "'-'");
                        break;
        case NUMBER:    sprintf(node_value, "%d", $int_num);
                        break;
        case VARIABLE:
        case VAR_DECL:  sprintf(node_value, "var_index: %d", $var_index);
                        break;
        case FUNC_CALL:
        case FUNC_DECL: sprintf(node_value, "func_index: %d", $func_index);
                        break;
        case OPERATOR:  sprintf(node_value, "%s", OPERATOR_NAMES[$op_type]);
                        break;
        default:        break;
    }
}

static void system_graphviz_dump(char *const dump_txt, char *const dump_png)
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