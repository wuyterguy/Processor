#include "asm.h" // TODO .h file

#define DEFAULT_TEXT_FILE_NAME "text_commands.txt"
#define DEFAULT_CODE_FILE_NAME "code.bin"
#define MAX_LABELS_COUNT 100
#define UNUSEING_COMMAND_NUMBER -67
#define ITERATING_COUNT 2

//#define COMMAND_SIZE = sizeof(int)
//#define ARGUMENT_SIZE = sizeof(double)
//#define ARGUMENT_SPECIFIER = "%d"

enum {MAIN_NORMAL = 0, MAIN_CODE_BUFFER_ERROR = 1};            // return of main()
typedef enum {SET_LABEL_NORMAL = 0, SET_LABEL_MEMORY_ERROR = 1,
              SET_LABEL_IDENTIFICATION_ERROR} Set_label_res_t; // return of SetLabel()

#define Print_error_message(message, object)                                         \
    fprintf(stderr, "%s: %s(): %s - %s, line %d\n", __FILE__, __func__, message, object, __LINE__);

struct Label_t {
    ssize_t label_position;
    char label_name[MAX_LEXEME_SIZE];
};

char* GetCodeBuffer(const char* filename);

int MakeCodeFile(const char* filename, char* code_buffer);

int GetArgument(char* lexeme, int registers_count, int last_command,
                Label_t labels[MAX_LABELS_COUNT], int iterating, FILE* code_file);

Set_label_res_t SetLabel(char* lexeme, size_t ip, Label_t labels[MAX_LABELS_COUNT], int iterating);

int GetCommand(char* lexeme, int operations_count, int* p_next_is_argument);

int InDigit(char* lexeme);

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

    char* code_buffer = GetCodeBuffer(text_file_name);
    if (code_buffer == NULL)
        return MAIN_CODE_BUFFER_ERROR;

    int make_code_res = MakeCodeFile(code_file_name, code_buffer);
}


char* GetCodeBuffer(const char* filename) {
    assert(filename != NULL);

    struct stat file_stat = {};
    stat(filename, &file_stat);

    char* buffer = (char*)calloc(file_stat.st_size, sizeof(char));
    if (buffer == NULL) {
        Print_error_message("Failed to allocate memory", "buffer");
        return NULL;
    }

    int file_descriptor = open(filename, O_RDONLY);
    if (file_descriptor < 0) {
        Print_error_message("Failed to open with mode O_RDONLY", filename);
        free(buffer);
        return NULL;
    }

    size_t text_size = read(file_descriptor, buffer, file_stat.st_size);
    if (text_size < 0) {
        Print_error_message("Failed to read", filename);
        free(buffer);
        close(file_descriptor);
        return NULL;
    }
    buffer[text_size] = '\0';

    close(file_descriptor);

    return buffer;
}


int MakeCodeFile(const char* filename, char* code_buffer) { // REVIEW split on funk for args and for commands
    FILE* code_file = fopen(filename, "wb"); // TODO add return
    if (code_file == NULL) {
        Print_error_message("Failed to open with \"wb\" permission", filename);
        return 1;
    }

    int registers_count = sizeof(Registers) / sizeof(Register_t);
    int operations_count = sizeof(Operations) / sizeof(Operation_t);

    Label_t labels[MAX_LABELS_COUNT] = {};
    for (int i = 0; i < MAX_LABELS_COUNT; i++) labels[i].label_position = -1;

    for (int iterating = 0; iterating < ITERATING_COUNT; iterating++) {
        char lexeme[MAX_LEXEME_SIZE] = {};
        int byte_code = 0;
        size_t offset = 0;
        size_t read_symbols_count = 0;
        size_t ip = 0;
        int next_is_argument = 0;
        int last_command = HLT;

        while(sscanf(code_buffer + offset, "%s%n", lexeme, &read_symbols_count) > 0) {
            offset += read_symbols_count;

            if (next_is_argument > 0) {
                byte_code = GetArgument(lexeme, registers_count, last_command,
                                        labels, iterating, code_file);
                next_is_argument--;

                fwrite(&byte_code, sizeof(int), 1, code_file);
                ip++;
            }
            else {
                if (lexeme[0] == ':') { // TODO labels errors
                    if (SetLabel(lexeme, ip, labels, iterating) != SET_LABEL_NORMAL) {
                        printf("Impossible to se label with name %s\n\n", lexeme);
                        fclose(code_file);
                        return 1;
                    }
                }
                else {
                    byte_code = GetCommand(lexeme, operations_count, &next_is_argument);
                    if (byte_code == UNUSEING_COMMAND_NUMBER) {
                        printf("Unknown command with name %s\n\n", lexeme);
                        fclose(code_file);
                        return 1;
                    }

                    last_command = byte_code;

                    fwrite(&byte_code, sizeof(int), 1, code_file);
                    ip++;
                }
            }
        }

        fseek(code_file, 0, SEEK_SET);
    }

    fclose(code_file);
    return 0;
}


