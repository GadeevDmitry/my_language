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

#include "frontend.h"
#include "terminal_colors.h"
#include "name_list.h"

/*
//===========================================================================================================================
// general: return none
//===========================================================================================================================

general = {var_declaration | function_declaration}+   //"CAMP_NOU" - имя главной функции

//===========================================================================================================================
// var_declaration: return none
//===========================================================================================================================

var_declaration = "BARCELONA" var_name ';'
                            //var_name - имя новой переменной
                            //имя      - последовательность не ключевых символов, не являющаяся ключевым словом
                            //имя не может начинаться с цифры

//===========================================================================================================================
// function_declaration: return none
// function_call:        return value returned by the function
// op_function_call:     return none
//===========================================================================================================================

function_declaration = "BARCELONA" func_name '(' {var_name {',' var_name}*}? ')' '{' operators '}'
                                // func_name - имя новой функции

function_call        = func '(' {rvalue {',' rvalue}*}? ')'
                    // func - имя существующей функции

op_function_call     = function_call ';'

//===========================================================================================================================
// operator: return none
//===========================================================================================================================

operator   = var_declaration | op_assignment | op_input | op_output | if | while | op_function_call | op_return
opearators = {operator}*

//===========================================================================================================================
// op_assignment: return none
//===========================================================================================================================

op_assignment = lvalue '=' rvalue ';'

//===========================================================================================================================
// op_input:  return none
// op_output: return none
//===========================================================================================================================

op_input  = "ARGENTINA" lvalue ';'
op_output = "POTUGAL"   rvalue ';'

//===========================================================================================================================
// if: return none
//===========================================================================================================================

if = "MESSI" '(' rvalue ')' '{' operators '}' {"SUAREZ" '{' operators '}'}?

//===========================================================================================================================
// while: return none
//===========================================================================================================================

while = "NEYMAR" '(' rvalue ')' '{' operators '}'

//===========================================================================================================================
// op_return: return none
//===========================================================================================================================

op_return = "CHAMPIONS_LEAGUE" rvalue ';'

//===========================================================================================================================
// rvalue: return value of expretion
//===========================================================================================================================

rvalue       = {lvalue '='}* op_or
op_or        = op_and     {'||'        op_and    }*
op_and       = op_equal   {'&&'        op_equal  }*
op_equal     = op_compare {[== !=]     op_compare}?
op_compare   = op_add_sub {[< > <= >=] op_add_sub}?
op_add_sub   = op_mul_div {[+-]        op_mul_div}*
op_mul_div   = op_pow     {[/*]        op_pow    }*
op_pow       = op_not     {'^'         op_not    }?
op_not       = {!}? operand
operand      = '('rvalue ')' | function_call | rvalue_token

rvalue_token = number | lvalue
number       = ['0'-'9']+

//===========================================================================================================================
// lvalue: return value of variable
//===========================================================================================================================

        //var - имя существующей переменной
lvalue = var
*/

//===========================================================================================================================
// MAIN
//===========================================================================================================================

int main(const int argc, const char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "you should give one parameter: source code\n");
        return 0;
    }
    source *code = new_source(argv[1]);
    if     (code == nullptr) return 0;
    source_text_dump(code);

    lexical_analyzer   (code);
    lexis_graphviz_dump(code);

    source_dtor(code);
}

//===========================================================================================================================
// TRANSLATOR
//===========================================================================================================================



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
    node->type        = ast_node_type;                                                                                      \
    union_value_field = value;                                                                                              \
                                                                                                                            \
    node->left        = left;                                                                                               \
    node->right       = right;                                                                                              \
    node->prev        = prev;                                                                                               \
}

AST_NODE_CTOR(node->value.fictional ,           int, FICTIONAL)
AST_NODE_CTOR(node->value.int_num   ,           int, NUMBER   )
AST_NODE_CTOR(node->value.var_index ,           int, VARIABLE )
AST_NODE_CTOR(node->value.fictional ,           int, OP_IF    )
AST_NODE_CTOR(node->value.fictional ,           int, IF_ELSE  )
AST_NODE_CTOR(node->value.fictional ,           int, OP_WHILE )
AST_NODE_CTOR(node->value.op_type   , OPERATOR_TYPE, OPERATOR )
AST_NODE_CTOR(node->value.var_index ,           int, VAR_DECL )
AST_NODE_CTOR(node->value.func_index,           int, FUNC_CALL)
AST_NODE_CTOR(node->value.func_index,           int, FUNC_DECL)
AST_NODE_CTOR(node->value.fictional ,           int, OP_RETURN)

