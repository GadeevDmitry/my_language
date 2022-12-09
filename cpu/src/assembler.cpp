/** @file */

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

#include "terminal_colors.h"
#include "assembler.h"

/*===========================================================================================================================*/
// MAIN
/*===========================================================================================================================*/

int main(const int argc, const char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "You should give two parameters: file to compile and execute file.\n");
        return 0;
    }
    source *code = new_source(argv[1]);
    FILE *stream = fopen     (argv[2], "w");

    if (code   == nullptr) { fclose(stream); return 0; }
    if (stream == nullptr)
    {
        source_dtor(code);

        fprintf(stderr, "can't open execute file\n");
        return 0;
    }

    lexical_analyzer   (code);
  //lexis_text_dump    (code);
  //lexis_graphviz_dump(code);

    translator my_asm = {};

    if (assembler(code, &my_asm))
    {
        fwrite (my_asm.cpu.cmd, sizeof(char), my_asm.cpu.pc + 1, stream);
        fprintf(stderr, TERMINAL_GREEN "compile success\n" TERMINAL_CANCEL);

        translator_dtor(&my_asm);
    }
    else fprintf(stderr, TERMINAL_RED "\ncompile faliled\n" TERMINAL_CANCEL);

    fclose(stream);
}

/*===========================================================================================================================*/
// ASSEMBLER
/*===========================================================================================================================*/

bool assembler(source *const code, translator *const my_asm)
{
    assert(code   != nullptr);
    assert(my_asm != nullptr);

    translator_ctor(my_asm, code);

    if (!do_assembler(code, my_asm, 1)) return false;
    
    my_asm->cpu.pc = 0;
    label_text_dump(&my_asm->link);

    if (!do_assembler(code, my_asm, 2)) return false;

    return true;
}

bool do_assembler(source *const code, translator *const my_asm, const int asm_num) //asm_num for name of labels after their calling
{
    log_header(__PRETTY_FUNCTION__);
    log_message("asm_num=%d\n", asm_num);

    assert(code   != nullptr);
    assert(my_asm != nullptr);

    bool no_err = true;

    for (int token_cnt = 0; token_cnt < lexis_pos;) //now lexis_pos is using as the size of lexis_data
    {
        int cur_no_err = true;

        switch(lexis_data[token_cnt].type)
        {
            case INSTRUCTION: cur_no_err = translate_instruction(my_asm, &token_cnt, asm_num);
                              break;

            case INT_NUM    : fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "number can't be the instruction or label\n", lexis_data[token_cnt++].token_line);
                              cur_no_err = false;
                              break;
            case REG_NAME   : fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "register can't be the instruction or label\n", lexis_data[token_cnt++].token_line);
                              cur_no_err = false;
                              break;
            case KEY_CHAR   : fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "instruction or label can't include key characters\n", lexis_data[token_cnt++].token_line);
                              cur_no_err = false;
                              break;
            case UNDEF_TOKEN: cur_no_err = translate_undef_token(my_asm, &token_cnt);
                              break;
            
            default: log_error(         "default case in assembler(): lexis_data[token_cnt].type=%d(%d)\n", lexis_data[token_cnt].type, __LINE__);
                     assert   (false && "default case in assembler()");
                     break;
        }
        no_err = (no_err == false) ? false : cur_no_err;
    }

    if (no_err) { log_end_header(); return true; }

    translator_dtor(my_asm);
    log_end_header ();
    return false;
}

#define cur_token    my_asm->lexis_data[*token_cnt]
#define still_inside still_inside_lexis_data(my_asm, token_cnt)
#define check_inside                                                                                \
   if (!still_inside)                                                                               \
   {                                                                                                \
    fprintf(stderr, TERMINAL_RED "\nERROR: " TERMINAL_CANCEL "no arguments at the end of file\n");  \
    return false;                                                                                   \
   }

/**
*   @brief Переводит инструкции процессора и их параметры.
*
*   @param my_asm    [in][out] - транслятор
*   @param token_cnt [in][out] - указатель на номер обрабатываемого токена
*
*   @return true, если синтаксической ошибки не произошло и false в противном случае
*
*   @note Значение по адресу token_cnt увеличивается после обработки очередного токена.
*/

