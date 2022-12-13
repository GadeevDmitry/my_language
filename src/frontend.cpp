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

#include "frontend.h"
#include "terminal_colors.h"

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
// VAR_NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void var_name_list_ctor(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    var_store->size     = 0;
    var_store->capacity = 4;    //default capacity
    var_store->var      = (var_info *) log_calloc(2, sizeof(var_info));
}

void var_name_list_dtor(var_name_list *const var_store)
{
    assert(var_store != nullptr);

    for (int i = 0; i < var_store->size; ++i) var_info_dtor(var_store->var+i);
    log_free(var_store->var);

    var_store->var      = nullptr;
    var_store->size     = 0;
    var_store->capacity = 0;
}

void var_info_ctor(var_info *const var, const source *const code, const token *const cur_token, const int scope = -1)
{
    assert(var       != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    assert(cur_token->type == UNDEF_TOKEN);

    const char *name_beg = buff_data + cur_token->token_beg;
    const int   name_len =             cur_token->token_len_val;

    var->name  = strndup(name_beg, name_len);
    stack_ctor(&var->scope, sizeof(int));

    if (scope != -1) stack_push(&var->scope, &scope);
}

void var_info_dtor(var_info *const var)
{
    assert(var != nullptr);

    log_free  ((char *)var->name);
    stack_dtor(&var->scope);
}

//===========================================================================================================================
// VAR_NAME_LIST USER
//===========================================================================================================================

int var_name_list_add_var(var_name_list *const var_store, const source *const code, const token *cur_token, const int scope)
{
    assert(var_store != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

}

void var_info_push_scope(var_info *const var, const int scope)
{
    assert(var != nullptr);

    stack_push(&var->scope, &scope);
}

//===========================================================================================================================
// FUNC_NAME_LIST_CTOR_DTOR
//===========================================================================================================================

void func_name_list_ctor(func_name_list *const func_store)
{
    assert(func_store != nullptr);

    func_store->size     = 0;
    func_store->capacity = 4;   //default capacity
    func_store->func     = (func_info *) log_calloc(4, sizeof(func_info));
}

void func_name_list_dtor(func_name_list *const func_store)
{
    for (int i = 0; i < func_store->size; ++i) func_info_dtor(func_store->func+i);
    log_free(func_store->func);

    func_store->func     = nullptr;
    func_store->size     = 0;
    func_store->capacity = 0;
}

void func_info_ctor(func_info *const func, const source *const code, const token *const cur_token)
{
    assert(func      != nullptr);
    assert(code      != nullptr);
    assert(cur_token != nullptr);

    assert(cur_token->type == UNDEF_TOKEN);

    const char *name_beg = buff_data + cur_token->token_beg;
    const int   name_len =             cur_token->token_len_val;

    func->name = strndup(name_beg, name_len);
    arg_list_ctor(&func->args);
}

void func_info_dtor(func_info *const func)
{
    assert(func != nullptr);

    log_free     ((char *) func->name);
    arg_list_dtor(&func->args);
}

void arg_list_ctor(arg_list *const arg_store)
{
    assert(arg_store != nullptr);

    arg_store->size       = 0;
    arg_store->capacity   = 4;
    arg_store->name_index = (int *) log_calloc(4, sizeof(int));
}

void arg_list_dtor(arg_list *const arg_store)
{
    assert(arg_store != nullptr);

    log_free(arg_store->name_index);

    arg_store->name_index = nullptr;
    arg_store->capacity   = 0;
    arg_store->size       = 0;
}

//===========================================================================================================================
// TRANSLATOR
//===========================================================================================================================

//===========================================================================================================================
// BOOL TOKEN_...
//===========================================================================================================================

bool token_int    (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == INT   ; }
bool token_if     (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == IF    ; }
bool token_else   (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == ELSE  ; }
bool token_while  (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == WHILE ; }
bool token_return (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == RETURN; }
bool token_input  (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == INPUT ; }
bool token_output (const token cur_token) { return cur_token.type == KEY_WORD && cur_token.key_word_val == OUTPUT; }

//===========================================================================================================================
// AST_NODE EXTRA FUNCTION
//===========================================================================================================================

//подвешивает subtree к main_tree, используя фиктивные вершины
void merge_tree(AST_node *const main_tree, AST_node *const subtree)
{
    assert(main_tree != nullptr);
    assert(subtree   != nullptr);

    assert(main_tree->type == FICTIONAL);
    assert(subtree  ->type != FICTIONAL);

    if (main_tree->left == nullptr)
    {
        main_tree->left = subtree;
        subtree  ->prev = main_tree;
        return;
    }
    if (main_tree->right == nullptr)
    {
        main_tree->right = new_FICTIONAL_AST_node(0, subtree, nullptr, main_tree);
        subtree  ->prev  = main_tree->right;
        return;
    }
    merge_tree(main_tree->right, subtree);
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
        if (comment(code)) continue;
        if (key_char(buff_data[buff_pos])) create_key_char_token(code);
        else
        {
            int token_beg = buff_pos;
            int token_len = get_another_token(code);

            if      (get_int_num        (code, token_beg, token_len)) create_int_num_token        (code, token_beg, token_len);
            else if (get_key_double_char(code, token_beg, token_len)) create_key_double_char_token(code, token_beg, token_len);
            else if (get_key_word_type  (code, token_beg, token_len)) create_key_word_token       (code, token_beg, token_len);
            else                                                      create_undef_token          (code, token_beg, token_len);
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
                                                                                                        // default type = nullptr
bool get_key_double_char(source *const code, const int token_beg, const int token_len, KEY_CHAR_DOUBLE_TYPE *const type)
{
    assert(code != nullptr);

    for (size_t i = 0; i * sizeof(char *) < sizeof(KEY_CHAR_DOUBLE_NAMES); ++i)
    {
        if (!strncmp(KEY_CHAR_DOUBLE_NAMES[i], buff_data + token_beg, (size_t)token_len))
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

void create_key_double_char_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = KEY_CHAR_DOUBLE;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;

    get_key_double_char(code, token_beg, token_len, &lexis_data[lexis_pos].key_dbl_char_val);
    ++lexis_pos;
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
        case KEY_CHAR_DOUBLE:   sprintf(token_value, "dbl_char: %s", KEY_CHAR_DOUBLE_NAMES[cur_token->key_dbl_char_val]);
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