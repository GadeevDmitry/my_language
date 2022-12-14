#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "../../lib/logs/log.h"
#include "../../lib/read_write/read_write.h"
#include "../../lib/algorithm/algorithm.h"

#include "terminal_colors.h"
#include "machine.h"

/*===========================================================================================================================*/
// MAIN
/*===========================================================================================================================*/

int main(const int argc, const char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "you should give execute file\n");
        return 0;
    }

    machine computer = {}; machine_ctor(&computer, argv[1]);

    if (execute(&computer)) fprintf(stderr, TERMINAL_GREEN "execute success\n" TERMINAL_CANCEL);
    else                    fprintf(stderr, TERMINAL_RED   "execute failed\n"  TERMINAL_CANCEL);
}

/*===========================================================================================================================*/
// EXECUTE
/*===========================================================================================================================*/

bool execute(machine *const computer)
{
    assert(computer != nullptr);

    unsigned char cur_cmd = 0;
    
    while ($cpu.pc < $cpu.capacity)
    {
        executer_pull_cmd(&$cpu, &cur_cmd, sizeof(unsigned char));

        switch(cur_cmd)
        {
            case HLT : machine_dtor(computer); return true;

            case IN  : if (execute_in   (computer) == false) { machine_dtor(computer); return false; } break;
            case OUT : if (execute_out  (computer) == false) { machine_dtor(computer); return false; } break;

            case RET : if (execute_ret  (computer) == false) { machine_dtor(computer); return false; } break;

            case ADD : if (execute_add  (computer) == false) { machine_dtor(computer); return false; } break;
            case SUB : if (execute_sub  (computer) == false) { machine_dtor(computer); return false; } break;
            case MUL : if (execute_mul  (computer) == false) { machine_dtor(computer); return false; } break;
            case DIV : if (execute_div  (computer) == false) { machine_dtor(computer); return false; } break;
            case POW : if (execute_pow  (computer) == false) { machine_dtor(computer); return false; } break;
            case SQRT: if (execute_sqrt (computer) == false) { machine_dtor(computer); return false; } break;
            case SIN : if (execute_sin  (computer) == false) { machine_dtor(computer); return false; } break;
            case COS : if (execute_cos  (computer) == false) { machine_dtor(computer); return false; } break;
            case LOG : if (execute_log  (computer) == false) { machine_dtor(computer); return false; } break;

            case CALL: if (execute_call (computer) == false) { machine_dtor(computer); return false; } break;
            case JMP :
            case JA  :
            case JAE :
            case JB  :
            case JBE :
            case JE  :
            case JNE : if (execute_jump (computer, cur_cmd) == false) { machine_dtor(computer); return false; } break;

            default  : if (execute_param(computer, cur_cmd) == false) { machine_dtor(computer); return false; } break;
        }
    }
    machine_dtor(computer);
    return true;
}

// checks if reg_arg is invalid
#define check_reg_arg(reg_arg, instruction_name)                                                                            \
  if (reg_arg <= 0 || reg_arg > REG_NUMBER)                                                                                 \
  {                                                                                                                         \
      fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "invalid register\n", #instruction_name);      \
      return false;                                                                                                         \
  }

// checks if ram_index is invalid
#define check_ram_index(ram_index, instruction_name)                                                                        \
  if (ram_index <  0 || ram_index >= RAM_SIZE)                                                                              \
  {                                                                                                                         \
      fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "invalid ram index\n", #instruction_name);     \
      return false;                                                                                                         \
  }

