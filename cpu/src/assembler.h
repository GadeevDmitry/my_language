#ifndef ASSEMBLER
#define ASSEMBLER

#include "cpu.h"
#include "label.h"

/*===========================================================================================================================*/
// DSL
/*===========================================================================================================================*/

#define buff_data code->buff.data
#define buff_line code->buff.line
#define buff_size code->buff.size
#define buff_pos  code->buff.pos

#define lexis_data      code->lexis.data
#define lexis_pos       code->lexis.pos
#define lexis_cur_token code->lexis.cur_token

/*===========================================================================================================================*/
// CONST
/*===========================================================================================================================*/

const char *LEXIS_GRAPHVIZ_HEADER = "digraph {\n"
                                    "splines=ortho\n"
                                    "node[shape=record, style=\"rounded, filled\", fontsize=8]\n";
const char *KEY_CHARS             = "[]#:+";
const int   REG_NAME_LEN          = 4; //including null-character in the end

enum TOKEN_TYPE
{
    INSTRUCTION ,
    REG_NAME    ,
    INT_NUM     ,
    DBL_NUM     ,
    KEY_CHAR    ,
    UNDEF_TOKEN ,
};

const char *TOKEN_TYPE_NAMES[] =
{
    "INSTRUCTION"   ,
    "REG_NAME"      ,
    "INT_NUM"       ,
    "DBL_NUM"       ,
    "KEY_CHAR"      ,
    "UNDEF_TOKEN"   ,
};

static const char *REGISTER_NAMES[] =
{
    "ERR_REG"   ,

    "RAX"       ,
    "RBX"       ,
    "RCX"       ,
    "RDX"       ,
    "REX"       ,
    "RFX"       ,
    "RGX"       ,
    "RHX"       ,
};

static const char *ASM_CMD_NAMES[] =
{
    "HLT"           ,

    "IN"            ,
    "OUT"           ,

    "PUSH"          ,
    "POP"           ,

    "JMP"           ,
    "JA"            ,
    "JAE"           ,
    "JB"            ,
    "JBE"           ,
    "JE"            ,
    "JNE"           ,

    "CALL"          ,
    "RET"           ,

    "ADD"           ,
    "SUB"           ,
    "MUL"           ,
    "DIV"           ,
    "POW"           ,

    "UNDEF_ASM_CMD" ,
};

/*===========================================================================================================================*/
// STRUCT
/*===========================================================================================================================*/

struct token
{
    TOKEN_TYPE type;
    int        token_beg;       // номер символа, с которого начинается токен
    int        token_line;      // номер строки, где находится токен
    union
    {
        ASM_CMD instruction;    // .type = INSTRUCTION,
        REGISTER    reg_num;    // .type = REG_NAME
        int         int_num;    // .type = INT_NUM
        double      dbl_num;    // .type = DBL_NUM
        char            key;    // .type = KEY_CHAR
        int       token_len;    // .type = UNDEF_TOKEN
    }
    value;

};

struct source
{
    struct
    {
        const char *data;       // буффер с исходным кодом
        int          pos;       // текущая позиция в .data
        int         line;       // текущая строка в .data
        int         size;       // размер .data
    }
    buff;

    struct
    {
        token      *data;       // массив с токенами
        int          pos;       // индекс первой свободной ячейки для нового токена (он же размер .data)
        char  *cur_token;       // текущее слово, для которого нужно определить токен
    }
    lexis;
};

struct translator
{
    source     *code;           // структура с исходным кодом
    label_store link;           // структура с метками
    executer     cpu;           // структура для хранения переведённых инструкций и параметров
};

/*===========================================================================================================================*/
// ASSEMBLER
/*===========================================================================================================================*/

bool             assembler(source *const code, translator *const my_asm);
bool          do_assembler(source *const code, translator *const my_asm, const int asm_num);

bool          translate_instruction       (translator *const my_asm, int *const token_cnt, const int asm_num);
bool          translate_no_parametres     (translator *const my_asm, int *const token_cnt);
bool          translate_push              (translator *const my_asm, int *const token_cnt);
bool          translate_pop               (translator *const my_asm, int *const token_cnt);
bool          translate_jump_call         (translator *const my_asm, int *const token_cnt, const int asm_num);

bool          translate_ram               (translator *const my_asm, int *const token_cnt, unsigned char cmd);
unsigned char translate_reg_int_expretion (translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                                           int           *const int_arg);
unsigned char translate_reg_dbl_expretion (translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                                           double        *const dbl_arg);
bool          translate_reg_plus_int      (translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                                           int           *const int_arg,
                                                                                           unsigned char *const cmd_param);
bool          translate_reg_plus_dbl      (translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                                           double        *const dbl_arg,
                                                                                           unsigned char *const cmd_param);
bool          translate_int_plus_reg      (translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                                           int           *const int_arg,
                                                                                           unsigned char *const cmd_param);
bool          translate_dbl_plus_reg      (translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                                           double        *const dbl_arg,
                                                                                           unsigned char *const cmd_param);
bool          translate_undef_token       (translator *const my_asm, int *const token_cnt);

bool          translate_reg_int           (translator *const my_asm,       unsigned char cmd, int *const token_cnt);
bool          translate_reg_dbl           (translator *const my_asm,       unsigned char cmd, int *const token_cnt);
void          executer_add_reg_int        (translator *const my_asm, const unsigned char cmd, const REGISTER reg_arg,
                                                                                              const int      int_arg);
void          executer_add_reg_dbl        (translator *const my_asm, const unsigned char cmd, const REGISTER reg_arg,
                                                                                              const double   dbl_arg);

/*===========================================================================================================================*/
// TRANSLATOR_CTOR_DTOR
/*===========================================================================================================================*/

void translator_ctor (translator *const my_asm, source *const code);
void translator_dtor (translator *const my_asm);

/*===========================================================================================================================*/
// EXTRA FUNCTION
/*===========================================================================================================================*/

int  get_undef_token_num     (const source     *const code);
bool still_inside_lexis_data (const translator *const my_asm, int *const token_cnt);
int  get_label_pc            (const translator *const my_asm, int *const token_cnt);

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
bool     get_int_num       (const char *cur_token, int    *const ret = nullptr);
bool     get_dbl_num       (const char *cur_token, double *const ret = nullptr);
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
void create_dbl_token         (source *const code, const int token_beg);
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

void label_text_dump         (const label_store *const link);
void lexis_text_dump         (const source      *const code);

void lexis_graphviz_dump     (const source *const code);
void do_lexis_graphviz_dump  (const source *const code, FILE *const stream);
void graphviz_dump_edge      (                          FILE *const stream, const int from, const int to);
void graphviz_dump_token     (const source *const code, FILE *const stream, const int dumping_pos);
void graphviz_describe_token (const source *const code, FILE *const stream, const int dumping_pos, const GRAPHVIZ_COLOR     color,
                                                                                                   const GRAPHVIZ_COLOR fillcolor);
void get_token_dump_message  (const token  *const cur_token, char *const token_message);
void system_graphviz_dump    (char *const dump_txt, char *const dump_png);

#endif //ASSEMBLER