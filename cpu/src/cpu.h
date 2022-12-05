#ifndef CPU
#define CPU

typedef int cpu_type;
const   int RAM_SIZE   = 10000;
const   int REG_NUMBER = 8;

struct cpu_cmd  // struct cpu_cmd to store information about cpu commands
{
    void *data; // array with commands
    int   size; // size of .data
    int     pc; // current position in .data
};

struct source_cmd           // struct source_cmd to store information about source file
{
    const char *code;       // array with source code
    int         code_size;  // size of .code
    int         code_pos;   // current position in .code
    int         code_line;  // current line in .code
};

struct cpu
{
    cpu_type  ram[RAM_SIZE];
    int       reg[REG_NUMBER + 1];

    stack     stk_data;
    stack     stk_calls;
};

enum REGISTER
{
    ERR_REG ,

    RAX     ,
    RBX     ,
    RCX     ,
    RDX     ,
    REX     ,
    RFX     ,
    RGX     ,
    RHX     ,
};

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

void add_cpu_cmd    (cpu_cmd    *const  cmd_store, const void *cmd,       const size_t    cmd_size);
void cpu_cmd_ctor   (cpu_cmd    *const  cmd_store,                        const int     store_size);
void cpu_cmd_dtor   (cpu_cmd    *const  cmd_store                                                 );
void source_cmd_ctor(source_cmd *const code_store, const void *code_buff, const int code_buff_size);

#endif //CPU