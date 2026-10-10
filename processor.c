#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>    // TODO calc type (double or int) or use already done STACK_TYPE or equal it for understanding

#include "Stack.c"

#define DEFAULT_CODE_FILE_NAME "code.bin"
#define REGISTERS_COUNT 5

enum {MAIN_NORMAL = 0, MAIN_CODE_BUFFER_ERROR = 1,
      MAIN_CODE_SEGMENT_ERROR = 2};                     // return of main()

#define Print_error_message(message, object)                                         \
    fprintf(stderr, "%s(): %s - %s, line %d\n", __func__, message, object, __LINE__);

struct Process_t {  // REVIEW struct process
    int* code_segment;
    int ip;
    Stack_t* numbers_stk;
    int registers[REGISTERS_COUNT];  // REVIEW register
};

struct Command_t {
    int command_number;
    const char* command_name;
    Stack_report_t (*CommandAction)(Process_t*);
};

Process_t* ProcessInit(const char* filename);

void ProcessDestroy(Process_t* process);

int* GetCodeSegment(const char* filename);

void ExecuteCommand(Process_t* process); // STUB add &result field for OUT and some new functions

void PrintMalfunction(Stack_report_t last_function_success);

Stack_report_t Halt(Process_t* process);

Stack_report_t Out(Process_t* process);

Stack_report_t Push(Process_t* process);

Stack_report_t Add(Process_t* process);

Stack_report_t Subtract(Process_t* process);

Stack_report_t Multiply(Process_t* process);

Stack_report_t Divide(Process_t* process);

Stack_report_t PushFromRegister(Process_t* process);

Stack_report_t SetRegister(Process_t* process);

Stack_report_t Jump(Process_t* process); // REVIEW Jump

Stack_report_t JumpAbove(Process_t* process);

Stack_report_t JumpEqualAbove(Process_t* process);

Stack_report_t JumpBelow(Process_t* process);

Stack_report_t JumpEqualBelow(Process_t* process);

Stack_report_t JumpEqual(Process_t* process);

Stack_report_t JumpNotEqual(Process_t* process);

static Command_t Commands[] = {
    {-2, "HLT", &Halt}, {-1, "OUT", &Out}, {0, "PUSH", &Push},
    {1, "ADD", &Add}, {2, "SUB", &Subtract}, {3, "MUL", &Multiply}, {4, "DIV", &Divide},
    {10, "PUSH", &PushFromRegister}, {11, "SET", &SetRegister}, {20, "JUMP", &Jump},
    {21, "JA", &JumpAbove}, {22, "JAE", &JumpEqualAbove}, {23, "JB", &JumpBelow},
    {24, "JBE", &JumpEqualBelow}, {25, "JE", &JumpEqual}, {26, "JNE", &JumpNotEqual}
};


int main(int argc, char* argv[]) {
    const char* filename = NULL;
    if (argc >= 2)
        filename = argv[1];
    else
        filename = DEFAULT_CODE_FILE_NAME;

    Process_t* process = ProcessInit(filename);

    for(process->ip = 0; ; process->ip++) {
        ExecuteCommand(process);
    }
}


Process_t* ProcessInit(const char* filename) {
    Process_t* process = (Process_t*)malloc(sizeof(Process_t));

    Process_t process_initer = {};
    *process = process_initer;

    process->code_segment = GetCodeSegment(filename);  // REVIEW get filename from arv and use console
    if (process->code_segment == NULL) {
        free(process);
        return NULL;
    }

    process->numbers_stk = StackInit(0);
    if (process->numbers_stk->error != STACK_NORMAL) {
        free(process->code_segment); free(process);
        return NULL;
    }

    return process;
}


void ProcessDestroy(Process_t* process) {
    free(process->code_segment);
    StackDestroy(process->numbers_stk);
    free(process);
}


int* GetCodeSegment(const char* filename) { // REVIEW binary
    assert(filename != NULL);

    struct stat file_stat = {};
    stat(filename, &file_stat);

    int* code_segment = (int*)calloc(file_stat.st_size, sizeof(int));
    if (code_segment == NULL) {
        Print_error_message("Failed to allocate memory", "code_segment");
        return NULL;
    }

    int file_descriptor = open(filename, O_RDONLY | O_BINARY);
    if (file_descriptor < 0) {
        Print_error_message("Failed to open with mode O_RDONLY", filename);
        free(code_segment);
        return NULL;
    }

    size_t code_size = read(file_descriptor, code_segment, file_stat.st_size);
    if (code_size < 0) {
        Print_error_message("Failed to read", filename);
        free(code_segment);
        close(file_descriptor);
        return NULL;
    }

    close(file_descriptor);

    return code_segment;
}


