#src
AST		  = src/ast
FRONTEND  = src/frontend
MIDDLEEND = src/middleend
BACKEND   = src/backend
DISCODER  = src/discoder
#----------------------------------------------------------------------------------------------------
#lib
LOG      = lib/logs/log
STACK    = lib/stack/stack
RW       = lib/read_write/read_write
ALG      = lib/algorithm/algorithm

LIB_CPP  = $(LOG).cpp $(STACK).cpp $(RW).cpp $(ALG).cpp
LIB_H	 = $(LOG).h   $(STACK).h   $(RW).h   $(ALG).h
#----------------------------------------------------------------------------------------------------

CFLAGS = -D _DEBUG -ggdb3 -std=c++20 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer -fPIE -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr -pie -Wlarger-than=8192 -Wstack-usage=8192

.PHONY: frontend, discoder, backend, middleend

frontend:  $(FRONTEND).cpp  $(AST).cpp $(LIB_CPP) $(FRONTEND).h $(AST).h $(LIB_H)
	g++    $(FRONTEND).cpp  $(AST).cpp $(LIB_CPP) $(CFLAGS) -o $@

backend:   $(BACKEND).cpp   $(AST).cpp $(LIB_CPP) $(BACKEND).h  $(AST).h $(LIB_H)
	g++    $(BACKEND).cpp   $(AST).cpp $(LIB_CPP) $(CFLAGS) -o $@

discoder:  $(DISCODER).cpp  $(AST).cpp $(LIB_CPP) $(DISCODER).h  $(AST).h $(LIB_H)
	g++    $(DISCODER).cpp  $(AST).cpp $(LIB_CPP) $(CFLAGS) -o $@

middleend: $(MIDDLEEND).cpp $(AST).cpp $(LIB_CPP) $(MIDDLEEND).h $(AST).h $(LIB_H)
	g++    $(MIDDLEEND).cpp $(AST).cpp $(LIB_CPP) $(CFLAGS) -o $@