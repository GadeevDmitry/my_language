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
#include "cpu.h"
#include "terminal_colors.h"

/*===========================================================================================================================*/
// EXECUTER_CTOR_DTOR
/*===========================================================================================================================*/

void executer_ctor(executer *const cpu)
{
    assert(cpu != nullptr);

    cpu->cmd     = nullptr;
    cpu->capcity = 0;
    cpu->pc      = 0;
}

void executer_ctor(executer *const cpu, const int size)
{
    assert(cpu != nullptr);
    //>>>>>>>>>>>>>>
    log_message("\nexecuter_ctor: capacity = %d(%d)\n", size, __LINE__);
    //<<<<<<<<<<<<<<
    cpu->cmd     = log_calloc((size_t) size, sizeof(cpu_type));
    cpu->capcity = size;
    cpu->pc      = 0;
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
    assert((size_t) cpu->pc + cmd_size <= (size_t) cpu->capcity * sizeof(cpu_type));

    memcpy((char *) cpu->cmd + cpu->pc, cmd, cmd_size);
    cpu->pc += (int) cmd_size;
}