#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "../../lib/logs/log.h"
#include "../../lib/read_write/read_write.h"
#include "../../lib/algorithm/algorithm.h"

#include "cpu.h"
#include "terminal_colors.h"

/*===========================================================================================================================*/
// EXECUTER_CTOR_DTOR
/*===========================================================================================================================*/

void executer_ctor(executer *const cpu)
{
    assert(cpu != nullptr);

    cpu->cmd      = nullptr;
    cpu->capacity = 0;
    cpu->pc       = 0;
}

void executer_ctor(executer *const cpu, const int size)
{
    assert(cpu != nullptr);

    cpu->cmd      = log_calloc((size_t) size, sizeof(cpu_type));
    cpu->capacity = size;
    cpu->pc       = 0;
}

bool executer_ctor(executer *const cpu, const char *execute_file)
{
    assert(cpu          != nullptr);
    assert(execute_file != nullptr);

    cpu->cmd = read_file(execute_file, &cpu->capacity);
    cpu->pc  = 0;

    if (cpu->cmd == nullptr)
    {
        log_error("can't open execute file \"%s\"(%d)\n", execute_file, __LINE__);
        return false;
    }
    return true;
}

void executer_dtor(executer *const cpu)
{
    assert(cpu != nullptr);

    log_free(cpu->cmd);
}

/*===========================================================================================================================*/
// EXTRA EXECUTER FUNCTION
/*===========================================================================================================================*/

void executer_add_cmd(executer *const cpu, const void *const cmd, const size_t cmd_size)
{
    assert(cpu != nullptr);
    assert(cmd != nullptr);
    assert((size_t) cpu->pc + cmd_size <= (size_t) cpu->capacity * sizeof(cpu_type));

    memcpy((char *) cpu->cmd + cpu->pc, cmd, cmd_size);
    cpu->pc += (int) cmd_size;
}

void executer_pull_cmd(executer *const cpu, void *const pull_in, const size_t pull_size)
{
    assert(cpu     != nullptr);
    assert(pull_in != nullptr);
    assert((size_t) cpu->pc + pull_size <= (size_t) cpu->capacity);

    memcpy(pull_in, (const char *) cpu->cmd + cpu->pc, pull_size);
    cpu->pc += (int) pull_size;
}