bool translate_instruction(translator *const my_asm, int *const token_cnt, const int asm_num)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    switch(cur_token.value.instruction)
    {
        case HLT :
        case IN  :
        case OUT :
        case RET :
        case ADD :
        case SUB :
        case MUL :
        case DIV :
        case POW : return translate_no_parametres(my_asm, token_cnt);
        case PUSH: return translate_push         (my_asm, token_cnt);
        case POP : return translate_pop          (my_asm, token_cnt);
        case CALL:
        case JMP :
        case JA  :
        case JAE :
        case JB  :
        case JBE :
        case JE  :
        case JNE : return translate_jump_call    (my_asm, token_cnt, asm_num);

        case UNDEF_ASM_CMD:
        default           : log_error(         "default case in translate_instruction(): cur_token.value.instruction=%d(%d)\n", cur_token.token_line);
                            assert   (false && "default case in translate_instruction()");
                            break;
    }
    return false;
}

/**
*   @brief Переводит инструкции без параметров.
*
*   @param my_asm    [in][out] - транслятор
*   @param token_cnt [in][out] - указатель на номер обрабатываемого токена
*
*   @return true, если синтаксической ошибки не произошло и false в противном случае
*
*   @note Значение по адресу token_cnt увеличивается после обработки очередного токена.
*/

bool translate_no_parametres(translator *const my_asm, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    unsigned char cmd = cur_token.value.instruction;

    executer_add_cmd(&my_asm->cpu, &cmd, sizeof(unsigned char));
    *token_cnt += 1;

    return true;
}

bool translate_push(translator *const my_asm, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    unsigned char cmd = PUSH;
    *token_cnt += 1;
    check_inside

    if (cur_token.type == KEY_CHAR && cur_token.value.key == '[')
    {
        *token_cnt += 1;
        check_inside
        return translate_ram(my_asm, token_cnt, cmd);
    }
    return translate_parametres(my_asm, cmd, token_cnt);
}

bool translate_pop(translator *const my_asm, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    unsigned char cmd = POP;
    *token_cnt += 1;
    check_inside

    if (cur_token.type == REG_NAME)
    {
        cmd = cmd | (1 << PARAM_REG);
        executer_add_parametres(my_asm, cmd, cur_token.value.reg_num, 0);

        *token_cnt += 1;
        return true;
    }

    if (cur_token.type == KEY_CHAR && cur_token.value.key == '[')
    {
        *token_cnt += 1;
        check_inside
        return translate_ram(my_asm, token_cnt, cmd);
    }

    if (cur_token.type == UNDEF_TOKEN)
    {
        if (cur_token.value.token_len == 5 &&                // "void": 4 characters + 1 null-character
            !strncasecmp(my_asm->buff_data+cur_token.token_beg, "void", 4))
        {
            executer_add_cmd(&my_asm->cpu, &cmd, sizeof(unsigned char));

            *token_cnt += 1;
            return true;
        }
    }
    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "invalid pop-arg\n", cur_token.token_line);
    *token_cnt += 1;
    return false;
}

bool translate_jump_call(translator *const my_asm, int *const token_cnt, const int asm_num)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    unsigned char cmd = cur_token.value.instruction;
    *token_cnt += 1;
    check_inside
    
    switch(cur_token.type)
    {
        case INSTRUCTION: fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "instruction can't be the name of label\n", cur_token.token_line);
                          *token_cnt += 1;
                          return false;
        case REG_NAME   : fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "register can't be the name of label\n", cur_token.token_line);
                          *token_cnt += 1;
                          return false;
        case INT_NUM    : fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "number can't be the name of label\n", cur_token.token_line);
                          *token_cnt += 1;
                          return false;
        case KEY_CHAR   : fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "key char can't be the name of label\n", cur_token.token_line);
                          *token_cnt += 1;
                          return false;

        case UNDEF_TOKEN: {
                            int link_pc = get_label_pc(my_asm, token_cnt);
                            executer_add_cmd(&my_asm->cpu, &cmd    , sizeof(unsigned char));
                            executer_add_cmd(&my_asm->cpu, &link_pc, sizeof(int));

                            if (link_pc == -1 && asm_num == 2)
                            {
                                fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "undefined label\n", cur_token.token_line);
                                *token_cnt += 1;
                                return false;
                            }
                            *token_cnt += 1;
                            return true;
                          }
        default: log_error(         "default case in translate_jump_call(): cur_token.type=%d(%d)\n", cur_token.type, __LINE__);
                 assert   (false && "default case in translate_jump_call()");
                 break;
    }
    return false;
}

