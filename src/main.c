#include "megap_asm.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo.asm>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error: No se pudo abrir el archivo %s\n", argv[1]);
        return 1;
    }

    /* Leer el archivo completo */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *input = malloc(file_size + 1);
    fread(input, 1, file_size, file);
    input[file_size] = '\0';
    fclose(file);

    /* Inicializar generador de código */
    CodeBuffer *code = codegen_init();

    /* Procesar línea por línea */
    const char *ptr = input;
    int line = 1;

    while (*ptr != '\0') {
        Token *token = lexer_next_token(&ptr, &line);
        
        if (token->type == TOKEN_EOF) {
            free_token(token);
            break;
        }

        if (token->type == TOKEN_ERROR) {
            fprintf(stderr, "Error léxico en línea %d: %s\n", token->line, token->value);
            free_token(token);
            free(input);
            codegen_free(code);
            return 1;
        }

        /* Aquí iría el parsing completo de tokens */
        /* Por ahora solo liberamos el token */
        free_token(token);
    }

    /* Generar archivo de salida */
    char output_name[256];
    snprintf(output_name, sizeof(output_name), "%s.bin", argv[1]);
    codegen_write_output(code, output_name);

    printf("Ensamblado exitoso: %s\n", output_name);

    free(input);
    codegen_free(code);
    return 0;
}
