#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "../../lib/logs/log.h"
#include "../../lib/read_write/read_write.h"
#include "../../lib/algorithm/algorithm.h"
#include "../../lib/stack/stack.h"


#include "../../lib/label/label.h"
#include "cpu.h"
#include "terminal_colors.h"

typedef int cpu_type;

const int CMD_SIZE = 10;

enum ASM_CMD
{
    HLT     , //  0

    IN      ,
    OUT     ,

    PUSH    , //  1
    POP     , //  2

    JMP     , //  3
    JA      , //  4
    JAE     , //  5
    JB      , //  6
    JBE     , //  7
    JE      , //  8
    JNE     , //  9

    CALL    , // 10
    RET     , // 11

    ADD     , // 12
    SUB     , // 13
    MUL     , // 14
    DIV     , // 15
    POW     , // 16

    UNDEF   , // 17
};

enum ASM_CMD_PARAM      //    |   1 bit   |   1 bit   |   1 bit   |         4 bit         |
{                       //----------------+-----------+-----------+-----------------------+----
    PARAM_INT = 5   ,   //    | PARAM_MEM | PARAM_REG | PARAM_INT |        ASM_CMD        |
    PARAM_REG = 6   ,   //----------------+-----------+-----------+-----------------------+----
    PARAM_MEM = 7   ,
};

/*_________________________________PARSERS_________________________________*/

ASM_CMD         define_cmd      (source_cmd *const code_store);
bool            parse_general   (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, int asm_num);

bool            parse_operators (cpu_cmd    *const  cmd_store, ASM_CMD cmd_asm);
bool            parse_call      (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, int asm_num);
bool            parse_ret       (                              cpu_cmd *const cmd_store                        );

bool            parse_jump      (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm, int asm_num);
bool            parse_label     (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store);

bool            parse_push      (source_cmd *const code_store, cpu_cmd *const cmd_store);
bool            parse_pop       (source_cmd *const code_store, cpu_cmd *const cmd_store);

unsigned char   parse_expretion (source_cmd *const code_store, int      *const int_arg, REGISTER *const reg_arg);
bool            parse_int       (source_cmd *const code_store, int      *const ret);
bool            parse_reg       (source_cmd *const code_store, REGISTER *const ret);

/*_________________________________TRANSLATORS_________________________________*/

bool cpu_operators  (                              cpu_cmd *const cmd_store,                         ASM_CMD cmd_asm);
bool cpu_call_jump  (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm, int asm_num);
bool cpu_ret        (                              cpu_cmd *const cmd_store                                         );

bool cpu_expretion  (source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm);
bool cpu_reg_arg    (source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm);
bool cpu_mem_arg    (source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm);

/*____________________________________OTHER____________________________________*/

bool assembler              (const char *assembler_buff, const int buff_size, cpu_cmd *const cmd_store);

void get_source_cmd         (char *const buff, source_cmd *const code_store);
bool is_cmd_split           (const char  check);
void skip_source_undef_cmd  (source_cmd *const code_store);
void skip_source_line       (source_cmd *const code_store);
void skip_source_word       (source_cmd *const code_store);
void skip_source_spaces     (source_cmd *const code_store);

/*_____________________________________________________________________________*/