bool translate_ram(translator *const my_asm, int *const token_cnt, unsigned char cmd)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    cmd = cmd | (1 << PARAM_MEM);
    bool right_param = translate_parametres(my_asm, cmd, token_cnt);

    if (cur_token.type == KEY_CHAR && cur_token.value.key == ']')
    {
        *token_cnt += 1;
        return right_param;
    }
    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "skipped \']\'\n", cur_token.token_line);
    *token_cnt += 1;
    return false;
}

/**
*   @brief Переводит выражение видов: REG, INT, REG+INT, INT+REG.
*
*   @param my_asm    [in][out] - транслятор
*   @param token_cnt [in][out] - указатель на номер обрабатываемого токена
*   @param reg_arg       [out] - указатель на переменную типа REGISTER, чтобы положить туда номер регистра-параметра
*   @param int_arg       [out] - указатель на переменную типа int     , чтобы положить туда число-параметр
*
*   @return маска, кодирующая одно из четырёх выражений.
*
*   @note Маска равна нулю, в случае ошибки. Ошибкой является выражение одного из видов: !(INT | REG), INT+(!REG), REG+(!INT).
*   @note Значение по адресу token_cnt увеличивается после обработки очередного токена.
*/

unsigned char translate_expretion(translator *const my_asm, int *const token_cnt, REGISTER *const reg_arg,
                                                                                  int      *const int_arg)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);
    assert(reg_arg   != nullptr);
    assert(int_arg   != nullptr);

    unsigned char cmd_param = 0;

    if (!translate_reg_plus_int(my_asm, token_cnt, reg_arg, int_arg, &cmd_param)) return cmd_param = 0; //ошибка
    if (cmd_param)                                                                return cmd_param;     //выражение правильного вида
    
    if (!translate_int_plus_reg(my_asm, token_cnt, reg_arg, int_arg, &cmd_param)) return cmd_param = 0;
    if (!cmd_param)
    {
        fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "invalid expretion\n", cur_token.token_line);
        *token_cnt += 1;
    }
    return cmd_param;
}

/**
*   @brief Переводит выражение видов: REG, REG+INT.
*
*   @param my_asm    [in][out] - транслятор
*   @param token_cnt [in][out] - указатель на номер обрабатываемого токена
*   @param reg_arg       [out] - указатель на переменную типа REGISTER     , чтобы положить туда номер регистра-параметра
*   @param int_arg       [out] - указатель на переменную типа int          , чтобы положить туда число-параметр
*   @param cmd_param     [out] - указатель на переменную типа unsigned char, чтобы положить туда маску, кодирующую одно из двух выражений
*
*   @return true, если ошибки не произошло и false в противном случае
*
*   @note Ошибкой является выражение вида REG+(!INT). Выражение REG+(!INT) имеет маску, равную 0(так как это ошибка).
*   @note Выражение (!REG) имеет маску, равную 0, но ошибкой не является. Выражения различаются по возвращаемому значению.
*   @note Значение по адресу token_cnt увеличивается после обработки очередного токена.
*/

bool translate_reg_plus_int(translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                            int           *const int_arg,
                                                                            unsigned char *const cmd_param)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);
    assert(reg_arg   != nullptr);
    assert(int_arg   != nullptr);
    assert(cmd_param != nullptr);

    if (cur_token.type == REG_NAME)
    {
        *cmd_param  = (*cmd_param) | (1 << PARAM_REG);
        *reg_arg    = cur_token.value.reg_num;
        *token_cnt += 1;
        check_inside

        if (cur_token.type == KEY_CHAR && cur_token.value.key == '+')
        {
            *token_cnt += 1;
            check_inside

            if (cur_token.type == INT_NUM)
            {
                *cmd_param  = (*cmd_param) | (1 << PARAM_INT);
                *int_arg    = cur_token.value.int_num;
                *token_cnt += 1;

                return true;
            }

            fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "invalid int-arg\n", cur_token.token_line);
            *token_cnt += 1;
            return false;
        }
        return true;
    }
    return true;
}

