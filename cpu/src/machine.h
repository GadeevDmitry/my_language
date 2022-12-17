#ifndef MACHINE
#define MACHINE

#include "cpu.h"
#include "../../lib/stack/stack.h"

/*===========================================================================================================================*/
// DSL
/*===========================================================================================================================*/

#define _CALL_STACK(computer) computer->call_stack
#define _DATA_STACK(computer) computer->data_stack
#define        _RAM(computer) computer->ram
#define        _CPU(computer) computer->cpu
#define        _REG(computer) computer->reg

#define $call_stack _CALL_STACK(computer)
#define $data_stack _DATA_STACK(computer)
#define $ram               _RAM(computer)
#define $cpu               _CPU(computer)
#define $reg               _REG(computer)

/*===========================================================================================================================*/
// CONST
/*===========================================================================================================================*/

const int RAM_SIZE = 10000;

/*===========================================================================================================================*/
// STRUCT
/*===========================================================================================================================*/

struct machine
{
    stack    call_stack;            // стек вызовов
    stack    data_stack;            // стек с данными
    executer cpu;                   // структура, содержащая инструкции и параметры
    cpu_type ram[RAM_SIZE];         // оперативка
    int      reg[REG_NUMBER + 1];   // регистры
};

/*===========================================================================================================================*/
// EXECUTE
/*===========================================================================================================================*/

bool execute                   (machine *const computer);

bool execute_param             (machine *const computer, const unsigned char cmd);
bool execute_push              (machine *const computer, const unsigned char cmd);
bool execute_pop               (machine *const computer, const unsigned char cmd);
bool executer_pull_reg_int     (machine *const computer, const unsigned char cmd, bool     *const mem_arg,
                                                                                  REGISTER *const reg_arg,
                                                                                  int      *const cpu_arg);
bool executer_pull_reg_dbl     (machine *const computer, const unsigned char cmd, bool     *const mem_arg,
                                                                                  REGISTER *const reg_arg,
                                                                                  double   *const cpu_arg);
bool execute_call              (machine *const computer);
bool execute_jump              (machine *const computer, const unsigned char cmd);
bool executer_pull_label       (machine *const computer, int *const label_pc);

bool execute_in                (machine *const computer);
bool execute_out               (machine *const computer);
bool execute_ret               (machine *const computer);
bool execute_add               (machine *const computer);
bool execute_sub               (machine *const computer);
bool execute_mul               (machine *const computer);
bool execute_div               (machine *const computer);
bool execute_pow               (machine *const computer);

/*===========================================================================================================================*/
// MACHINE_CTOR_DTOR
/*===========================================================================================================================*/

bool machine_ctor (machine *const computer, const char *execute_file);
void machine_dtor (machine *const computer);

#endif //MACHINE