int main(const int argc, const char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "you should give two parameters: name of file to compile and name of file to put the cpu code in\n");
        return 0;
    }

    int buff_size = 0;
    char *assembler_buff = (char *) read_file(argv[1], &buff_size);
    if   (assembler_buff == nullptr)
    {
        fprintf(stderr, "can't open \"%s\"\n", argv[1]);
        return 0;
    }

    FILE *execute_file = fopen(argv[2], "w");
    if   (execute_file == nullptr)
    {
        log_free(assembler_buff);
        fprintf(stderr, "can't open \"%s\"\n", argv[2]);
        return 0;
    }

    cpu_cmd cmd_store = {};
    cpu_cmd_ctor(&cmd_store, buff_size * (int) sizeof(cpu_type));

    if (assembler(assembler_buff, buff_size, &cmd_store))
    {
        fwrite(cmd_store.data, sizeof(char), (size_t) cmd_store.pc + 1, execute_file);
        fclose(execute_file);
        
        log_free    (assembler_buff);
        cpu_cmd_dtor(&cmd_store);

        fprintf(stderr, TERMINAL_GREEN "compile success\n" TERMINAL_CANCEL);
        return 0;
    }

    fclose      (execute_file);
    log_free    (assembler_buff);
    cpu_cmd_dtor(&cmd_store);

    fprintf(stderr, TERMINAL_RED "compile failed\n" TERMINAL_CANCEL);
}

/*_________________________________RECURSIVE_DESCENT_________________________________*/

// general      ::= {operator | call | ret | jump | label | push | pop}* '\0'

// operator     ::= "add" | "sub" | "mul" | "div" | "hlt"

// call         ::= "call" name
// ret          ::= "ret"
// jump         ::= {"jmp" | "ja" | "jae" | "jb" | "jbe" | "je" | "jne"} name

// label        ::= name ':'
// name         ::= ['A'-'Z', 'a'-'z', '_', '0'-'9']+

// push         ::= "push" push_arg
// push_arg     ::= '[' expretion ']' | expretion

// pop          ::= "pop" pop_arg
// pop_arg      ::= "void" | reg | '[' expretion ']'

// expretion    ::= int {'+' reg}? | reg {'+' int}?
// int          ::= ['0'-'9']+
// reg          ::= 'r'['a'-'h']'x'

/*___________________________________________________________________________________*/

bool assembler(const char *assembler_buff, const int buff_size, cpu_cmd *const cmd_store)
{
    assert(assembler_buff != nullptr);
    assert(buff_size > 0);

    source_cmd code_store = {};
    source_cmd_ctor(&code_store, assembler_buff, buff_size);

    label tag_store = {};
    label_ctor(&tag_store);

    int asm_num = 0;
    if (!parse_general(&code_store, cmd_store, &tag_store, asm_num++)) { label_dtor(&tag_store); return false; }
    
    code_store.code_line = 1;
    code_store.code_pos  = 0;

    cmd_store->pc = 0;

    if (!parse_general(&code_store, cmd_store, &tag_store, asm_num++)) { label_dtor(&tag_store); return false; }

    label_dtor(&tag_store);
    return true;
}

/*_________________________________PARSERS_________________________________*/

#include "dsl.h"

bool parse_general(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, int asm_num)
{
    log_header(__PRETTY_FUNCTION__);

    log_message("\"\n");
    for (int i = 0; i < code_size; ++i) log_message("%c", code[i]);
    log_message("\"\n\n");

    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    bool err = false;
    while (code_pos < code_size)
    {
        if (code[code_pos] == '\0') break;
        if (code[code_pos] ==  '#') { skip_source_line(code_store); continue; }

        ASM_CMD cur_cmd     = define_cmd(code_store);
        bool    cur_status  = true;

        log_message("line %d. cur_cmd = %d\n\n", code_line, cur_cmd);

        switch (cur_cmd)
        {
            case PUSH: cur_status = !parse_push(code_store, cmd_store); break;
            case POP : cur_status = !parse_pop (code_store, cmd_store); break;
            
            case  JA : case JB : case JE :
            case  JAE: case JBE: case JNE:
            case  JMP: cur_status = !parse_jump(code_store, cmd_store, tag_store, cur_cmd, asm_num); break;

            case CALL: cur_status = !parse_call(code_store, cmd_store, tag_store, asm_num); break;
            case RET : cur_status = !parse_ret (cmd_store);                        break;

            case IN  : case OUT:
            case ADD : case SUB:
            case MUL : case DIV:
            case POW :
            case HLT : cur_status = !parse_operators(cmd_store, cur_cmd); break;

            case UNDEF:cur_status = !parse_label(code_store, cmd_store, tag_store); break;

            default: assert(false && "default case in parse_general() switch"); break;
        }

        err = (err == true) ? true : cur_status;
        skip_source_spaces(code_store);
    }

    log_message("err = %d\n", err);
    log_end_header();
    return err == false;
}