// checks if stack #stack_name is empty
#define check_empty(stack_name, instruction_name)                                                                           \
  if (stack_empty(&$##stack_name))                                                                                          \
  {                                                                                                                         \
      fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL #stack_name " is empty\n", #instruction_name); \
      return false;                                                                                                         \
  }

// checks if #pc pointed out of execute boundary
#define check_pc_inside(pc)                                                                                                 \
    if ((size_t) pc < 0 || (size_t) pc >= (size_t) $cpu.capacity)                                                           \
    {                                                                                                                       \
        fprintf(stderr, TERMINAL_RED "RUNTIME ERROR: " TERMINAL_CANCEL "label pointed out of execute boundary\n");          \
        return false;                                                                                                       \
    }

// checks if pc will go out of boundary after pulling cmd with size #pull_size
#define check_inside(pull_size)                                                                                             \
    if ((size_t) $cpu.pc + pull_size > (size_t) $cpu.capacity)                                                              \
    {                                                                                                                       \
        fprintf(stderr, TERMINAL_RED "RUNTIME ERROR: " TERMINAL_CANCEL "no parameter at the end of file\n");                \
        return false;                                                                                                       \
    }

bool execute_param(machine *const computer, const unsigned char cmd)
{
    assert(computer != nullptr);

    switch (cmd & 31) // 5 bit for cmd_asm
    {
        case PUSH: return execute_push(computer, cmd);
        case POP : return execute_pop (computer, cmd);

        default  : fprintf(stderr, TERMINAL_RED "RUNTIME ERROR: " TERMINAL_CANCEL "undefined command\n");
                   log_error("default case in execute_param(): cmd=%d(%d)\n", cmd, __LINE__);
                   break;
    }
    return false;
}

bool execute_push(machine *const computer, const unsigned char cmd)
{
    assert(computer != nullptr);

    bool     mem_arg = false;
    REGISTER reg_arg = ERR_REG;
    double   dbl_arg = 0;
    int      int_arg = 0;

    if (cmd & (1 << PARAM_MEM))
    {
        if  (!executer_pull_reg_int(computer, cmd, &mem_arg, &reg_arg, &int_arg)) return false;
    }
    else if (!executer_pull_reg_dbl(computer, cmd, &mem_arg, &reg_arg, &dbl_arg)) return false;

    int    int_param = 0;
    double dbl_param = 0;
    if (mem_arg)
    {
        if (cmd & (1 << PARAM_NUM)) int_param += int_arg;
        if (cmd & (1 << PARAM_REG))
        {
            check_reg_arg(reg_arg, PUSH);
            if (!is_int_reg(reg_arg))
            {
                fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL, "expected int register, but it is double\n");
                return false;
            }
            int_param += $int_reg[reg_arg];
        }
        check_ram_index(int_param, PUSH);
        stack_push(&$data_stack, &$ram[int_param]);
        return true;
    }
    if (cmd & (1 << PARAM_NUM)) dbl_param += dbl_arg;
    if (cmd & (1 << PARAM_REG))
    {
        check_reg_arg(reg_arg, PUSH);
        if (is_int_reg(reg_arg)) dbl_param += $int_reg[reg_arg];
        else                     dbl_param += $dbl_reg[reg_arg];
    }
    stack_push(&$data_stack, &dbl_param);
    return true;
}

bool execute_pop(machine *const computer, const unsigned char cmd)
{
    assert(computer != nullptr);

    check_empty(data_stack, POP);

    bool     mem_arg = false;
    REGISTER reg_arg = ERR_REG;
    int      cpu_arg = 0;

    if (!(cmd & (1 << PARAM_MEM)) && (cmd & (1 << PARAM_NUM)))
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "rvalue as a pop-argument\n", "POP");
        return false;
    }
    if (!executer_pull_reg_int(computer, cmd, &mem_arg, &reg_arg, &cpu_arg)) return false;

    int num_param = 0;
    if (cmd & (1 << PARAM_MEM))
    {
        if (cmd & (1 << PARAM_REG))
        {
            check_reg_arg(reg_arg, POP);
            if (!is_int_reg(reg_arg))   // ???????????? ?????????????????????????? ???????????????? ?????????? ???????? ???????????????? ?? RAM
            {
                fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL, "expected int register, but it is double\n");
                return false;
            }
            num_param += $int_reg[reg_arg];
        }
        if (cmd & (1 << PARAM_NUM)) num_param += cpu_arg;

        check_ram_index(num_param, POP);
        $ram[num_param] = *(cpu_type *) stack_pop(&$data_stack);
        return true;
    }
    if (cmd & (1 << PARAM_REG))
    {
        check_reg_arg (reg_arg, POP);
        if (is_int_reg(reg_arg)) $int_reg[reg_arg] = (int) *(cpu_type *) stack_pop(&$data_stack); // ???????????? ???????????????????????????? ?????????? ?? ?????????????????????????? ??????????????
        else                     $dbl_reg[reg_arg] =       *(cpu_type *) stack_pop(&$data_stack);
        return true;
    }
    stack_pop(&$data_stack); // void case
    return true;
}

