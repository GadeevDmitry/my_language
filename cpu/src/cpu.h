#ifndef CPU
#define CPU

/*===========================================================================================================================*/
// CONST
/*===========================================================================================================================*/

typedef   double cpu_type;

static const int REG_NUMBER     = 8;
static const int INT_REG_NUMBER = 4;

enum REGISTER   // регистры для хранения целых чисел
{
    ERR_REG ,

    RAX     ,   // double-type registers
    RBX     ,   //
    RCX     ,   //
    RDX     ,   //

    REX     ,   // int-type registers
    RFX     ,   //
    RGX     ,   //
    RHX     ,   //
};

enum ASM_CMD
{
    HLT             , //  0

    IN              , //  1
    OUT             , //  2

    PUSH            , //  3
    POP             , //  4

    JMP             , //  5
    JA              , //  6
    JAE             , //  7
    JB              , //  8
    JBE             , //  9
    JE              , // 10
    JNE             , // 11

    CALL            , // 12
    RET             , // 13

    ADD             , // 14
    SUB             , // 15
    MUL             , // 16
    DIV             , // 17
    POW             , // 18
    SQRT            , // 19
    SIN             , // 20

    UNDEF_ASM_CMD   , // 21
};

enum ASM_CMD_PARAM      //    |   1 bit   |   1 bit   |   1 bit   |         5 bit         |
{                       //----------------+-----------+-----------+-----------------------+----
    PARAM_NUM = 5   ,   //    | PARAM_MEM | PARAM_REG | PARAM_NUM |        ASM_CMD        |
    PARAM_REG = 6   ,   //----------------+-----------+-----------+-----------------------+----
    PARAM_MEM = 7   ,
};

/*===========================================================================================================================*/
// STRUCT
/*===========================================================================================================================*/

struct executer
{
    void *cmd;          // массив, содержащий инструкции и параметры исполнителя(бинарный код)
    int   capacity;     // емкость .cmd
    int   pc;           // program counter(он же размер .cmd)
};

/*===========================================================================================================================*/
// EXECUTER_CTOR_DTOR
/*===========================================================================================================================*/

void executer_ctor(executer *const cpu);
void executer_ctor(executer *const cpu, const int size);
bool executer_ctor(executer *const cpu, const char *execute_file);

void executer_dtor(executer *const cpu);

/*===========================================================================================================================*/
// EXTRA EXECUTER FUNCTION
/*===========================================================================================================================*/

void executer_add_cmd  (executer *const cpu, const void *const cmd, const size_t cmd_size);
void executer_pull_cmd (executer *const cpu, void *const   pull_in, const size_t pull_size);

/*===========================================================================================================================*/
// DEFINE_REGISTER_TYPE
/*===========================================================================================================================*/

bool is_dbl_reg (const REGISTER reg);
bool is_int_reg (const REGISTER reg);

#endif //CPU