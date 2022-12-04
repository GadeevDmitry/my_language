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
#include "dsl.h"
#include "terminal_colors.h"

typedef int cpu_type;

const int CMD_SIZE = 10;

enum ASM_CMD
{
    HLT     ,

    PUSH    ,
    POP     ,

    JMP     ,
    JA      ,
    JAE     ,
    JB      ,
    JBE     ,
    JE      ,
    JNE     ,

    CALL    ,
    RET     ,

    ADD     ,
    SUB     ,
    MUL     ,
    DIV     ,
    POW     ,

    UNDEF   ,
};

enum ASM_CMD_PARAM      //    |   1 bit   |   1 bit   |   1 bit   |         4 bit         |
{                       //----------------+-----------+-----------+-----------------------+----
    PARAM_INT = 5   ,   //    | PARAM_MEM | PARAM_REG | PARAM_INT |        ASM_CMD        |
    PARAM_REG = 6   ,   //----------------+-----------+-----------+-----------------------+----
    PARAM_MEM = 7   ,
};

/*_________________________________PARSERS_________________________________*/

ASM_CMD         define_cmd      (source_cmd *const code_store);
bool            parse_general   (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store);

bool            parse_operators (cpu_cmd    *const  cmd_store, ASM_CMD cmd_asm);
bool            parse_call      (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store);
bool            parse_ret       (                              cpu_cmd *const cmd_store                        );

bool            parse_jump      (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm);
bool            parse_label     (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store);

bool            parse_push      (source_cmd *const code_store, cpu_cmd *const cmd_store);
bool            parse_pop       (source_cmd *const code_store, cpu_cmd *const cmd_store);

unsigned char   parse_expretion (source_cmd *const code_store, int      *const int_arg, REGISTER *const reg_arg);
bool            parse_int       (source_cmd *const code_store, int      *const ret);
bool            parse_reg       (source_cmd *const code_store, REGISTER *const ret);

/*_________________________________TRANSLATORS_________________________________*/

bool cpu_operators  (                              cpu_cmd *const cmd_store,                         ASM_CMD cmd_asm);
bool cpu_call_jump  (source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm);
bool cpu_ret        (                              cpu_cmd *const cmd_store                                         );

bool cpu_expretion  (source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm);
bool cpu_reg_arg    (source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm);
bool cpu_mem_arg    (source_cmd *const code_store, cpu_cmd *const cmd_store, ASM_CMD cmd_asm);

/*____________________________________OTHER____________________________________*/

bool assembler              (const char *assembler_buff, const int buff_size, cpu_cmd *const cmd_store);

void get_source_word        (char *const buff, source_cmd *const code_store);
void skip_source_undef_cmd  (source_cmd *const code_store);
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

    FILE *execute_file = fopen(argv[2], "r");
    if   (execute_file == nullptr)
    {
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

    if (!parse_general(&code_store, cmd_store, &tag_store)) {label_dtor(&tag_store); return false; }

    label_dtor(&tag_store);
    return true;
}

/*_________________________________PARSERS_________________________________*/

bool parse_general(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    bool err = false;
    while (code_pos < code_size)
    {
        if (code[code_pos] == '\0') break;

        ASM_CMD cur_cmd = define_cmd(code_store);
        switch (cur_cmd)
        {
            case PUSH: err = (err == true) ? err : parse_push(code_store, cmd_store); break;
            case POP : err = (err == true) ? err : parse_pop (code_store, cmd_store); break;
            
            case  JA : case JB : case JE :
            case  JAE: case JBE: case JNE:
            case  JMP: err = (err == true) ? err : parse_jump(code_store, cmd_store, tag_store, cur_cmd); break;

            case CALL: err = (err == true) ? err : parse_call(code_store, cmd_store, tag_store); break;
            case RET : err = (err == true) ? err : parse_ret (cmd_store);                        break;

            case ADD : case SUB:
            case MUL : case DIV:
            case POW :
            case HLT : err = (err == true) ? err : parse_operators(cmd_store, cur_cmd); break;

            case UNDEF:err = (err == true) ? err : parse_label(code_store, cmd_store, tag_store); break;

            default: assert(false && "default case in parse_general() switch"); break;
        }
    }

    return err == false;
}