/**
*   @brief Аналог translate_reg_plus_int().
*/

bool translate_int_plus_reg(translator *const my_asm, int *const token_cnt, REGISTER      *const reg_arg,
                                                                            int           *const int_arg,
                                                                            unsigned char *const cmd_param)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);
    assert(reg_arg   != nullptr);
    assert(int_arg   != nullptr);
    assert(cmd_param != nullptr);

    if (cur_token.type == INT_NUM)
    {
        *cmd_param  = (*cmd_param) | (1 << PARAM_INT);
        *int_arg    = cur_token.value.int_num;
        *token_cnt += 1;
        check_inside

        if (cur_token.type == KEY_CHAR && cur_token.value.key == '+')
        {
            *token_cnt += 1;
            check_inside

            if (cur_token.type == REG_NAME)
            {
                *cmd_param  = (*cmd_param) | (1 << PARAM_REG);
                *reg_arg    = cur_token.value.reg_num;
                *token_cnt += 1;

                return true;
            }
            fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "invalid reg-arg\n", cur_token.token_line);
            *token_cnt += 1;
            return false;
        }
        return true;
    }
    return true;
}

bool translate_undef_token(translator *const my_asm, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);
    assert(cur_token.type == UNDEF_TOKEN);

    int cur_label_pc = get_label_pc(my_asm, token_cnt);
    if (cur_label_pc != -1 && cur_label_pc != my_asm->cpu.pc)
    {
        fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "redefined label\n", cur_token.token_line);
        *token_cnt += 1;
        return false;
    }
    if (cur_label_pc == -1)
    {
        label_store_push(&my_asm->link, *token_cnt, my_asm->cpu.pc);
    }
    *token_cnt += 1;
    check_inside
    if (cur_token.type == KEY_CHAR && cur_token.value.key == ':')
    {
        *token_cnt += 1;
        return true;
    }
    fprintf(stderr, "line %-5d" TERMINAL_RED " ERROR: " TERMINAL_CANCEL "skipped \':\' after label or undefined instruction\n", cur_token.token_line);
    *token_cnt += 1;
    return false;
}

bool translate_parametres(translator *const my_asm, unsigned char cmd, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    REGISTER reg_arg = ERR_REG;
    int      int_arg = 0;

    unsigned char cmd_param = translate_expretion(my_asm, token_cnt, &reg_arg, &int_arg);
    cmd = cmd  |  cmd_param;

    executer_add_parametres(my_asm, cmd, reg_arg, int_arg);
    return cmd_param != 0;
}

void executer_add_parametres(translator *const my_asm, const unsigned char cmd, const REGISTER reg_arg, const int int_arg)
{
    assert(my_asm != nullptr);

    executer_add_cmd(&my_asm->cpu, &cmd, sizeof(unsigned char));
        
    if (cmd & (1 << PARAM_REG)) executer_add_cmd(&my_asm->cpu, &reg_arg, sizeof(REGISTER));
    if (cmd & (1 << PARAM_INT)) executer_add_cmd(&my_asm->cpu, &int_arg, sizeof(int));
}

#undef cur_token
#undef still_inside
#undef check_inside

/*===========================================================================================================================*/
// TRANSLATOR_CTOR_DTOR
/*===========================================================================================================================*/

void translator_ctor(translator *const my_asm, source *const code)
{
    assert(my_asm != nullptr);
    assert(code != nullptr);

    my_asm->code = code;
    
    label_store_ctor(&my_asm->link, get_undef_token_num(code));
    executer_ctor   (&my_asm->cpu , lexis_pos);
}

void translator_dtor(translator *const my_asm)
{
    assert(my_asm != nullptr);

    source_dtor     ( my_asm->code);
    label_store_dtor(&my_asm->link);
    executer_dtor   (&my_asm->cpu );
}

/*===========================================================================================================================*/
// EXTRA FUNCTION
/*===========================================================================================================================*/

bool still_inside_lexis_data(const translator *const my_asm, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);

    return *token_cnt < my_asm->lexis_pos;
}