#undef AST_NODE_CTOR

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
// SOURCE_CTOR_DTOR
//===========================================================================================================================

source *new_source(const char *src_file)
{
    assert(src_file != nullptr);

    source *new_code = (source *) log_calloc(1, sizeof(source));
    
    if (new_buff (src_file, new_code) == false) { source_dtor(new_code); return nullptr; }
    new_lexis(new_code);

    return new_code;
}

bool new_buff(const char *src_file, source *const code)
{
    assert(src_file != nullptr);
    assert(code     != nullptr);

    buff_data = (const char *) read_file(src_file, &buff_size);
    buff_line = 1;
    buff_pos  = 0;

    if (buff_data == nullptr)
    {
        fprintf(stderr, "can't open sorce file \"%s\"\n", src_file);
        return false;
    }
    return true;
}

void new_lexis(source *const code)
{
    assert(code != nullptr);

    lexis_data     = (token *) log_calloc((size_t) buff_size, sizeof(token));
    lexis_pos      = 0;
    lexis_capacity = buff_size;
}

void source_dtor(source *const code) //only if parameter "code" was created by malloc or calloc
{
    assert(code != nullptr);

    log_free((char *)buff_data);
    log_free(       lexis_data);
    log_free(code);
}

//===========================================================================================================================
// LEXICAL_ANALYSIS
//===========================================================================================================================

void lexical_analyzer(source *const code)
{
    assert(code != nullptr);

    skip_source_spaces(code);

    while (buff_pos < buff_size && buff_data[buff_pos] != '\0')
    {
        if      (comment(code)) continue;
        if      (key_double_char(code))         create_key_double_char_token(code);
        else if (key_char(buff_data[buff_pos])) create_key_char_token       (code);
        else
        {
            int token_beg = buff_pos;
            int token_len = get_another_token(code);

            if      (get_int_num      (code, token_beg, token_len)) create_int_num_token (code, token_beg, token_len);
            else if (get_key_word_type(code, token_beg, token_len)) create_key_word_token(code, token_beg, token_len);
            else                                                    create_undef_token   (code, token_beg, token_len);
        }
        skip_source_spaces(code);
    }
}

int get_another_token(source *const code)
{
    assert(code != nullptr);

    int token_len = 0;
    while (buff_pos < buff_size && !split_char(buff_data[buff_pos]))
    {
        ++buff_pos;
        ++token_len;
    }
    return token_len;
}
                                                                                                //default type = nullptr
bool get_key_word_type(source *const code, const int token_beg, const int token_len, KEY_WORD_TYPE *const type)
{
    assert(code != nullptr);

    for (size_t i = 0; i * sizeof(char *) < sizeof(KEY_WORD_NAMES); ++i)
    {
        if (!strncmp(KEY_WORD_NAMES[i], buff_data + token_beg, (size_t)token_len))
        {
            if (type != nullptr) *type = (KEY_WORD_TYPE) i;
            return true;
        }
    }
    return false;
}
                                                           //default type = nullptr
bool key_double_char(source *const code, KEY_CHAR_DOUBLE_TYPE *const type)
{
    assert(code != nullptr);

    if (buff_pos + 2 > buff_size) return false;

    for (size_t i = 0; i * sizeof(char *) < sizeof(KEY_CHAR_DOUBLE_NAMES); ++i)
    {
        if (!strncmp(KEY_CHAR_DOUBLE_NAMES[i], buff_data+buff_pos, 2))
        {
            if (type != nullptr) *type = (KEY_CHAR_DOUBLE_TYPE) i;
            return true;
        }
    }
    return false;
}
                                                                                //default num = nullptr