void ExecuteCommand(Process_t* process) { // TODO binsearch
    int commands_count = sizeof(Commands) / sizeof(Command_t);
    bool known_command = false;

    for (int i_cmd = 0; i_cmd < commands_count; i_cmd++) {
        if (Commands[i_cmd].command_number == process->code_segment[process->ip]) {
            known_command = true;

            Stack_report_t last_command_success = Commands[i_cmd].CommandAction(process);
            if (last_command_success != SUCCESS) {
                printf("the command %s failed with code %d\n\n",
                       Commands[i_cmd].command_name, last_command_success);
                Halt(process); // STUB
            }

            break;
        }
    }

    if (known_command == false) {
        printf("Unknown command with number %d\n\n", process->code_segment[process->ip]);
        Halt(process);
    }
}


#define CHECK_SUCCESS(stack)                           \
    if (stack->last_function_success != SUCCESS) {     \
        PrintMalfunction(stack->last_function_success);\
        return stack->last_function_success;           \
    }

void PrintMalfunction(Stack_report_t last_function_success) {
    switch (last_function_success) {
        case OVERFLOW:
            printf("stack overflow, impossible to memorize number\n");
            break;
        case VACUUM:
            printf("stack empty, impossible to get number\n");
            break;
        case FATAL_ERROR:
            printf("stack fatal damaged, impossible to work with it\n");
        default: break;
    }
}


Stack_report_t Halt(Process_t* process) {
    assert(process != NULL);

    ProcessDestroy(process);

    exit(0);
}

Stack_report_t Out(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 1) {
        printf("there must be at least one number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    printf("< %d >\n", a);

    return SUCCESS;
}

Stack_report_t Push(Process_t* process) {
    assert(process != NULL);

    process->ip++;

    StackPush(process->numbers_stk, process->code_segment[process->ip]);
    CHECK_SUCCESS(process->numbers_stk);

    return SUCCESS;
}

Stack_report_t Add(Process_t* process) { // REVIEW stack errors check
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    StackPush(process->numbers_stk, a + b);
    CHECK_SUCCESS(process->numbers_stk);

    return SUCCESS;
}

Stack_report_t Subtract(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    StackPush(process->numbers_stk, b - a);
    CHECK_SUCCESS(process->numbers_stk);

    return SUCCESS;
}

Stack_report_t Multiply(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    StackPush(process->numbers_stk, a * b);
    CHECK_SUCCESS(process->numbers_stk);

    return SUCCESS;
}

Stack_report_t Divide(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    StackPush(process->numbers_stk, b / a);
    CHECK_SUCCESS(process->numbers_stk);

    return SUCCESS;
}

Stack_report_t PushFromRegister(Process_t* process) {
    assert(process != NULL);

    process->ip++;

    if ((process->code_segment[process->ip] < 0) || (process->code_segment[process->ip] >= REGISTERS_COUNT)) {
        printf("incorrect register number - %d\n", process->code_segment[process->ip]);
        return FATAL_ERROR;
    }

    StackPush(process->numbers_stk, process->registers[process->code_segment[process->ip]]);
    CHECK_SUCCESS(process->numbers_stk);

    return SUCCESS;
}

Stack_report_t SetRegister(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 1) {
        printf("there must be at least one number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    process->ip++;

    if ((process->code_segment[process->ip] < 0) || (process->code_segment[process->ip] >= REGISTERS_COUNT)) {
        printf("incorrect register number %d\n", process->code_segment[process->ip]);
        return FATAL_ERROR;
    }

    process->registers[process->code_segment[process->ip]] = a;

    return SUCCESS;
}

Stack_report_t Jump(Process_t* process) { // STUB macros
    assert(process != NULL);

    process->ip++;

    if (process->code_segment[process->ip] < 0) {
        printf("incorrect label %d\n", process->code_segment[process->ip]);
        return FATAL_ERROR;
    }

    process->ip = (process->code_segment[process->ip] - 1); // -1 to compensate ip++

    return SUCCESS;
}

Stack_report_t JumpAbove(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    if (b < a)
        return Jump(process);

    process->ip++; // skip argument of Jump

    return SUCCESS;
}

Stack_report_t JumpEqualAbove(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    if (b <= a)
        return Jump(process);

    process->ip++; // skip argument of Jump

    return SUCCESS;
}

Stack_report_t JumpBelow(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    if (b > a)
        return Jump(process);

    process->ip++; // skip argument of Jump

    return SUCCESS;
}

Stack_report_t JumpEqualBelow(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    if (b >= a)
        return Jump(process);

    process->ip++; // skip argument of Jump

    return SUCCESS;
}

Stack_report_t JumpEqual(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    if (b == a)
        return Jump(process);

    process->ip++; // skip argument of Jump

    return SUCCESS;
}

Stack_report_t JumpNotEqual(Process_t* process) {
    assert(process != NULL);

    if (process->numbers_stk->size < 2) {
        printf("there must be at least two number on stack to execute the command\n");
        return VACUUM;
    }

    int a = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);
    int b = StackPop(process->numbers_stk);
    CHECK_SUCCESS(process->numbers_stk);

    if (b != a)
        return Jump(process);

    process->ip++; // skip argument of Jump

    return SUCCESS;
}