int get_undef_token_num(const source *const code)
{
    assert(code != nullptr);

    int num = 0;

    for (int i = 0; i < lexis_pos; ++i)
    {
        if (lexis_data[i].type == UNDEF_TOKEN) ++num;
    }
    return num;
}

#define cur_token my_asm->lexis_data[*token_cnt]

#define cur_label_pc        my_asm->link.store[label_cnt].pc
#define cur_label_token_num my_asm->link.store[label_cnt].token_num
#define cur_label_token     my_asm->lexis_data[cur_label_token_num]

int get_label_pc(const translator *const my_asm, int *const token_cnt)
{
    assert(my_asm    != nullptr);
    assert(token_cnt != nullptr);
    assert(cur_token.type == UNDEF_TOKEN);

    for (int label_cnt = 0; label_cnt < my_asm->link.size; ++label_cnt)
    {
        if (cur_label_token.value.token_len == cur_token.value.token_len)
        {
            if (!strncasecmp(my_asm->buff_data+      cur_token.token_beg,
                             my_asm->buff_data+cur_label_token.token_beg, (size_t) cur_token.value.token_len-1))
            {
                return cur_label_pc;
            }
        }
    }
    return -1;
}

#undef cur_token

#undef cur_label_pc
#undef cur_label_token_num
#undef cur_label_token

/*===========================================================================================================================*/
// SOURCE_CTOR_DTOR
/*===========================================================================================================================*/

source *new_source(const char *source_file)
{
    assert(source_file != nullptr);

    source *code = (source *) log_calloc(1, sizeof(source));

    if (!source_buff_ctor (code, source_file)) { source_dtor(code); return nullptr; }
    if (!source_lexis_ctor(code)             ) { source_dtor(code); return nullptr; }

    return code;
}

bool source_buff_ctor(source *const code, const char *source_file)
{
    assert(code        != nullptr);
    assert(source_file != nullptr);

    buff_data = (const char *) read_file(source_file, &buff_size);
    buff_line = 1;
    buff_pos  = 0;

    if (buff_data == nullptr)
    {
        log_error(        "can't open \"%s\"(%d)\n", source_file, __LINE__);
        fprintf  (stderr, "can't open \"%s\"\n"    , source_file);
        return false;
    }
    return true;
}

bool source_lexis_ctor(source *const code)
{
    assert(code != nullptr);

    lexis_data      = (token *) log_calloc((size_t) buff_size, sizeof(token));
    lexis_cur_token = (char  *) log_calloc((size_t) buff_size, sizeof(char));
    lexis_pos       = 0;

    if (lexis_data == nullptr || lexis_cur_token == nullptr)
    {
        log_error(        "can't allocate memory for lexis(%d)\n", __LINE__);
        fprintf  (stderr, "can't allocate memory for lexis\n");
        return false;
    }
    return true;
}

void source_dtor(source *const code) //only if "code" was created by calloc or malloc
{
    if (code == nullptr)
    {
        log_warning("struct source to dtor is nullptr(%d)\n", __LINE__);
        return;
    }

    log_free((char *) buff_data);
    log_free(lexis_data);
    log_free(lexis_cur_token);

    log_free(code);
}

/*===========================================================================================================================*/
// LEXICAL_ANALYSIS
/*===========================================================================================================================*/

void lexical_analyzer(source *const code)
{
    assert(code != nullptr);

    skip_source_spaces(code);
    while (buff_pos < buff_size && buff_data[buff_pos] != '\0')
    {
        if (is_comment (code)) continue;      // spaces are skipped after is_comment()
        if (is_key_char(buff_data[buff_pos])) create_key_char_token(code);
        else
        {
            int token_beg = buff_pos;
            int token_len = get_another_token(code);

            if      (get_reg_name(lexis_cur_token, token_len) != ERR_REG) create_reg_token(code, token_beg, token_len);
            else if (get_int_num (lexis_cur_token           ))            create_int_token(code, token_beg);
            else
            {
                ASM_CMD cur_instruction = get_asm_cmd(lexis_cur_token);

                if (cur_instruction == UNDEF_ASM_CMD) create_undef_token      (code, token_beg, token_len);
                else                                  create_instruction_token(code, token_beg, cur_instruction);
            }
        }

        skip_source_spaces(code);
    }
}

