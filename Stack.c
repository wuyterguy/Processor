#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

//#define DEBUG

#ifdef DEBUG
    #define ONDEB(...) __VA_ARGS__
#else
    #define ONDEB(...)
#endif

#define CANARY_PROTECTION

#ifdef CANARY_PROTECTION
    #define ONCAN(...) __VA_ARGS__
#else
    #define ONCAN(...)
#endif

#define HASH_PROTECTION

#ifdef HASH_PROTECTION
    #define ONHASH(...) __VA_ARGS__
#else
    #define ONHASH(...)
#endif

static FILE* log_file = stdout;
#define STACK_LOG_FILE_NAME "stack_log.txt"

/*#define Print_error_message(message, object)                                         \
    fprintf(stderr, "%s(): %s - %s, line %d\n", __func__, message, object, __LINE__);*/
#undef OVERFLOW

typedef enum {STACK_NORMAL =            0b0000000, STACK_DATA_ERROR =         0b0000001,
              STACK_CAPACITY_ERROR =    0b0000010, STACK_SIZE_ERROR =         0b0000100,
              STACK_LEFT_CANARY_ERROR = 0b0001000, STACK_RIGHT_CANARY_ERROR = 0b0010000,
              STACK_DATA_HASH_ERROR =   0b0100000, STACK_HANDLE_HASH_ERROR =  0b1000000} Stack_err_t;
typedef enum {SUCCESS = 0, FATAL_ERROR = 1, OVERFLOW = -1, VACUUM = -2}                  Stack_report_t;
#define FATAL true
#define NOT_FATAL false
/*typedef enum {PUSH_NORMAL = 0} Push_err_t; // return of StackPush()
typedef enum {POP_NORMAL = 0}  Pop_err_t;  // return of StackPop()*/

#define INT 1
#define FLOAT 2
#define DOUBLE 3
#define POINTER 4

#define STACK_TYPE INT

#if STACK_TYPE == INT
    #define Stack_type_t int
    #define STACK_TYPE_SPECIFIER "%d"
    #define POISON 1666667
#elif STACK_TYPE == FLOAT
    #define Stack_type_t float
    #define STACK_TYPE_SPECIFIER "%f"
    #define POISON NAN
#elif STACK_TYPE == DOUBLE
    #define Stack_type_t double
    #define STACK_TYPE_SPECIFIER "%lf"
    #define POISON NAN
#elif STACK_TYPE == POINTER
    #define Stack_type_t void*
    #define STACK_TYPE_SPECIFIER "%p"
    #define POISON NULL
#else
    #define Stack_type_t long long int
    #define STACK_TYPE_SPECIFIER "%lld"
    #define POISON 166666666667
#endif


typedef char Byte_t;
typedef unsigned long long int Canary_t;

#define CANARY_SPECIFIER "0x%.16llX"
#define CANARY_BYTE_SIZE sizeof(Canary_t)
#define LEFT_CANARY         0xCA14A2E7CE111A27ull
#define RIGHT_CANARY        0xB12D1412E17B12D5ull
#define LEFT_HANDLE_CANARY  0xEA1AE2EABEE11A98ull
#define RIGHT_HANDLE_CANARY 0xAE4D1DC2F65B134Bull

#define DEFAULT_CAPACITY 10ull
#define CAPACITY_FACTOR 1.5

#define WRAP_IN_STR(not_str) #not_str
#define IN_STR(not_str) WRAP_IN_STR(not_str)

