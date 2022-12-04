#ifndef CPU
#define CPU

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

void add_cpu_cmd    (cpu_cmd    *const  cmd_store, const void *cmd,       const size_t    cmd_size);
void cpu_cmd_ctor   (cpu_cmd    *const  cmd_store,                        const int     store_size);
void cpu_cmd_dtor   (cpu_cmd    *const  cmd_store                                                 );
void source_cmd_ctor(source_cmd *const code_store, const void *code_buff, const int code_buff_size);

#endif //CPU