int get_another_token(source *const code)
{
    assert(code != nullptr);

    int token_len = 0;
    while (buff_pos < buff_size &&  !is_key_char(buff_data[buff_pos]) &&
                                    !isspace    (buff_data[buff_pos]))
    {
        lexis_cur_token[token_len++] = buff_data[buff_pos++];
    }

    lexis_cur_token[token_len++] = '\0';
    return token_len;
}

 REGISTER get_reg_name(const char *cur_token, const int token_len)
{
    assert(cur_token != nullptr);

    if (token_len != REG_NAME_LEN) return ERR_REG;

    REGISTER reg = ERR_REG;
    if (tolower(cur_token[0]) == 'r' && tolower(cur_token[2]) == 'x') reg = (REGISTER) (tolower(cur_token[1]) - 'a' + 1);

    if (0 <= reg && reg <= REG_NUMBER) return reg;
    return reg = ERR_REG;
}

bool get_int_num(const char *cur_token, int *const ret) //ret = nullptr
{
    assert(cur_token != nullptr);

    int int_num = 0;
    int num_len = 0;

    if (sscanf(cur_token, "%d%n", &int_num, &num_len) <= 0) return false;
    if (cur_token[num_len] != '\0')                         return false;

    if (ret != nullptr) *ret = int_num;
    return true;
}

ASM_CMD get_asm_cmd(const char *cur_token)
{
    assert(cur_token != nullptr);

    if (!strcasecmp("hlt" , cur_token)) return HLT ;
    
    if (!strcasecmp("in"  , cur_token)) return IN  ;
    if (!strcasecmp("out" , cur_token)) return OUT ;

    if (!strcasecmp("push", cur_token)) return PUSH;
    if (!strcasecmp("pop" , cur_token)) return POP ;

    if (!strcasecmp("jmp" , cur_token)) return JMP ;
    if (!strcasecmp("ja"  , cur_token)) return JA  ;
    if (!strcasecmp("jae" , cur_token)) return JAE ;
    if (!strcasecmp("jb"  , cur_token)) return JB  ;
    if (!strcasecmp("jbe" , cur_token)) return JBE ;
    if (!strcasecmp("je"  , cur_token)) return JE  ;
    if (!strcasecmp("jne" , cur_token)) return JNE ;

    if (!strcasecmp("call", cur_token)) return CALL;
    if (!strcasecmp("ret" , cur_token)) return RET ;

    if (!strcasecmp("add" , cur_token)) return ADD ;
    if (!strcasecmp("sub" , cur_token)) return SUB ;
    if (!strcasecmp("mul" , cur_token)) return MUL ;
    if (!strcasecmp("div" , cur_token)) return DIV ;
    if (!strcasecmp("pow" , cur_token)) return POW ;

    return UNDEF_ASM_CMD;
}

bool is_key_char(const char char_to_check)
{
    if (char_to_check == '\0') return true;

    for (int i = 0; KEY_CHARS[i] != '\0'; ++i)
    {
        if (char_to_check == KEY_CHARS[i]) return true;
    }
    return false;
}

bool is_comment(source *const code)
{
    assert(code != nullptr);

    if (buff_data[buff_pos] == '#')
    {
        skip_source_line  (code);
        skip_source_spaces(code);
        return true;
    }
    return false;
}

/*===========================================================================================================================*/
// TOKEN_CTOR
/*===========================================================================================================================*/

void create_key_char_token(source *const code)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type       = KEY_CHAR;
    lexis_data[lexis_pos].token_beg  = buff_pos;
    lexis_data[lexis_pos].token_line = buff_line;
    lexis_data[lexis_pos].value.key  = buff_data[buff_pos++];

    lexis_pos++;
}

void create_reg_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type          = REG_NAME;
    lexis_data[lexis_pos].token_beg     = token_beg;
    lexis_data[lexis_pos].token_line    = buff_line;
    lexis_data[lexis_pos].value.reg_num = get_reg_name(lexis_cur_token, token_len);

    lexis_pos++;
}

void create_int_token(source *const code, const int token_beg)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type       = INT_NUM;
    lexis_data[lexis_pos].token_beg  = token_beg;
    lexis_data[lexis_pos].token_line = buff_line;

    get_int_num(lexis_cur_token, &lexis_data[lexis_pos].value.int_num);
    lexis_pos++;
}