ASM_CMD define_cmd(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    skip_source_spaces(code_store);

    int code_pos_before = code_pos;
    log_message("code_pos_before = %d\n", code_pos_before);

    char cmd[CMD_SIZE] = {};
    get_source_cmd(cmd, code_store);

    if (!strcasecmp("hlt" , cmd)) return HLT ;

    if (!strcasecmp("in"  , cmd)) return IN  ;
    if (!strcasecmp("out" , cmd)) return OUT ;

    if (!strcasecmp("push", cmd)) return PUSH;
    if (!strcasecmp("pop" , cmd)) return POP ;
    
    if (!strcasecmp("jmp" , cmd)) return JMP ;
    if (!strcasecmp("ja"  , cmd)) return JA  ;
    if (!strcasecmp("jae" , cmd)) return JAE ; 
    if (!strcasecmp("jb"  , cmd)) return JB  ;
    if (!strcasecmp("jbe" , cmd)) return JBE ; 
    if (!strcasecmp("je"  , cmd)) return JE  ;
    if (!strcasecmp("jne" , cmd)) return JNE ;

    if (!strcasecmp("call", cmd)) return CALL;
    if (!strcasecmp("ret" , cmd)) return RET ;

    if (!strcasecmp("add" , cmd)) return ADD ;
    if (!strcasecmp("sub" , cmd)) return SUB ;
    if (!strcasecmp("mul" , cmd)) return MUL ;
    if (!strcasecmp("div" , cmd)) return DIV ;
    if (!strcasecmp("pow" , cmd)) return POW ;

    code_pos = code_pos_before;
    return UNDEF;
}

bool parse_operators(cpu_cmd *const cmd_store, ASM_CMD cmd_asm)
{
    assert(cmd_store != nullptr);

    return cpu_operators(cmd_store, cmd_asm);
}

bool parse_call(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, int asm_num)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    return cpu_call_jump(code_store, cmd_store, tag_store, CALL, asm_num);
}

bool parse_ret(cpu_cmd *const cmd_store)
{
    assert(cmd_store  != nullptr);

    return cpu_ret(cmd_store);
}

bool parse_jump(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm, int asm_num)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    return cpu_call_jump(code_store, cmd_store, tag_store, cmd_asm, asm_num);
}

bool parse_label(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    skip_source_spaces(code_store);
    int code_pos_before = code_pos;
    skip_source_undef_cmd(code_store);

    if (code_pos < code_size && code[code_pos] == ':') ++code_pos;
    else
    {
        fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined command or skipped \':\' in name of tag\n", code_line);
        return false;
    }

    int find_pc = tag_find(tag_store, code + code_pos_before);
    if (find_pc == -1)
    {
        label_push(tag_store, code + code_pos_before, cmd_store->pc);
        log_message("new tag: \"");
        for (int i = 0; i < tag_store->data[tag_store->size - 1].name_size; ++i)
        {
            log_message("%c", tag_store->data[tag_store->size - 1].name[i]);
        }
        log_message("\"\n");
        return true;
    }
    if (find_pc == cmd_store->pc) return true;

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "redefined tag\n", code_line);
    return false;
}

bool parse_push(source_cmd *const code_store, cpu_cmd *const cmd_store)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    skip_source_spaces(code_store);

    if (code_pos < code_size && code[code_pos] == '[')
    {
        ++code_pos;
        return cpu_mem_arg(code_store, cmd_store, PUSH);
    }
    if (cpu_expretion(code_store, cmd_store, PUSH)) return true;

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined push-argument\n", code_line);
    return false;
}

