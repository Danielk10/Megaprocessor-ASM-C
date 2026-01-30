#include "megap_asm.h"
#include <ctype.h>

Token* lexer_next_token(const char **input, int *line) {
    Token *token = malloc(sizeof(Token));
    token->line = *line;
    token->value = NULL;

    const char *ptr = *input;

    /* Saltar espacios en blanco */
    while (*ptr == ' ' || *ptr == '\t') {
        ptr++;
    }

    /* Nueva línea */
    if (*ptr == '\n') {
        token->type = TOKEN_NEWLINE;
        token->value = strdup("\n");
        (*line)++;
        *input = ptr + 1;
        return token;
    }

    /* Fin de archivo */
    if (*ptr == '\0') {
        token->type = TOKEN_EOF;
        token->value = strdup("");
        *input = ptr;
        return token;
    }

    /* Comentario */
    if (*ptr == ';') {
        while (*ptr != '\n' && *ptr != '\0') {
            ptr++;
        }
        *input = ptr;
        return lexer_next_token(input, line);
    }

    /* Coma */
    if (*ptr == ',') {
        token->type = TOKEN_COMMA;
        token->value = strdup(",");
        *input = ptr + 1;
        return token;
    }

    /* Número hexadecimal o decimal */
    if (isdigit(*ptr) || (*ptr == '0' && (*(ptr+1) == 'x' || *(ptr+1) == 'X'))) {
        const char *start = ptr;
        
        if (*ptr == '0' && (*(ptr+1) == 'x' || *(ptr+1) == 'X')) {
            ptr += 2;
            while (isxdigit(*ptr)) ptr++;
        } else {
            while (isdigit(*ptr)) ptr++;
        }

        size_t len = ptr - start;
        token->type = TOKEN_NUMBER;
        token->value = malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';
        *input = ptr;
        return token;
    }

    /* Identificador (etiqueta, instrucción o registro) */
    if (isalpha(*ptr) || *ptr == '_') {
        const char *start = ptr;
        while (isalnum(*ptr) || *ptr == '_') {
            ptr++;
        }

        size_t len = ptr - start;
        token->value = malloc(len + 1);
        strncpy(token->value, start, len);
        token->value[len] = '\0';

        /* Por defecto, asumir que es una instrucción */
        /* Aquí se podría clasificar mejor según un diccionario */
        token->type = TOKEN_INSTRUCTION;
        *input = ptr;
        return token;
    }

    /* Token no reconocido */
    token->type = TOKEN_ERROR;
    token->value = malloc(2);
    token->value[0] = *ptr;
    token->value[1] = '\0';
    *input = ptr + 1;
    return token;
}

void free_token(Token *token) {
    if (token) {
        free(token->value);
        free(token);
    }
}