ASM_CMD define_cmd(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    skip_source_spaces(code_store);

    int code_pos_before = code_pos;
    char cmd[CMD_SIZE] = {};
    get_source_word(cmd, code_store);

    if (!strcasecmp("hlt" , code)) { return HLT ;}

    if (!strcasecmp("push", code)) { return PUSH; }
    if (!strcasecmp("pop" , code)) { return POP ; }
    
    if (!strcasecmp("jmp" , code)) { return JMP ; }
    if (!strcasecmp("ja"  , code)) { return JA  ; }
    if (!strcasecmp("jae" , code)) { return JAE ; }
    if (!strcasecmp("jb"  , code)) { return JB  ; }
    if (!strcasecmp("jbe" , code)) { return JBE ; }
    if (!strcasecmp("je"  , code)) { return JE  ; }
    if (!strcasecmp("jne" , code)) { return JNE ; }
    
    if (!strcasecmp("call", code)) { return CALL; }
    if (!strcasecmp("ret" , code)) { return RET ; }

    if (!strcasecmp("add" , code)) { return ADD ; }
    if (!strcasecmp("sub" , code)) { return SUB ; }
    if (!strcasecmp("mul" , code)) { return MUL ; }
    if (!strcasecmp("div" , code)) { return DIV ; }
    if (!strcasecmp("pow" , code)) { return POW ; }

    code_pos = code_pos_before;
    return UNDEF;
}

bool parse_operators(cpu_cmd *const cmd_store, ASM_CMD cmd_asm)
{
    assert(cmd_store != nullptr);

    return cpu_operators(cmd_store, cmd_asm);
}

bool parse_call(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    return cpu_call_jump(code_store, cmd_store, tag_store, CALL);
}

bool parse_ret(cpu_cmd *const cmd_store)
{
    assert(cmd_store  != nullptr);

    return cpu_ret(cmd_store);
}

bool parse_jump(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);
    assert(tag_store  != nullptr);

    return cpu_call_jump(code_store, cmd_store, tag_store, cmd_asm);
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

    if (tag_find(tag_store, code + code_pos_before) != -1)
    {
        fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "redefined tag\n", code_line);
        return false;
    }

    label_push(tag_store, code + code_pos, cmd_store->pc);
    return true;
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

    char arg_void[CMD_SIZE] = {};
    get_source_word(arg_void, code_store);

    if (strcasecmp(arg_void, "void"))
    {
        unsigned char cmd = POP;
        add_cpu_cmd(cmd_store, &cmd, sizeof(unsigned char));
        
        return true;
    }

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined pop-argument\n", code_line);
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
                return 0;
            }
        }
        return ret;
    }

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined name of register or int\n", code_line);
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

bool cpu_call_jump(source_cmd *const code_store, cpu_cmd *const cmd_store, label *const tag_store, ASM_CMD cmd_asm)
{
    assert(code_store != nullptr);
    assert(cmd_store  != nullptr);

    skip_source_spaces(code_store);

    int tag_address  = tag_find(tag_store, code + code_pos);
    if (tag_address != -1)
    {
        unsigned char cmd = cmd_asm;
        add_cpu_cmd(cmd_store, &cmd,         sizeof(unsigned char));
        add_cpu_cmd(cmd_store, &tag_address, sizeof(int));

        return true;
    }

    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined tag\n", code_line);
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

void get_source_word(char *const buff, source_cmd *const code_store)
{
    assert(buff       != nullptr);
    assert(code_store != nullptr);

    skip_source_spaces(code_store);

    int  i = 0;
    for (i = 0; i < CMD_SIZE - 1 && code_pos < code_size; ++i)
    {
        if (isspace(code[code_pos]) || code[code_pos] == ':' || code[code_pos] == '\0') break;
        buff[i] =   code[code_pos++];
    }
    buff[i] = '\0';
}

void skip_source_undef_cmd(source_cmd *const code_store)
{
    assert(code_store != nullptr);

    skip_source_spaces(code_store);
    while(code_pos < code_size && !isspace(code[code_pos]) && code[code_pos] != ':' && code[code_pos] != '\0') ++code_pos;
    skip_source_spaces(code_store);
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