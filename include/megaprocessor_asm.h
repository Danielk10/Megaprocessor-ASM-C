#ifndef MEGAPROCESSOR_ASM_H
#define MEGAPROCESSOR_ASM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Lexer */
typedef enum {
    TOKEN_LABEL,
    TOKEN_INSTRUCTION,
    TOKEN_REGISTER,
    TOKEN_NUMBER,
    TOKEN_COMMA,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *value;
    int line;
} Token;

Token* lexer_next_token(const char **input, int *line);
void free_token(Token *token);

/* Parser */
typedef struct {
    char *label;
    char *instruction;
    char *operands[3];
    int operand_count;
    int line;
} Instruction;

Instruction* parser_parse_line(Token **tokens, int *token_count);
void free_instruction(Instruction *inst);

/* Code Generator */
typedef struct {
    uint8_t *code;
    size_t size;
    size_t capacity;
} CodeBuffer;

CodeBuffer* codegen_init();
void codegen_generate(CodeBuffer *buf, Instruction *inst);
void codegen_write_output(CodeBuffer *buf, const char *filename);
void codegen_free(CodeBuffer *buf);

#endif /* MEGAPROCESSOR_ASM_H */
