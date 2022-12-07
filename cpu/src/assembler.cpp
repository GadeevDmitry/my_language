#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "../../lib/logs/log.h"
#include "../../lib/read_write/read_write.h"
#include "../../lib/algorithm/algorithm.h"
#include "../../lib/graphviz_dump/graphviz_dump.h"

#include "dsl.h"
#include "label.h"
#include "terminal_colors.h"
#include "assembler.h"

/*===========================================================================================================================*/
// MAIN
/*===========================================================================================================================*/

int main(const int argc, const char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "You should give two parameters: file to compile and execute file.\n");
        return 0;
    }
    source *code = new_source(argv[1]);
    FILE *stream = fopen     (argv[2], "w");
    
    if (code   == nullptr) { fclose(stream); return 0; }
    if (stream == nullptr)
    {
        source_dtor(code);

        fprintf(stderr, "can't open execute file\n");
        return 0;
    }

    lexical_analyzer   (code);
    lexis_graphviz_dump(code);

    source_dtor(code);
    fclose(stream);
}

/*===========================================================================================================================*/
// SOURCE_CTOR_DTOR
/*===========================================================================================================================*/

source *new_source(const char *source_file)
{
    assert(source_file != nullptr);

    source *code = (source *) log_calloc(1, sizeof(source));

    if (!source_buff_ctor (code, source_file)) { source_dtor(code); return nullptr; }
    if (!source_lexis_ctor(code)             ) { source_dtor(code); return nullptr; }

    return code;
}

bool source_buff_ctor(source *const code, const char *source_file)
{
    assert(code        != nullptr);
    assert(source_file != nullptr);

    buff_data = (const char *) read_file(source_file, &buff_size);
    buff_line = 1;
    buff_pos  = 0;

    if (buff_data == nullptr)
    {
        log_error(        "can't open \"%s\"(%d)\n", source_file, __LINE__);
        fprintf  (stderr, "can't open \"%s\"\n"    , source_file);
        return false;
    }
    return true;
}

bool source_lexis_ctor(source *const code)
{
    assert(code != nullptr);

    lexis_data      = (token *) log_calloc((size_t) buff_size, sizeof(token));
    lexis_cur_token = (char  *) log_calloc((size_t) buff_size, sizeof(char));
    lexis_pos       = 0;

    if (lexis_data == nullptr || lexis_cur_token == nullptr)
    {
        log_error(        "can't allocate memory for lexis(%d)\n", __LINE__);
        fprintf  (stderr, "can't allocate memory for lexis\n");
        return false;
    }
    return true;
}

void source_dtor(source *const code)
{
    if (code == nullptr)
    {
        log_warning("struct source to dtor is nullptr(%d)\n", __LINE__);
        return;
    }

    log_free((char *) buff_data);
    log_free(lexis_data);
    log_free(lexis_cur_token);

    log_free(code);
}

/*===========================================================================================================================*/
// LEXICAL_ANALYSIS
/*===========================================================================================================================*/

void lexical_analyzer(source *const code)
{
    assert(code != nullptr);

    skip_source_spaces(code);
    while (buff_pos < buff_size)
    {
        if (is_comment (code)) continue;      // spaces are skipped after is_comment()
        if (is_key_char(buff_data[buff_pos])) create_key_char_token(code);
        else
        {
            int token_beg = buff_pos;
            int token_len = get_another_token(code);

            if      (get_reg_name(lexis_cur_token, token_len) != ERR_REG) create_reg_token(code, token_beg, token_len);
            else if (get_int_num (lexis_cur_token           ))            create_int_token(code, token_beg);
            else
            {
                ASM_CMD cur_instruction = get_asm_cmd(lexis_cur_token);

                if (cur_instruction == UNDEF_ASM_CMD) create_undef_token      (code, token_beg, token_len);
                else                                  create_instruction_token(code, token_beg, cur_instruction);
            }
        }

        skip_source_spaces(code);
    }
}