bool get_int_num(source *const code, const int token_beg, const int token_len, int *const num)
{
    assert(code != nullptr);

    int num_ = 0;
    for (int i = token_beg; i < token_beg + token_len; ++i)
    {
        if (!isdigit(buff_data[i])) return false;
        num_ = 10 * num_ + buff_data[i] - '0';
    }
    if (num != nullptr) *num = num_;
    return true;
}

bool comment(source *const code)
{
    assert(code != nullptr);

    if (buff_data[buff_pos] == '#')
    {
        skip_source_line(code);
        return true;
    }
    return false;
}

bool key_char(const char to_check)
{
    for (int i = 0; KEY_CHAR_NAMES[i] != '\0'; ++i)
    {
        if (KEY_CHAR_NAMES[i] == to_check) return true;
    }
    return false;
}

bool split_char(const char to_check)
{
    if (to_check == '\0')   return true;
    if (isspace (to_check)) return true;
    if (key_char(to_check)) return true;

    return false;
}

//===========================================================================================================================
// TOKEN_CTOR_DTOR
//===========================================================================================================================

void create_key_word_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_WORD;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;

    get_key_word_type(code, token_beg, token_len, &lexis_data[lexis_pos].key_word_val);
    ++lexis_pos;
}

void create_key_char_token(source *const code)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_CHAR;
    lexis_data[lexis_pos].token_beg     = buff_pos;
    lexis_data[lexis_pos].token_line    = buff_line;
    lexis_data[lexis_pos].key_char_val  = buff_data[buff_pos++];

    ++lexis_pos;
}

void create_key_double_char_token(source *const code)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_CHAR_DOUBLE;
    lexis_data[lexis_pos].token_beg     = buff_pos;
    lexis_data[lexis_pos].token_line    = buff_line;

    key_double_char(code, &lexis_data[lexis_pos].key_dbl_char_val);
    ++lexis_pos; buff_pos += 2;
}

void create_int_num_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = INT_NUM;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;

    get_int_num(code, token_beg, token_len, &lexis_data[lexis_pos].int_num_val);
    ++lexis_pos;
}

void create_undef_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = UNDEF_TOKEN;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;
    lexis_data[lexis_pos].token_len_val = token_len;

    ++lexis_pos;
}

//===========================================================================================================================
// SKIP_SOURCE
//===========================================================================================================================

void skip_source_line(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && buff_data[buff_pos] != '\n') ++buff_pos;
    
    if (buff_pos < buff_size)
    {
        ++buff_pos;
        ++buff_line;
    }
    skip_source_spaces(code);
}

void skip_source_spaces(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && isspace(buff_data[buff_pos]))
    {
        if (buff_data[buff_pos] == '\n') ++buff_line;
        ++buff_pos;
    }
}

//===========================================================================================================================
// DUMP
//===========================================================================================================================

void source_text_dump(source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr\n");
        return;
    }

    log_message("\n"
                "code\n"
                "{\n"
                "    buff\n"
                "    {\n"
                "        data = %p\n"
                "        pos  = %d\n"
                "        line = %d\n"
                "        size = %d\n"
                "    }\n"
                "    lexis\n"
                "    {\n"
                "        data     = %p\n"
                "        pos      = %d\n"
                "        capacity = %d\n"
                "    }\n"
                "}\n\n",
                buff_data, buff_pos, buff_line, buff_size, lexis_data, lexis_pos, lexis_capacity);
}

void lexis_graphviz_dump(source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr(%d)\n", __LINE__);
        return;
    }
    static int cur = 0;

    char dump_txt[GRAPHVIZ_SIZE_FILE] = "";
    char dump_png[GRAPHVIZ_SIZE_FILE] = "";

    sprintf(dump_txt, "dump_txt/lexis%d.txt", cur);
    sprintf(dump_png, "dump_png/lexis%d.png", cur);

    FILE *stream_txt =  fopen(dump_txt, "w");
    if   (stream_txt == nullptr)
    {
        log_error("can't open dump file(%d)\n", __LINE__);
        return;
    }
    ++cur;

    setvbuf(stream_txt, nullptr, _IONBF, 0);
    fprintf(stream_txt, "%s", LEXIS_GRAPHVIZ_HEADER);

    do_lexis_graphviz_dump(code, stream_txt);

    fprintf(stream_txt, "}\n");
    system_graphviz_dump(dump_txt, dump_png);

    fclose(stream_txt);
}