bool parse_pop(source_cmd *const code_store, cpu_cmd *const cmd_store)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    skip_source_spaces(code_store);

    if (code_pos < code_size && code[code_pos] == '[')
    {
        ++code_pos;
        return cpu_mem_arg(code_store, cmd_store, POP);
    }
    if (cpu_reg_arg(code_store, cmd_store, POP)) return true;

    int code_pos_before = code_pos;

    char arg_void[CMD_SIZE] = {};
    get_source_cmd(arg_void, code_store);

    if (strcasecmp(arg_void, "void"))
    {
        unsigned char cmd = POP;
        add_cpu_cmd(cmd_store, &cmd, sizeof(unsigned char));
        
        return true;
    }

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined pop-argument\n", code_line);
    
    code_pos = code_pos_before;
    skip_source_word(code_store);
    return false;
}

unsigned char parse_expretion(source_cmd *const code_store, int *const int_arg, REGISTER *const reg_arg)
{
    assert(code_store != nullptr);
    assert(int_arg    != nullptr);
    assert(reg_arg    != nullptr);

    unsigned char ret = 0;

    if (parse_int(code_store, int_arg))
    {
        ret = ret | (1 << PARAM_INT);
        skip_source_spaces(code_store);

        if (code_pos < code_size && code[code_pos] == '+')
        {
            ++code_pos;
            if (parse_reg(code_store, reg_arg)) return ret = ret | (1 << PARAM_REG);
            else
            {
                fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined name of register\n", code_line);
                skip_source_word(code_store);
                return 0;
            }
        }
        return ret;
    }
    else if (parse_reg(code_store, reg_arg))
    {
        ret = ret | (1 << PARAM_REG);
        skip_source_spaces(code_store);

        if (code_pos < code_size && code[code_pos] == '+')
        {
            ++code_pos;
            if (parse_int(code_store, int_arg)) return ret = ret | (1 << PARAM_INT);
            else
            {
                fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined int\n", code_line);
                skip_source_word(code_store);
                return 0;
            }
        }
        return ret;
    }

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined name of register or int\n", code_line);
    skip_source_word(code_store);
    return 0;
}

bool parse_int(source_cmd *const code_store, int *const ret)
{
    assert(code_store != nullptr);
    assert(ret        != nullptr);

    skip_source_spaces(code_store);

    if (code_pos >= code_size) return false;

    int int_len = 0;
    if (sscanf(code + code_pos, "%d%n", ret, &int_len) == 0) return false;
    code_pos += int_len;

    return true;
}

bool parse_reg(source_cmd *const code_store, REGISTER *const ret)
{
    assert(code_store != nullptr);
    assert(ret        != nullptr);
    
    skip_source_spaces(code_store);
    bool err = 0;

    if (code_pos + 3 > code_size) return false;

    if (tolower(code[code_pos    ]) == 'r' &&
        tolower(code[code_pos + 2]) == 'x') *ret = (REGISTER) (code[code_pos + 1] - 'a' + 1);
    else                                     err = true;

    if (err == true || *ret <= 0 || *ret > 8) { *ret = ERR_REG; return false; }

    code_pos += 3;
    return true;
}

/*_________________________________TRANSLATORS_________________________________*/

bool cpu_operators(cpu_cmd *const cmd_store, ASM_CMD cmd_asm)
{
    assert(cmd_store != nullptr);

    unsigned char cmd = cmd_asm;
    add_cpu_cmd(cmd_store, &cmd, sizeof(unsigned char));
    return true;
}

bool cpu_call_jump(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm, int asm_num)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    skip_source_spaces(code_store);

    int tag_address  = tag_find(tag_store, code + code_pos);

    unsigned char cmd = cmd_asm;
    add_cpu_cmd(cmd_store, &cmd,         sizeof(unsigned char));
    add_cpu_cmd(cmd_store, &tag_address, sizeof(int));

    if (tag_address != -1 || asm_num == 0)
    {
        skip_source_word(code_store);
        return true;
    }

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined tag\n", code_line);
    skip_source_word(code_store);
    return false;
}