void create_instruction_token(source *const code, const int token_beg, const ASM_CMD asm_cmd)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type              = INSTRUCTION;
    lexis_data[lexis_pos].token_beg         = token_beg;
    lexis_data[lexis_pos].token_line        = buff_line;
    lexis_data[lexis_pos].value.instruction = asm_cmd;

    lexis_pos++;
}

void create_undef_token(source *const code, const int token_beg, const int token_len)
{
    assert(code != nullptr);

    lexis_data[lexis_pos].type            = UNDEF_TOKEN;
    lexis_data[lexis_pos].token_beg       = token_beg;
    lexis_data[lexis_pos].token_line      = buff_line;
    lexis_data[lexis_pos].value.token_len = token_len;

    lexis_pos++;
}

/*===========================================================================================================================*/
// SKIP_SOURCE
/*===========================================================================================================================*/

void skip_source_line(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && buff_data[buff_pos] != '\n') buff_pos++;

    if (buff_pos < buff_size)
    {
        ++buff_line;
        ++buff_pos;
    }
}

void skip_source_spaces(source *const code)
{
    assert(code != nullptr);

    while (buff_pos < buff_size && isspace(buff_data[buff_pos]))
    {
        if (buff_data[buff_pos] == '\n') ++buff_line;
        ++buff_pos;
    }
}

/*===========================================================================================================================*/
// DUMP
/*===========================================================================================================================*/

void label_text_dump(const label_store *const link)
{
    if (link == nullptr)
    {
        log_warning("link to dump is nullptr(%d)\n", __LINE__);
        return;
    }

    log_message("\n"
                "link\n"
                "{\n"
                "    store   =%p\n"
                "    size    =%d\n"
                "    capacity=%d\n"
                "}\n",
                link->store, link->size, link->capacity);
    
    log_message("link.store:\n\n"
                "     index:");
    for (int i = 0; i < link->size; ++i) log_message("%5d ", i);
    
    log_message("\n"
                " token_num:");
    for (int i = 0; i < link->size; ++i) log_message("%5d ", link->store[i].token_num);

    log_message("\n"
                "        pc:");
    for (int i = 0; i < link->size; ++i) log_message("%5d ", link->store[i].pc);

    log_message("\n\n");
}

void lexis_text_dump(const source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr(%d)\n", __LINE__);
        return;
    }

    log_message("\n"
                "code\n"
                "{\n"
                "    buff\n"
                "    {\n"
                "        data = %p\n"
                "        pos  = %d\n"
                "        line = %d\n"
                "        size = %d\n"
                "    }\n"
                "    lexis\n"
                "    {\n"
                "        data      = %p\n"
                "        pos       = %d\n"
                "        cur_token = %p\n"
                "    }\n"
                "}\n\n"
                ,
                 buff_data,  buff_pos, buff_line, buff_size,
                lexis_data, lexis_pos, lexis_cur_token);
}

void lexis_graphviz_dump(const source *const code)
{
    if (code == nullptr)
    {
        log_warning("code to dump is nullptr(%d)\n", __LINE__);
        return;
    }

    static int cur = 0;

    char    dump_txt[GRAPHVIZ_SIZE_FILE] = "";
    char    dump_png[GRAPHVIZ_SIZE_FILE] = "";

    sprintf(dump_txt, "dump_txt/lexis%d.txt", cur);
    sprintf(dump_png, "dump_png/lexis%d.png", cur);

    FILE *stream_txt =  fopen(dump_txt, "w");
    if   (stream_txt == nullptr)
    {
        log_error("can't open dump file(%d)\n", __LINE__);
        return;
    }
    ++cur;

    setvbuf(stream_txt, nullptr, _IONBF, 0);
    fprintf(stream_txt, "%s", LEXIS_GRAPHVIZ_HEADER);

    do_lexis_graphviz_dump(code, stream_txt);

    fprintf(stream_txt, "}\n");
    system_graphviz_dump(dump_txt, dump_png);

    fclose(stream_txt);
}

