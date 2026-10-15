#include "asm.h"

#define DEFAULT_CODE_FILE_NAME "code.bin"
#define DEFAULT_TEXT_FILE_NAME "text_disassed_commands.txt"

enum {MAIN_NORMAL = 0, MAIN_CODE_SEGMENT_ERROR = 1};   // return of main()

#define Print_error_message(message, object)                                         \
    fprintf(stderr, "%s: %s(): %s - %s, line %d\n", __FILE__, __func__, message, object, __LINE__);

int* GetCodeSegment(const char* filename);

int DisassembleCode(int* code_segment, const char* filename);

const char* GetRegisterName(int register_number, int registers_count);

int main(int argc, char* argv[]) {
    const char* text_file_name = NULL;
    const char* code_file_name = NULL;
    if (argc >= 3) {
        text_file_name = argv[1];
        code_file_name = argv[2];
    }
    else {
        text_file_name = DEFAULT_TEXT_FILE_NAME;
        code_file_name = DEFAULT_CODE_FILE_NAME;
    }

    int* code_segment = GetCodeSegment(code_file_name);
    if (code_segment == NULL)
        return MAIN_CODE_SEGMENT_ERROR;

    DisassembleCode(code_segment, text_file_name);

    free(code_segment);
}


int* GetCodeSegment(const char* filename) {
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


int DisassembleCode(int* code_segment, const char* filename) { // TODO binsearch
    FILE* text_file = fopen(filename, "w"); // TODO return error
    if (text_file == NULL) {
        Print_error_message("Failed to open with \"w\" permission", filename);
        return 1;
    }

    int registers_count = sizeof(Registers) / sizeof(Register_t);
    int operations_count = sizeof(Operations) / sizeof(Operation_t);

    const char* register_name = NULL;

    for(int ip = 0; ; ip++) {
        bool known_operation = false;

        for (int i_op = 0; i_op < operations_count; i_op++) {
            if (Operations[i_op].operation_number == code_segment[ip]) {
                known_operation = true;

                fprintf(text_file, "%s", Operations[i_op].operation_name);
                for (int i = 0; i < Operations[i_op].arguments_count; i++) {
                    if (Operations[i_op].operation_number == PUSH_FROM_REGISTER) {
                        register_name = GetRegisterName(code_segment[++ip], registers_count);

                        if (register_name != NULL)
                            fprintf(text_file, " %s", register_name);
                        else{
                            printf("disassembler.c: Unknown register with number %d\n\n", code_segment[ip]);
                            fclose(text_file);
                            return 1;
                        }
                    }
                    else
                        fprintf(text_file, " %d", code_segment[++ip]);
                }
                fprintf(text_file, "\n");

                if (code_segment[ip] == HLT) { //FIXME
                    fclose(text_file);
                    return 0;
                }

                break;
            }
        }

        if (known_operation == false) {
            printf("disassembler.c: Unknown command with number %d\n\n", code_segment[ip]);
            fclose(text_file);
            return 1;
        }
    }
}


const char* GetRegisterName(int register_number, int registers_count) {
    for (int i_reg = 0; i_reg < registers_count; i_reg++) {
        if (Registers[i_reg].register_number == register_number)
            return Registers[i_reg].register_name;
    }

    return NULL;
}