#define STACK_INIT(stack_name, capacity)                                  \
    StackInit(capacity ONDEB(, #stack_name, __FILE__, __func__, __LINE__))
#define STACK_VERIFY(stack)               \
    StackVerify(stack, __func__, __LINE__)

#define ON 1
#define OFF 0

#define COLOR_SWITCH OFF           // switch color output

#if COLOR_SWITCH == ON
    #define RED "\x1b[31m"
    #define GRE "\x1b[32m"
    #define YEL "\x1b[33m"
    #define BLU "\x1b[34m"
    #define MAG "\x1b[35m"
    #define END "\x1b[0m"

    /* Color: is replaced by s colored in color */
    #define SetColor(color) fprintf(log_file, color);
    #define EndColor fprintf(log_file, END);
    #define Color(s, color) color s END
#else
    #define RED ""
    #define BLU ""
    #define GRE ""
    #define YEL ""
    #define MAG ""
    #define END ""

    #define SetColor(color)
    #define EndColor
    #define Color(s, color) s
#endif

struct Stack_t {
    ONCAN(Canary_t left_handle_canary;)

    ONDEB(
        const char* stack_name;
        const char* file_name;
        const char* function_name;
        int line_number;
    )

    Stack_type_t* data;
    ssize_t capacity; // capasity unsigned chech prisvaemoe znachenie na polojitelnost
    size_t size;
    Stack_err_t error;
    bool error_fatality;
    Stack_report_t last_function_success;

    ONHASH(unsigned long long int data_hash;)
    ONHASH(unsigned long long int handle_hash;)

    ONCAN(Canary_t right_handle_canary;)
};

Stack_t* StackInit(ssize_t capacity ONDEB(, const char* stack_name, const char* file_name,
                                          const char* function_name, int line_number));

void StackDestroy(Stack_t* stack);

void StackFillPoison(Stack_t* stack, int start_offset);

Stack_err_t StackVerify(Stack_t* stack, const char* calling_function_name, int from_line);

Stack_err_t DiagnoseError(Stack_t* stack, const char* calling_function_name, int from_line);

void StackDump(const Stack_t* stack);

void StackPrintError(const Stack_t* stack);

Stack_report_t StackPush(Stack_t* stack, Stack_type_t new_item);

Stack_type_t StackPop(Stack_t* stack);

Stack_report_t StackIncreaseCapacity(Stack_t* stack);

Stack_report_t StackReduceCapacity(Stack_t* stack);

Stack_type_t* StackRealloc(Stack_type_t* old_data, ssize_t new_capacity);

ONCAN(
    Canary_t* LeftCanaryPtr(Stack_t* stack);
    Canary_t* RightCanaryPtr(Stack_t* stack);
    void StackSetDataCanary(Stack_t* stack);
)

ONHASH(
    unsigned long long int StackDataHash(Stack_t* stack);
    unsigned long long int StackHandleHash(Stack_t* stack);
    unsigned long long int StackHash(unsigned char* from_ptr, size_t hashing_size);
)


Stack_t* StackInit(ssize_t capacity ONDEB(, const char* stack_name, const char* file_name,
                                          const char* function_name, int line_number)) {
    ONDEB(assert(stack_name != NULL); assert(file_name != NULL); assert(function_name != NULL);)

    Stack_t* stack = (Stack_t*)malloc(sizeof(Stack_t));

    if (stack == NULL)
        return NULL;

    if (capacity < 1)
        capacity = DEFAULT_CAPACITY;

    ONCAN(stack->left_handle_canary = LEFT_HANDLE_CANARY;)

    ONDEB(
        stack->stack_name = stack_name;
        stack->file_name = file_name;
        stack->function_name = function_name;
        stack->line_number = line_number;
    )

    stack->data = NULL;
    stack->capacity = capacity;
    stack->size = 0;
    stack->error = STACK_NORMAL;
    stack->error_fatality = NOT_FATAL;
    stack->last_function_success = SUCCESS;
    stack->data = StackRealloc(NULL, stack->capacity);
    if (stack->data != NULL)
        ONCAN(StackSetDataCanary(stack);)

    ONDEB(StackFillPoison(stack, 0);)

    ONHASH(stack->data_hash = StackDataHash(stack);)
    ONHASH(stack->handle_hash = StackHandleHash(stack);)

    ONCAN(stack->right_handle_canary = RIGHT_HANDLE_CANARY;)

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return NULL;

    return stack;
}


void StackDestroy(Stack_t* stack) {
    assert(stack != NULL);

    (void)STACK_VERIFY(stack);

    if (stack->data != NULL) {
        ONDEB(StackFillPoison(stack, 0);)
        free((Byte_t*)stack->data ONCAN(- CANARY_BYTE_SIZE));
    }

    free(stack);
}


void StackFillPoison(Stack_t* stack, int start_offset) {
    assert(stack != NULL);

    if (stack->data != NULL) {
        for (Stack_type_t* in_stack_ptr = stack->data + start_offset;
             (in_stack_ptr - stack->data) < stack->capacity; in_stack_ptr++)
            *in_stack_ptr = POISON;
    }
}


Stack_err_t StackVerify(Stack_t* stack, const char* calling_function_name, int from_line) {
    assert(stack != NULL); assert(calling_function_name != NULL);
    ONDEB(assert(log_file != NULL);)

    ONDEB(fprintf(log_file, ">>>>>\n");)

    (void)DiagnoseError(stack, calling_function_name, from_line);

    ONDEB(
        fprintf(log_file, "\n");
        StackDump(stack);
    )

    ONDEB(fprintf(log_file, "<<<<<\n\n");)

    return stack->error;
}


Stack_err_t DiagnoseError(Stack_t* stack, const char* calling_function_name, int from_line) {
    assert(stack != NULL); assert(calling_function_name != NULL);
    ONDEB(assert(log_file != NULL);)

    ONDEB(SetColor(RED);)

    ONCAN(
        if ((stack->left_handle_canary != LEFT_HANDLE_CANARY) ||
            (stack->right_handle_canary != RIGHT_HANDLE_CANARY)) {
            ONDEB(
                fprintf(log_file, "Diagnostic from %s(), line %d: Fatal error - handle information has been damaged, program aborted\n",
                        calling_function_name, from_line);
                fprintf(log_file, "Canaries values: Left " CANARY_SPECIFIER ", Right " CANARY_SPECIFIER "\n",
                        stack->left_handle_canary, stack->right_handle_canary);
                fprintf(log_file, "Must be:         Left " CANARY_SPECIFIER ", Right " CANARY_SPECIFIER "\n",
                        LEFT_HANDLE_CANARY, RIGHT_HANDLE_CANARY);
            )
            ONDEB(EndColor;)
            abort();
        }
    )

    ONHASH(
        unsigned long long int handle_hash = StackHandleHash(stack);
        if (handle_hash != stack->handle_hash) {
            ONDEB(
                fprintf(log_file, "Diagnostic from %s(), line %d: Fatal error - incorrect handle hash, program aborted\n",
                        calling_function_name, from_line);
                fprintf(log_file, "Its value %lld, Must be %lld\n", handle_hash, stack->handle_hash);
            )
            ONDEB(EndColor;)
            abort();
        }
    )

    stack->error = STACK_NORMAL;
    stack->error_fatality = NOT_FATAL;

    if (stack->data == NULL) {
        ONDEB(fprintf(log_file, "Diagnostic from %s(), line %d: Fatal error - data is lost, may be it did not allocated or was free\n", calling_function_name, from_line);)
        stack->error = (Stack_err_t)(stack->error | STACK_DATA_ERROR);
        stack->error_fatality = FATAL;
    }

    ONCAN(
        if (stack->data != NULL) {
            if (*LeftCanaryPtr(stack) != LEFT_CANARY) {
                ONDEB(
                    fprintf(log_file, "Diagnostic from %s(), line %d: Warning - left data canary has been damaged\n",
                           calling_function_name, from_line);
                    fprintf(log_file, "Its value " CANARY_SPECIFIER ", must be " CANARY_SPECIFIER "\n", *LeftCanaryPtr(stack), LEFT_CANARY);
                )
                stack->error = (Stack_err_t)(stack->error | STACK_LEFT_CANARY_ERROR);
            }
            if (*RightCanaryPtr(stack) != RIGHT_CANARY) {
                ONDEB(
                    fprintf(log_file, "Diagnostic from %s(), line %d: Warning - right data canary has been damaged\n",
                           calling_function_name, from_line);
                    fprintf(log_file, "Its value " CANARY_SPECIFIER ", must be " CANARY_SPECIFIER "\n", *RightCanaryPtr(stack), RIGHT_CANARY);
                )
                stack->error = (Stack_err_t)(stack->error | STACK_RIGHT_CANARY_ERROR);
            }
        }
    )

    ONHASH(
        if (stack->data != NULL) {
            unsigned long long int data_hash = StackDataHash(stack);
            if (data_hash != stack->data_hash) {
                ONDEB(
                    fprintf(log_file, "Diagnostic from %s(), line %d: Warning - incorrect data hash\n",
                           calling_function_name, from_line);
                    fprintf(log_file, "Its value %lld, Must be %lld\n", data_hash, stack->data_hash);
                )
                stack->error = (Stack_err_t)(stack->error | STACK_DATA_HASH_ERROR);
            }
        }
    )

    if (stack->capacity < 1) {
        ONDEB(fprintf(log_file, "Diagnostic from %s(), line %d: Fatal error - invalid stack capacity, it must be positive\n", calling_function_name, from_line);)
        stack->error = (Stack_err_t)(stack->error | STACK_CAPACITY_ERROR);
        stack->error_fatality = FATAL;
    }
    if (stack->size > stack->capacity) {
        ONDEB(fprintf(log_file, "Diagnostic from %s(), line %d: Fatal error - invalid stack size, it must not exceed capacity\n", calling_function_name, from_line);)
        stack->error = (Stack_err_t)(stack->error | STACK_SIZE_ERROR);
        stack->error_fatality = FATAL;
    }
    if (stack->size < 0) {
        ONDEB(fprintf(log_file, "Diagnostic from %s(), line %d: Fatal error - invalid stack size, it can not be negative\n", calling_function_name, from_line);)
        stack->error = (Stack_err_t)(stack->error | STACK_SIZE_ERROR);
        stack->error_fatality = FATAL;
    }

    ONDEB(SetColor(GRE);)
    if (stack->error == STACK_NORMAL) {
        ONDEB(fprintf(log_file, "Diagnostic from %s(), line %d: No fatal damage\n", calling_function_name, from_line);)
    }

    ONDEB(EndColor;)

    return stack->error;
}


void StackDump(const Stack_t* stack) {
    assert(stack != NULL);
    assert(log_file != NULL);

    ONDEB(
        fprintf(log_file, "%s[%p] with type %s from file %s, func %s(), line %d:\n\n",
               stack->stack_name, stack->data, IN_STR(Stack_type_t),
               stack->file_name, stack->function_name, stack->line_number);
    )

    fprintf(log_file, "capacity = %d\nsize = %d\n\n", stack->capacity, stack->size);
    StackPrintError(stack);

    fprintf(log_file, "data = %p:\n", stack->data);
    if (stack->data != NULL) {
        for (Stack_type_t* in_stack_ptr = stack->data;
             (in_stack_ptr - stack->data) < stack->capacity; in_stack_ptr++) {
            if ((in_stack_ptr - stack->data) < stack->size) fprintf(log_file, " * ");
            else fprintf(log_file, "   ");

            fprintf(log_file, STACK_TYPE_SPECIFIER " [%zu]", *in_stack_ptr, in_stack_ptr - stack->data);

            if (*in_stack_ptr == POISON) fprintf(log_file, " (POISON)");
            fprintf(log_file, "\n");
        }
    }
    else
        fprintf(log_file, "   Impossible to print data\n");
}


#define STACK_CHECK_ERROR(errors_code, checking_error)\
    if ((errors_code & checking_error) != 0) {        \
        fprintf(log_file, "   " #checking_error "\n");           \
    }

void StackPrintError(const Stack_t* stack) {
    assert(stack != NULL);
    assert(log_file != NULL);

    fprintf(log_file, "error:\n");

    if (stack->error == STACK_NORMAL)
        fprintf(log_file, "   no error\n");
    else {
        SetColor(RED);

        STACK_CHECK_ERROR(stack->error, STACK_DATA_ERROR);
        STACK_CHECK_ERROR(stack->error, STACK_CAPACITY_ERROR);
        STACK_CHECK_ERROR(stack->error, STACK_SIZE_ERROR);
        ONCAN(
            STACK_CHECK_ERROR(stack->error, STACK_LEFT_CANARY_ERROR);
            STACK_CHECK_ERROR(stack->error, STACK_RIGHT_CANARY_ERROR);
        )
        ONHASH(
            STACK_CHECK_ERROR(stack->error, STACK_DATA_HASH_ERROR);
            STACK_CHECK_ERROR(stack->error, STACK_HANDLE_HASH_ERROR);
        )

        EndColor;
    }

    fprintf(log_file, "\n");
}


Stack_report_t StackPush(Stack_t* stack, Stack_type_t new_item) {
    assert(stack != NULL);

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return stack->last_function_success = FATAL_ERROR;

    if ((stack->size + 1) > stack->capacity) {
        Stack_report_t increase_res = SUCCESS;
        if ((increase_res = StackIncreaseCapacity(stack)) != SUCCESS)
            return stack->last_function_success = increase_res;
    }

    stack->data[stack->size] = new_item;
    stack->size++;
    ONHASH(stack->data_hash = StackDataHash(stack);)
    ONHASH(stack->handle_hash = StackHandleHash(stack);)

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return stack->last_function_success = FATAL_ERROR;

    return stack->last_function_success = SUCCESS;
}


Stack_type_t StackPop(Stack_t* stack) {
    assert(stack != NULL);
    ONDEB(assert(log_file != NULL);)

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL)) {
        stack->last_function_success = FATAL_ERROR;
        return POISON;
    }

    if (stack->size < 1) {
        ONDEB(
            SetColor(YEL);
            fprintf(log_file, "note from %s(), line %d: can not do pop because stack is empty, pop was cancelled\n\n",
                   __func__, __LINE__, stack->stack_name);
            EndColor;
        )
        stack->last_function_success = VACUUM;
        return POISON;
    }

    stack->size--;
    Stack_type_t result = stack->data[stack->size];
    ONDEB(stack->data[stack->size] = POISON);
    ONHASH(stack->data_hash = StackDataHash(stack);)
    ONHASH(stack->handle_hash = StackHandleHash(stack);)

    if (((ssize_t)(stack->size * pow(CAPACITY_FACTOR, 2)) < stack->capacity) && (stack->size > 0)) {
        Stack_report_t reduce_res = SUCCESS;
        if ((reduce_res = StackReduceCapacity(stack)) != SUCCESS) {
            stack->last_function_success = reduce_res;
            return result;
        }
    }

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL)) {
        stack->last_function_success = FATAL_ERROR;
        return result;
    }

    stack->last_function_success = SUCCESS;
    return result;
}


