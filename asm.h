#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#define MAX_LEXEME_SIZE 100

#define HLT -2
#define PUSH 0
#define PUSH_FROM_REGISTER 10
#define JUMP 20
#define JNE 26
#define CALL 30

struct Operation_t {
    int operation_number;
    const char operation_name[MAX_LEXEME_SIZE];
    int arguments_count;
};

struct Register_t {
    int register_number;
    const char register_name[MAX_LEXEME_SIZE];
};

Operation_t Operations[] = {{HLT /*-2*/, "HLT", 0}, {-1, "OUT", 0}, {PUSH /*0*/, "PUSH", 1},
                            {1, "ADD", 0}, {2, "SUB", 0}, {3, "MUL", 0}, {4, "DIV", 0},
                            {5, "SQRT", 0},
                            {PUSH_FROM_REGISTER /*10*/, "PUSH", 1}, {11, "SET", 1}, {JUMP /*20*/, "JUMP", 1},
                            {21, "JA", 1}, {22, "JAE", 1}, {23, "JB", 1},
                            {24, "JBE", 1}, {25, "JE", 1}, {JNE /*26*/, "JNE", 1},
                            {CALL, "CALL", 1}, {31, "RET", 2},
                            {50, "IN", 0}
                           };

Register_t Registers[] = {{1, "AX"}, {2, "BX"}, {3, "CX"}, {4, "DX"}};