bool cpu_ret(cpu_cmd *const cmd_store)
{
    assert(cmd_store != nullptr);

    unsigned char cmd = RET;
    add_cpu_cmd(cmd_store, &cmd, sizeof(unsigned char));
    return true;
}

bool cpu_expretion(source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    unsigned char cmd     = cmd_asm;
    int           int_arg = 0;
    REGISTER      reg_arg = ERR_REG;

    unsigned char cmd_param = parse_expretion(code_store, &int_arg, &reg_arg);
    cmd = cmd  |  cmd_param;

    add_cpu_cmd(cmd_store, &cmd, sizeof(unsigned char));
    if (cmd_param & (1 << PARAM_INT)) add_cpu_cmd(cmd_store, &int_arg, sizeof(int));
    if (cmd_param & (1 << PARAM_REG)) add_cpu_cmd(cmd_store, &reg_arg, sizeof(REGISTER));

    return cmd_param != 0;
}

bool cpu_reg_arg(source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    unsigned char cmd     = (unsigned char) cmd_asm | (1 << PARAM_REG);
    REGISTER      reg_arg = ERR_REG;
    parse_reg(code_store, &reg_arg);

    if (reg_arg == ERR_REG) return false;

    add_cpu_cmd(cmd_store, &cmd    , sizeof(unsigned char));
    add_cpu_cmd(cmd_store, &reg_arg, sizeof(REGISTER));
    return true;
}

bool cpu_mem_arg(source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    unsigned char cmd     = (unsigned char) cmd_asm | (1 << PARAM_MEM);
    int           int_arg = 0;
    REGISTER      reg_arg = ERR_REG;

    unsigned char cmd_param = parse_expretion(code_store, &int_arg, &reg_arg);
    cmd = cmd  |  cmd_param;

    add_cpu_cmd(cmd_store, &cmd, sizeof(unsigned char));
    if (cmd_param & (1 << PARAM_INT)) add_cpu_cmd(cmd_store, &int_arg, sizeof(int));
    if (cmd_param & (1 << PARAM_REG)) add_cpu_cmd(cmd_store, &reg_arg, sizeof(REGISTER));

    skip_source_spaces(code_store);
    
    if (code_pos < code_size && code[code_pos] == ']') ++code_pos; 
    else
    {
        fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "skipped \']\'\n", code_line);
        return false;
    }
    return cmd_param != 0;
}

/*____________________________________OTHER____________________________________*/

void get_source_cmd(char *const buff, source_cmd *const code_store)
{
    assert(buff       != nullptr);
    assert(code_store != nullptr);

    skip_source_spaces(code_store);

    int  i = 0;
    for (i = 0; i < CMD_SIZE - 1 && code_pos < code_size; ++i)
    {
        if (is_cmd_split(code[code_pos])) break;
        buff[i] =   code[code_pos++];
    }
    buff[i] = '\0';
}

bool is_cmd_split(const char check)
{
    if (isspace(check)) return true;
    if (check == ':')   return true;
    if (check == '#')   return true;
    if (check == '\0')  return true;

    return false;
}

void skip_source_word(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    skip_source_spaces(code_store);

    while (code_pos < code_size && !isspace(code[code_pos]) && code[code_pos] != '#') ++code_pos;
}

void skip_source_undef_cmd(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    skip_source_spaces(code_store);
    while(code_pos < code_size && !is_cmd_split(code[code_pos])) ++code_pos;
    skip_source_spaces(code_store);
}

void skip_source_line(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    while (code_pos < code_size && code[code_pos] != '\n') ++code_pos;
}

void skip_source_spaces(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    while (code_pos < code_size && isspace(code[code_pos]))
    {
        if (code[code_pos] == '\n') code_line++;
        code_pos++;
    }
}