Stack_report_t StackIncreaseCapacity(Stack_t* stack) {
    assert(stack != NULL);
    ONDEB(assert(log_file != NULL);)

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return FATAL_ERROR;

    ssize_t new_capacity = (ssize_t)(stack->capacity * CAPACITY_FACTOR) + 1;
        Stack_type_t* new_data = StackRealloc(stack->data, new_capacity);

        if (new_data == NULL) {
            ONDEB(
                SetColor(YEL);
                fprintf(log_file, "note from %s(), line %d: can not grow up capacity for %s, push was cancelled\n\n",
                       __func__, __LINE__, stack->stack_name);
                EndColor;
            )
            return OVERFLOW;
        }

        stack->capacity = new_capacity;
        stack->data = new_data;
        ONCAN(StackSetDataCanary(stack);)
        ONDEB(StackFillPoison(stack, stack->size);)

        ONHASH(stack->handle_hash = StackHandleHash(stack);)

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return FATAL_ERROR;

    return SUCCESS;
}


Stack_report_t StackReduceCapacity(Stack_t* stack) {
    assert(stack != NULL);

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return FATAL_ERROR;

    ssize_t new_capacity = (ssize_t)(stack->capacity / CAPACITY_FACTOR);
        Stack_type_t* new_data = StackRealloc(stack->data, new_capacity);

        if (new_data != NULL) {
            stack->capacity = new_capacity;
            stack->data = new_data;
            ONCAN(StackSetDataCanary(stack);)
            ONDEB(StackFillPoison(stack, stack->size);)

            ONHASH(stack->handle_hash = StackHandleHash(stack);)
        }

    if ((STACK_VERIFY(stack) != STACK_NORMAL) && (stack->error_fatality == FATAL))
        return FATAL_ERROR;

    return SUCCESS;
}