void do_lexis_graphviz_dump(const source *const code, FILE *const stream)
{
    assert(code        != nullptr);
    assert(stream      != nullptr);

    for (int i = 0; i < lexis_pos    ; ++i) graphviz_dump_token(code, stream, i);
    for (int i = 0; i < lexis_pos - 1; ++i) graphviz_dump_edge (      stream, i, i + 1);
}

void graphviz_dump_edge(FILE *const stream, const int from, const int to)
{
    assert(stream != nullptr);

    fprintf(stream, "node%d->node%d[weight=10000, style=\"invis\"]\n", from, to);
}

void graphviz_dump_token(const source *const code, FILE *const stream, const int dumping_pos)
{
    assert(code        != nullptr);
    assert(stream      != nullptr);

    GRAPHVIZ_COLOR     color = BLACK;
    GRAPHVIZ_COLOR fillcolor = BLACK;

    switch(lexis_data[dumping_pos].type)
    {
        case INSTRUCTION:     color =  DARK_BLUE;
                          fillcolor = LIGHT_BLUE;
                          break;
        case REG_NAME   :     color =  DARK_GREEN;
                          fillcolor = LIGHT_GREEN;
                          break;
        case INT_NUM    :     color =  DARK_ORANGE;
                          fillcolor = LIGHT_ORANGE;
                          break;
        case KEY_CHAR   :     color =   DARK_RED;
                          fillcolor = LIGHT_PINK;
                          break;
        case UNDEF_TOKEN:     color =      BLACK;
                          fillcolor = LIGHT_GREY;
                          break;

        default: assert(false && "default case in graphviz_dump_token()");
                 break;
    }
    graphviz_describe_token(code, stream, dumping_pos, color, fillcolor);
}

void graphviz_describe_token(const source *const code, FILE *const stream, const int dumping_pos, const GRAPHVIZ_COLOR     color,
                                                                                                  const GRAPHVIZ_COLOR fillcolor)
{
    assert(code        != nullptr);
    assert(stream      != nullptr);

    const token *cur_token = lexis_data + dumping_pos;
    char         cur_token_value[GRAPHVIZ_SIZE_CMD];

    get_token_dump_message(cur_token, cur_token_value);

    fprintf(stream, "node%d[color=\"%s\", fillcolor=\"%s\", label=\"{type=%s\\n | token_beg=%d\\n | token_line=%d\\n | %s}\"]\n",
                         dumping_pos,
                                    GRAPHVIZ_COLOR_NAMES[color],
                                                      GRAPHVIZ_COLOR_NAMES[fillcolor],
                                                                          TOKEN_TYPE_NAMES[cur_token->type],
                                                                                            cur_token->token_beg,
                                                                                                                cur_token->token_line,
                                                                                                                       cur_token_value);
}

void get_token_dump_message(const token *const cur_token, char *const token_message)
{
    assert(cur_token     != nullptr);
    assert(token_message != nullptr);

    switch(cur_token->type)
    {
        case INSTRUCTION: sprintf(token_message, "value.instruction=%s" , ASM_CMD_NAMES[cur_token->value.instruction]);
                          break;
        case REG_NAME   : sprintf(token_message, "value.reg_num=%s"     , REGISTER_NAMES[cur_token->value.reg_num]);
                          break;
        case INT_NUM    : sprintf(token_message, "value.int_num=%d"     , cur_token->value.int_num);
                          break;
        case KEY_CHAR   : sprintf(token_message, "value.key=\'%c\'"     , cur_token->value.key);
                          break;
        case UNDEF_TOKEN: sprintf(token_message, "value.token_len=%d"   , cur_token->value.token_len);
                          break;

        default: assert(false && "default case in get_token_dump_message()");
                 break;
    }
}

void system_graphviz_dump(char *const dump_txt, char *const dump_png)
{
    assert(dump_txt != nullptr);
    assert(dump_png != nullptr);

    dump_txt[GRAPHVIZ_SIZE_FILE - 1] = '\0';
    dump_png[GRAPHVIZ_SIZE_FILE - 1] = '\0';

    char cmd[GRAPHVIZ_SIZE_CMD] = "";

    sprintf       (cmd, "dot %s -T png -o %s", dump_txt, dump_png);
    system        (cmd);
    log_message   ("<img src=%s>\n", dump_png);
}