int get_another_token(source *const code)
{
    assert(code != nullptr);

    int token_len = 0;
    while (buff_pos < buff_size &&  !is_key_char(buff_data[buff_pos]) &&
                                    !isspace    (buff_data[buff_pos]))
    {
        lexis_cur_token[token_len++] = buff_data[buff_pos++];
    }

    lexis_cur_token[token_len++] = '\0';
    return token_len;
}

 REGISTER get_reg_name(const char *cur_token, const int token_len)
{
    assert(cur_token != nullptr);

    if (token_len != REG_NAME_LEN) return ERR_REG;

    REGISTER reg = ERR_REG;
    if (tolower(cur_token[0]) == 'r' && tolower(cur_token[2]) == 'x') reg = (REGISTER) (tolower(cur_token[1]) - 'a' + 1);

    if (0 <= reg && reg <= REG_NUMBER) return reg;
    return reg = ERR_REG;
}

bool get_int_num(const char *cur_token, int *const ret) //ret = nullptr
{
    assert(cur_token != nullptr);

    int int_num = 0;
    if (sscanf(cur_token, "%d", &int_num) == 0) return false;

    if (ret != nullptr) *ret = int_num;
    return true;
}

ASM_CMD get_asm_cmd(const char *cur_token)
{
    assert(cur_token != nullptr);

    if (!strcasecmp("hlt" , cur_token)) return HLT ;
    
    if (!strcasecmp("in"  , cur_token)) return IN  ;
    if (!strcasecmp("out" , cur_token)) return OUT ;

    if (!strcasecmp("push", cur_token)) return PUSH;
    if (!strcasecmp("pop" , cur_token)) return POP ;

    if (!strcasecmp("jmp" , cur_token)) return JMP ;
    if (!strcasecmp("ja"  , cur_token)) return JA  ;
    if (!strcasecmp("jae" , cur_token)) return JAE ;
    if (!strcasecmp("jb"  , cur_token)) return JB  ;
    if (!strcasecmp("jbe" , cur_token)) return JBE ;
    if (!strcasecmp("je"  , cur_token)) return JE  ;
    if (!strcasecmp("jne" , cur_token)) return JNE ;

    if (!strcasecmp("call", cur_token)) return CALL;
    if (!strcasecmp("ret" , cur_token)) return RET ;

    if (!strcasecmp("add" , cur_token)) return ADD ;
    if (!strcasecmp("sub" , cur_token)) return SUB ;
    if (!strcasecmp("mul" , cur_token)) return MUL ;
    if (!strcasecmp("div" , cur_token)) return DIV ;
    if (!strcasecmp("pow" , cur_token)) return POW ;

    return UNDEF_ASM_CMD;
}

bool is_key_char(const char char_to_check)
{
    for (int i = 0; KEY_CHARS[i] != '\0'; ++i)
    {
        if (char_to_check == KEY_CHARS[i]) return true;
    }
    return false;
}

bool is_comment(source *const code)
{
    assert(code != nullptr);

    if (buff_data[buff_pos] == '#')
    {
        skip_source_line  (code);
        skip_source_spaces(code);
        return true;
    }
    return false;
}

/*===========================================================================================================================*/
// TOKEN_CTOR
/*===========================================================================================================================*/

void create_key_char_token(source *const code)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type       = KEY_CHAR;
    lexis_data[lexis_pos].token_beg  = buff_pos;
    lexis_data[lexis_pos].token_line = buff_line;
    lexis_data[lexis_pos].value.key  = buff_data[buff_pos++];

    lexis_pos++;
}

void create_reg_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = REG_NAME;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;
    lexis_data[lexis_pos].value.reg_num = get_reg_name(lexis_cur_token, token_len);

    lexis_pos++;
}

void create_int_token(source *const code, const int token_beg)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type       = INT_NUM;
    lexis_data[lexis_pos].token_beg  = token_beg;
    lexis_data[lexis_pos].token_line = buff_line;

    get_int_num(lexis_cur_token, &lexis_data[lexis_pos].value.int_num);
    lexis_pos++;
}

void create_instruction_token(source *const code, const int token_beg, const ASM_CMD asm_cmd)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type              = INSTRUCTION;
    lexis_data[lexis_pos].token_beg         = token_beg;
    lexis_data[lexis_pos].token_line        = buff_line;
    lexis_data[lexis_pos].value.instruction = asm_cmd;

    lexis_pos++;
}

void create_undef_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type            = UNDEF_TOKEN;
    lexis_data[lexis_pos].token_beg       = token_beg;
    lexis_data[lexis_pos].token_line      = buff_line;
    lexis_data[lexis_pos].value.token_len = token_len;

    lexis_pos++;
}

/*===========================================================================================================================*/
// SKIP_SOURCE
/*===========================================================================================================================*/