void do_lexis_graphviz_dump(source *const code, FILE *const stream)
{
    assert(code   != nullptr);
    assert(stream != nullptr);

    for (int i = 0; i < lexis_pos; ++i)
    {
        graphviz_dump_token(lexis_data+i, stream, i);
    }
    for (int i = 0; i < lexis_pos - 1; ++i)
    {
        graphviz_dump_edge(i, i+1, stream);
    }
}

void graphviz_dump_edge(const int token_num_from, const int token_num_to, FILE *const stream)
{
    assert(stream != nullptr);

    fprintf(stream, "node%d->node%d[weight=0, style=\"invis\"]\n", token_num_from, token_num_to);
}

void graphviz_dump_token(const token *const cur_token, FILE *const stream, const int token_num)
{
    assert(cur_token != nullptr);
    assert(stream    != nullptr);

    GRAPHVIZ_COLOR     color = BLACK;
    GRAPHVIZ_COLOR fillcolor = BLACK;

    switch (cur_token->type)
    {
        case KEY_WORD:          fillcolor = LIGHT_BLUE;
                                    color =  DARK_BLUE;
                                break;
        case KEY_CHAR_DOUBLE:   fillcolor = LIGHT_GREEN;
                                    color =  DARK_GREEN;
                                break;
        case KEY_CHAR:          fillcolor = LIGHT_PINK;
                                    color =   DARK_RED;
                                break;
        case INT_NUM :          fillcolor = LIGHT_GREY;
                                    color =      BLACK;
                                break;
        case UNDEF_TOKEN:       fillcolor = LIGHT_ORANGE;
                                    color =  DARK_ORANGE;
                                break;
        default:                break;
    }
    graphviz_describe_token(cur_token, stream, color, fillcolor, token_num);
}

void graphviz_describe_token(const token *const cur_token, FILE *const stream, const GRAPHVIZ_COLOR     color,
                                                                               const GRAPHVIZ_COLOR fillcolor, const int token_num)
{
    assert(cur_token != nullptr);
    assert(stream    != nullptr);

    char token_value[GRAPHVIZ_SIZE_CMD] = {};
    get_token_value_message(cur_token, token_value);

    fprintf(stream, "node%d[color=\"%s\", fillcolor=\"%s\", label=\"{token number %d\\n | type: %s\\n | token_beg: %d\\n | token_line: %d\\n | %s}\"]\n",
                         token_num,
                                    GRAPHVIZ_COLOR_NAMES[color],
                                                      GRAPHVIZ_COLOR_NAMES[fillcolor],
                                                                                  token_num,
                                                                                                TOKEN_TYPE_NAMES[cur_token->type],
                                                                                                                   cur_token->token_beg,
                                                                                                                                       cur_token->token_line,
                                                                                                                                               token_value);
}

void get_token_value_message(const token *const cur_token, char *const token_value)
{
    assert(cur_token   != nullptr);
    assert(token_value != nullptr);

    switch(cur_token->type)
    {
        case KEY_WORD   :       sprintf(token_value, "word: %s", KEY_WORD_NAMES[cur_token->key_word_val]);
                                break;
        case KEY_CHAR_DOUBLE:   sprintf(token_value, "dbl_char: \\\"%s\\\"", KEY_CHAR_DOUBLE_NAMES[cur_token->key_dbl_char_val]);
                                break;
        case KEY_CHAR   :       if      (cur_token->key_char_val == '{') sprintf(token_value, "char: opened fig");
                                else if (cur_token->key_char_val == '}') sprintf(token_value, "char: closed fig");
                                else                                     sprintf(token_value, "char: \\\'%c\\\'", cur_token->key_char_val);
                                break;
        case INT_NUM    :       sprintf(token_value, "num: %d", cur_token->int_num_val);
                                break;
        case UNDEF_TOKEN:       sprintf(token_value, "len: %d", cur_token->token_len_val);
                                break;
        default:                break;
    }
}

void system_graphviz_dump(char *const dump_txt, char *const dump_png)
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