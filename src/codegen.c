#include "megap_asm.h"

CodeBuffer* codegen_init() {
    CodeBuffer *buf = malloc(sizeof(CodeBuffer));
    buf->capacity = 1024;
    buf->size = 0;
    buf->code = malloc(buf->capacity);
    return buf;
}

void codegen_generate(CodeBuffer *buf, Instruction *inst) {
    /* TODO: Implementar generación de código */
    /* Aquí se traducen las instrucciones a bytecode del Megaprocessor */
    
    /* Ejemplo placeholder: agregar un byte */
    if (buf->size >= buf->capacity) {
        buf->capacity *= 2;
        buf->code = realloc(buf->code, buf->capacity);
    }
    
    buf->code[buf->size++] = 0x00; /* NOP placeholder */
}

void codegen_write_output(CodeBuffer *buf, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: No se pudo crear el archivo de salida %s\n", filename);
        return;
    }

    fwrite(buf->code, 1, buf->size, file);
    fclose(file);
}

void codegen_free(CodeBuffer *buf) {
    if (buf) {
        free(buf->code);
        free(buf);
    }
}