int GetArgument(char* lexeme, int registers_count, int last_command,
                Label_t labels[MAX_LABELS_COUNT], int iterating, FILE* code_file) {
    for (int i_reg = 0; i_reg < registers_count; i_reg++) {
        if (strncmp(Registers[i_reg].register_name, lexeme, MAX_LEXEME_SIZE) == 0) {
            if (last_command == PUSH) {
                int byte_code = PUSH_FROM_REGISTER;
                fseek(code_file, -sizeof(int), SEEK_CUR);
                fwrite(&byte_code, sizeof(int), 1, code_file);
            }
            return Registers[i_reg].register_number;
        }
    }

    if (lexeme[0] == ':') {
        for (int i_lab = 0; (i_lab < MAX_LABELS_COUNT) && (labels[i_lab].label_position >= 0);  i_lab++) {
            if (strncmp(labels[i_lab].label_name, lexeme, MAX_LEXEME_SIZE) == 0)
                return labels[i_lab].label_position;
        }

        return -1;
    }
    else
        return InDigit(lexeme);
}


Set_label_res_t SetLabel (char* lexeme, size_t ip, Label_t labels[MAX_LABELS_COUNT], int iterating) {
    int i_lab = 0;
    for (; (i_lab < MAX_LABELS_COUNT) && (labels[i_lab].label_position >= 0);  i_lab++) {
        if (strncmp(labels[i_lab].label_name, lexeme, MAX_LEXEME_SIZE) == 0) {
            if (iterating == 0) {
                printf("Re-identification of label %s\n", lexeme);
                return SET_LABEL_IDENTIFICATION_ERROR;
            }
            if (iterating == 1) return SET_LABEL_NORMAL;
        }
    }

    if (i_lab >= MAX_LABELS_COUNT) {
        printf("assembler.c: not enough memory for label %s, it's max count %d\n", lexeme, MAX_LABELS_COUNT);
        return SET_LABEL_MEMORY_ERROR;
    }

    labels[i_lab].label_position = ip;
    strncpy(labels[i_lab].label_name, lexeme, MAX_LEXEME_SIZE);

    return SET_LABEL_NORMAL;
}


int GetCommand(char* lexeme, int operations_count, int* p_next_is_argument) {
    for (int i_op = 0; i_op < operations_count; i_op++) {
        if (strncmp(Operations[i_op].operation_name, lexeme, MAX_LEXEME_SIZE) == 0) {
            *p_next_is_argument = Operations[i_op].arguments_count;
            return Operations[i_op].operation_number;
        }
    }

    return UNUSEING_COMMAND_NUMBER;
}


int InDigit(char* lexeme) {
    assert(lexeme != NULL);

    int sign = 1;
    int digit = 0;

    switch (*lexeme) {
        case '-': sign = -1; break;
        case '\0': return 0; break;
        default:
            if (isdigit(*lexeme))
                digit = *lexeme - '0';
            else
                return 0;
            break;
    }

    for (char* in_lexeme_ptr = lexeme + 1; isdigit(*in_lexeme_ptr) && (*in_lexeme_ptr != '\0');
         in_lexeme_ptr++) {
        digit = (digit * 10) + (*in_lexeme_ptr - '0');
    }

    return sign * digit;
}
