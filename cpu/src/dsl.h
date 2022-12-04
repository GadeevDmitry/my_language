#ifndef DSL
#define DSL

#define CODE(code_store) (code_store)->code
#define SIZE(code_store) (code_store)->code_size
#define  POS(code_store) (code_store)->code_pos
#define LINE(code_store) (code_store)->code_line

#define code         CODE(code_store)
#define code_size    SIZE(code_store)
#define code_pos      POS(code_store)
#define code_line    LINE(code_store)

#endif //DSL