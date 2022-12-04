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

void cpu_cmd_ctor(cpu_cmd *const cmd_store, const int store_size)
{
    assert(cmd_store != nullptr);
    assert(store_size >       0);

    cmd_store->data = log_calloc((size_t) store_size, sizeof(char));
    cmd_store->size = store_size;
    cmd_store->pc   = 0;
}

void cpu_cmd_dtor(cpu_cmd *const cmd_store)
{
    assert(cmd_store != nullptr);

    log_free(cmd_store->data);
}

void add_cpu_cmd(cpu_cmd *const cmd_store, const void *cmd, const size_t cmd_size)
{
    assert(cmd_store != nullptr);
    assert(cmd       != nullptr);
    assert(cmd_size  > 0);

    memcpy((char *) cmd_store->data + cmd_store->pc, cmd, cmd_size);
    cmd_store->pc += (int) cmd_size;
}

void source_cmd_ctor(source_cmd *const code_store, const void *code_buff, const int code_buff_size)
{
    assert(code_store != nullptr);
    assert(code_buff  != nullptr);
    assert(code_buff_size > 0);

    code_store->code      = (const char *) code_buff;
    code_store->code_size = code_buff_size;
    code_store->code_pos  = 0;
    code_store->code_line = 1;
}