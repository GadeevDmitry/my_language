#ifndef ASSEMBLER
#define ASSEMBLER

#include "cpu.h"

/*===========================================================================================================================*/
// CONST
/*===========================================================================================================================*/

const char *LEXIS_GRAPHVIZ_HEADER = "digraph {\n"
                                    "splines=ortho\n"
                                    "node[shape=record, style=\"rounded, filled\", fontsize=8]\n";
const char *KEY_CHARS             = "[]#:";
const int   REG_NAME_LEN          = 4; //including null-character in the end

enum TOKEN_TYPE
{
    INSTRUCTION ,
    REG_NAME    ,
    INT_NUM     ,
    KEY_CHAR    ,
    UNDEF_TOKEN ,
};

const char *TOKEN_TYPE_NAMES[] =
{
    "INSTRUCTION"   ,
    "REG_NAME"      ,
    "INT_NUM"       ,
    "KEY_CHAR"      ,
    "UNDEF_TOKEN"   ,
};

/*===========================================================================================================================*/
// STRUCT
/*===========================================================================================================================*/

struct token
{
    TOKEN_TYPE type;
    int        token_beg;       // number of position in buffer where token starts
    int        token_line;      // number of line     in buffer where token places
    union
    {
        ASM_CMD instruction;    // .type = INSTRUCTION
        REGISTER    reg_num;    // .type = REG_NAME
        int         int_num;    // .type = INT_NUM
        char            key;    // .type = KEY_CHAR
        int       token_len;    // .type = UNDEF_TOKEN
    }
    value;

};

struct source
{
    struct
    {
        const char *data;       // source code from file
        int          pos;       // current position in .data
        int         line;       // current line     in .data
        int         size;       // size of .data
    }
    buff;

    struct
    {
        token      *data;       // array with tokens
        int          pos;       // current free place for new token
        char  *cur_token;       // current word to determine the next token
    }
    lexis;
};


/*===========================================================================================================================*/
// SOURCE_CTOR_DTOR
/*===========================================================================================================================*/

source *new_source        (                    const char *source_file);
bool    source_buff_ctor  (source *const code, const char *source_file);
bool    source_lexis_ctor (source *const code);
void    source_dtor       (source *const code);

/*===========================================================================================================================*/
// LEXICAL_ANALYSIS
/*===========================================================================================================================*/

void     lexical_analyzer  (source *const code);
int      get_another_token (source *const code);
bool     get_int_num       (const char *cur_token, int *const ret = nullptr);
ASM_CMD  get_asm_cmd       (const char *cur_token);
REGISTER get_reg_name      (const char *cur_token, const int token_len);
bool     is_key_char       (const char char_to_check);
bool     is_comment        (source *const code);

/*===========================================================================================================================*/
// TOKEN_CTOR
/*===========================================================================================================================*/

void create_key_char_token    (source *const code);
void create_reg_token         (source *const code, const int token_beg, const int token_len);
void create_int_token         (source *const code, const int token_beg);
void create_undef_token       (source *const code, const int token_beg, const int token_len);
void create_instruction_token (source *const code, const int token_beg, const ASM_CMD asm_cmd);

/*===========================================================================================================================*/
// SKIP_SOURCE
/*===========================================================================================================================*/

void skip_source_line   (source *const code);
void skip_source_spaces (source *const code);

/*===========================================================================================================================*/
// DUMP
/*===========================================================================================================================*/

void lexis_graphviz_dump     (const source *const code);
void do_lexis_graphviz_dump  (const source *const code, FILE *const stream);
void graphviz_dump_edge      (                          FILE *const stream, const int from, const int to);
void graphviz_dump_token     (const source *const code, FILE *const stream, const int dumping_pos);
void graphviz_describe_token (const source *const code, FILE *const stream, const int dumping_pos, const GRAPHVIZ_COLOR     color,
                                                                                                   const GRAPHVIZ_COLOR fillcolor);
void get_token_dump_message  (const token  *const cur_token, char *const token_message);
void system_graphviz_dump    (char *const dump_txt, char *const dump_png);

#endif //ASSEMBLER