bool executer_pull_reg_int(machine *const computer, const unsigned char cmd, bool     *const mem_arg,
                                                                             REGISTER *const reg_arg,
                                                                             int      *const cpu_arg)
{
    assert(computer != nullptr);
    assert(mem_arg  != nullptr);
    assert(reg_arg  != nullptr);
    assert(cpu_arg  != nullptr);

    *mem_arg = cmd & (1 << PARAM_MEM);

    bool is_reg_arg = cmd & (1 << PARAM_REG);
    bool is_cpu_arg = cmd & (1 << PARAM_NUM);
    
    if (is_reg_arg)
    {
        check_inside(sizeof(REGISTER));
        executer_pull_cmd(&$cpu, reg_arg, sizeof(REGISTER));
    }
    if (is_cpu_arg)
    {
        check_inside(sizeof(cpu_type));
        executer_pull_cmd(&$cpu, cpu_arg, sizeof(int));
    }
    return true;
}

bool executer_pull_reg_dbl(machine *const computer, const unsigned char cmd, bool     *const mem_arg,
                                                                             REGISTER *const reg_arg,
                                                                             double   *const cpu_arg)
{
    assert(computer != nullptr);
    assert(mem_arg  != nullptr);
    assert(reg_arg  != nullptr);
    assert(cpu_arg  != nullptr);

    *mem_arg = cmd & (1 << PARAM_MEM);

    bool is_reg_arg = cmd & (1 << PARAM_REG);
    bool is_cpu_arg = cmd & (1 << PARAM_NUM);
    
    if (is_reg_arg)
    {
        check_inside(sizeof(REGISTER));
        executer_pull_cmd(&$cpu, reg_arg, sizeof(REGISTER));
    }
    if (is_cpu_arg)
    {
        check_inside(sizeof(cpu_type));
        executer_pull_cmd(&$cpu, cpu_arg, sizeof(double));
    }
    return true;
}

bool execute_call(machine *const computer)
{
    assert(computer != nullptr);

    int label_pc = 0;
    if (!executer_pull_label(computer, &label_pc)) return false;

    check_pc_inside(label_pc);
    stack_push     (&$call_stack, &$cpu.pc);
    $cpu.pc = label_pc;
    return true;
}

bool execute_jump(machine *const computer, const unsigned char cmd)
{
    assert(computer != nullptr);

    int label_pc = 0;
    if (!executer_pull_label(computer, &label_pc)) return false;

    check_pc_inside(label_pc);
    if (cmd == JMP)
    {
        $cpu.pc = label_pc;
        return true;
    }

    cpu_type num1 = 0;
    cpu_type num2 = 0;

    check_empty(data_stack, "JUMP");
    num2 = *(cpu_type *) stack_pop(&$data_stack);

    check_empty(data_stack, "JUMP");
    num1 = *(cpu_type *) stack_pop(&$data_stack);

    switch(cmd)
    {
        case JA : if (num1 > num2)                             { $cpu.pc = label_pc; } return true;
        case JAE: if (num1 > num2 || approx_equal(num1, num2)) { $cpu.pc = label_pc; } return true;
        case JB : if (num1 < num2)                             { $cpu.pc = label_pc; } return true;
        case JBE: if (num1 < num2 || approx_equal(num1, num2)) { $cpu.pc = label_pc; } return true;
        case JE : if (               approx_equal(num1, num2)) { $cpu.pc = label_pc; } return true;
        case JNE: if (              !approx_equal(num1, num2)) { $cpu.pc = label_pc; } return true;

        default : log_error(         "default case in execute_jump: cmd=%d(%d)\n", cmd, __LINE__);
                  assert   (false && "default vase in execute_jump");
                  break;
    }
    return false;
}

bool executer_pull_label(machine *const computer, int *const label_pc)
{
    assert(computer != nullptr);
    assert(label_pc != nullptr);

    check_inside(sizeof(int));
    executer_pull_cmd(&$cpu, label_pc, sizeof(int));
    return true;
}

bool execute_in(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num = 0;
    if (scanf("%lg", &num) != 1)
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "input value is not double\n", "IN");
        return false;
    }

    stack_push(&$data_stack, &num);
    return true;
}

bool execute_out(machine *const computer)
{
    assert(computer != nullptr);

    check_empty(data_stack, OUT);

    cpu_type num = *(cpu_type *) stack_front(&$data_stack);
    fprintf(stderr, "%lg\n", num);
    return true;
}

bool execute_ret(machine *const computer)
{
    assert(computer != nullptr);

    check_empty(call_stack, RET);

    $cpu.pc = *(int *) stack_pop(&$call_stack);
    return true;
}