void skip_source_line(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && buff_data[buff_pos] != '\n') buff_pos++;

    if (buff_pos < buff_size)
    {
        ++buff_line;
        ++buff_pos;
    }
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

/*===========================================================================================================================*/
// DUMP
/*===========================================================================================================================*/

void lexis_graphviz_dump(const source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr(%d)\n", __LINE__);
        return;
    }

    static int cur = 0;

    char    dump_txt[GRAPHVIZ_SIZE_FILE] = "";
    char    dump_png[GRAPHVIZ_SIZE_FILE] = "";

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

void do_lexis_graphviz_dump(const source *const code, FILE *const stream)
{
    assert(code        != nullptr);
    assert(stream      != nullptr);

    for (int i = 0; i < lexis_pos    ; ++i) graphviz_dump_token(code, stream, i);
    for (int i = 0; i < lexis_pos - 1; ++i) graphviz_dump_edge (      stream, i, i + 1);
}

void graphviz_dump_edge(FILE *const stream, const int from, const int to)
{
    assert(stream != nullptr);

    fprintf(stream, "node%d->node%d[weight=10000, style=\"invis\"]\n", from, to);
}

void graphviz_dump_token(const source *const code, FILE *const stream, const int dumping_pos)
{
    assert(code        != nullptr);
    assert(stream      != nullptr);

    GRAPHVIZ_COLOR     color = BLACK;
    GRAPHVIZ_COLOR fillcolor = BLACK;

    switch(lexis_data[dumping_pos].type)
    {
        case INSTRUCTION:     color =  DARK_BLUE;
                          fillcolor = LIGHT_BLUE;
                          break;
        case REG_NAME   :     color =  DARK_GREEN;
                          fillcolor = LIGHT_GREEN;
                          break;
        case INT_NUM    :     color =  DARK_ORANGE;
                          fillcolor = LIGHT_ORANGE;
                          break;
        case KEY_CHAR   :     color =   DARK_RED;
                          fillcolor = LIGHT_PINK;
                          break;
        case UNDEF_TOKEN:     color =      BLACK;
                          fillcolor = LIGHT_GREY;
                          break;

        default: assert(false && "default case in graphviz_dump_token()");
                 break;
    }
    graphviz_describe_token(code, stream, dumping_pos, color, fillcolor);
}

void graphviz_describe_token(const source *const code, FILE *const stream, const int dumping_pos, const GRAPHVIZ_COLOR     color,
                                                                                                  const GRAPHVIZ_COLOR fillcolor)
{
    assert(code        != nullptr);
    assert(stream      != nullptr);

    const token *cur_token = lexis_data + dumping_pos;
    char         cur_token_value[GRAPHVIZ_SIZE_CMD];

    get_token_dump_message(cur_token, cur_token_value);

    fprintf(stream, "node%d[color=\"%s\", fillcolor=\"%s\", label=\"{type=%s\\n | token_beg=%d\\n | token_line=%d\\n | %s}\"]\n",
                         dumping_pos,
                                    GRAPHVIZ_COLOR_NAMES[color],
                                                      GRAPHVIZ_COLOR_NAMES[fillcolor],
                                                                          TOKEN_TYPE_NAMES[cur_token->type],
                                                                                            cur_token->token_beg,
                                                                                                                cur_token->token_line,
                                                                                                                       cur_token_value);
}

void get_token_dump_message(const token *const cur_token, char *const token_message)
{
    assert(cur_token     != nullptr);
    assert(token_message != nullptr);

    switch(cur_token->type)
    {
        case INSTRUCTION: sprintf(token_message, "value.instruction=%s" , ASM_CMD_NAMES[cur_token->value.instruction]);
                          break;
        case REG_NAME   : sprintf(token_message, "value.reg_num=%s"     , REGISTER_NAMES[cur_token->value.reg_num]);
                          break;
        case INT_NUM    : sprintf(token_message, "value.int_num=%d"     , cur_token->value.int_num);
                          break;
        case KEY_CHAR   : sprintf(token_message, "value.key=\'%c\'"     , cur_token->value.key);
                          break;
        case UNDEF_TOKEN: sprintf(token_message, "value.token_len=%d"   , cur_token->value.token_len);
                          break;

        default: assert(false && "default case in get_token_dump_message()");
                 break;
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