Stack_type_t* StackRealloc(Stack_type_t* old_data, ssize_t new_capacity) {
    Byte_t* byte_old_data = (Byte_t*)old_data;

    ONCAN(
    if (old_data != NULL)
        byte_old_data -= CANARY_BYTE_SIZE;
    )

    void* buffer_start = realloc(byte_old_data, (new_capacity * sizeof(Stack_type_t)) ONCAN(+ (2 * CANARY_BYTE_SIZE)));

    if (buffer_start == NULL)
        return NULL;

    return (Stack_type_t*)((Byte_t*)buffer_start ONCAN(+ CANARY_BYTE_SIZE));
}


ONCAN(
    Canary_t* LeftCanaryPtr(Stack_t* stack) {
        assert(stack != NULL);

        return (Canary_t*)(stack->data) - 1;
    }

    Canary_t* RightCanaryPtr(Stack_t* stack) {
        assert(stack != NULL);

        return (Canary_t*)(stack->data + stack->capacity);
    }

    void StackSetDataCanary(Stack_t* stack) {
        *LeftCanaryPtr(stack) = LEFT_CANARY;
        *RightCanaryPtr(stack) = RIGHT_CANARY;
    }
)


ONHASH(
    unsigned long long int StackDataHash(Stack_t* stack) {
        assert(stack != NULL);

        return StackHash((unsigned char*)stack->data, stack->size * sizeof(Stack_type_t));
    }

    unsigned long long int StackHandleHash(Stack_t* stack) {
        assert(stack != NULL);

        return StackHash((unsigned char*)&(stack->data),
                         (unsigned char*)&(stack->error) - (unsigned char*)&(stack->data));
    }

    unsigned long long int StackHash(unsigned char* from_ptr, size_t hashing_size) {
        assert(from_ptr != NULL);

        unsigned long long hash = 5381;

        for (unsigned char* i_ptr = from_ptr; i_ptr < from_ptr + hashing_size; i_ptr++)
            hash = (hash << 5) + hash + *i_ptr;

        return hash;
    }
)