bool execute_add(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num1 = 0;
    cpu_type num2 = 0;

    check_empty(data_stack, ADD);
    num2 = *(cpu_type *) stack_pop(&$data_stack);
    
    check_empty(data_stack, ADD);
    num1 = *(cpu_type *) stack_pop(&$data_stack);

    num1 += num2;
    stack_push(&$data_stack, &num1);
    return true;
}

bool execute_sub(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num1 = 0;
    cpu_type num2 = 0;

    check_empty(data_stack, SUB);
    num2 = *(cpu_type *) stack_pop(&$data_stack);

    check_empty(data_stack, SUB);
    num1 = *(cpu_type *) stack_pop(&$data_stack);

    num1 -= num2;
    stack_push(&$data_stack, &num1);
    return true;
}

bool execute_mul(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num1 = 0;
    cpu_type num2 = 0;

    check_empty(data_stack, MUL);
    num2 = *(cpu_type *) stack_pop(&$data_stack);

    check_empty(data_stack, MUL);
    num1 = *(cpu_type *) stack_pop(&$data_stack);

    num1 *= num2;
    stack_push(&$data_stack, &num1);
    return true;
}

bool execute_div(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num1 = 0;
    cpu_type num2 = 0;

    check_empty(data_stack, DIV);
    num2 = *(cpu_type *) stack_pop(&$data_stack);

    check_empty(data_stack, DIV);
    num1 = *(cpu_type *) stack_pop(&$data_stack);

    if (approx_equal(num2, 0))
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "division by zero\n", "DIV");
        return false;
    }

    num1 /= num2;
    stack_push(&$data_stack, &num1);
    return true;
}

bool execute_pow(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num1 = 0;
    cpu_type num2 = 0;

    check_empty(data_stack, POW);
    num2 = *(cpu_type *) stack_pop(&$data_stack);

    check_empty(data_stack, POW);
    num1 = *(cpu_type *) stack_pop(&$data_stack);

    if (approx_equal(num1, 0) && num2 < 0)
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "division by zero\n", "POW");
        return false;
    }
    if (num1 < 0)
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "pow of less zero basis\n", "POW");
        return false;
    }
    num1 = pow(num1, num2);
    stack_push(&$data_stack, &num1);
    return true;
}

bool execute_sqrt(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num = 0;

    check_empty(data_stack, SQRT);
    num = *(cpu_type *) stack_pop(&$data_stack);

    if (num < 0)
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "sqrt of less zero number\n", "SQRT");
        return false;
    }
    num = sqrt(num);
    stack_push(&$data_stack, &num);
    return true;
}

bool execute_sin(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num = 0;

    check_empty(data_stack, SIN);
    num = *(cpu_type *) stack_pop(&$data_stack);

    num = sin(num);
    stack_push(&$data_stack, &num);
    return true;
}

bool execute_cos(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num = 0;

    check_empty(data_stack, COS);
    num = *(cpu_type *) stack_pop(&$data_stack);

    num = cos(num);
    stack_push(&$data_stack, &num);

    return true;
}

bool execute_log(machine *const computer)
{
    assert(computer != nullptr);

    cpu_type num = 0;

    check_empty(data_stack, LOG);
    num = *(cpu_type *) stack_pop(&$data_stack);

    if (num < 0 || approx_equal(num, 0))
    {
        fprintf(stderr, "%-5s" TERMINAL_RED " RUNTIME ERROR: " TERMINAL_CANCEL "log of less zero number\n", "LOG");
        return false;
    }
    num = log(num);
    stack_push(&$data_stack, &num);

    return true;
}

/*===========================================================================================================================*/
// MACHINE_CTOR_DTOR
/*===========================================================================================================================*/

bool machine_ctor(machine *const computer, const char *execute_file)
{
    assert(computer     != nullptr);
    assert(execute_file != nullptr);

    bool no_err = true;

    stack_ctor(&$call_stack, sizeof(int));
    stack_ctor(&$data_stack, sizeof(cpu_type));

    no_err = executer_ctor(&$cpu, execute_file);

    for (int i = 0; i <  RAM_SIZE  ; ++i)     $ram[i] = 0;
    for (int i = 0; i <= REG_NUMBER; ++i) $int_reg[i] = 0;

    if (no_err) return true;

    log_error("executer_ctor returned false(%d)\n", __LINE__);
    return false;
}

void machine_dtor(machine *const computer)
{
    assert(computer != nullptr);

    stack_dtor   (&$call_stack);
    stack_dtor   (&$data_stack);
    executer_dtor(&$cpu);
}