#ifndef MEGAP_ASM_H
#define MEGAP_ASM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;
    int32_t value;
    int type;
    int is_defined;
} AsmSymbol;

typedef struct {
    uint16_t address;
    uint8_t *bytes;
    size_t byte_count;
    size_t byte_cap;
    char *original_line;
    int line_number;
    int is_directive;
} AsmInstruction;

typedef struct {
    char *name_upper;
    char *content;
} IncludeEntry;

typedef struct {
    AsmSymbol *symbols;
    size_t symbol_count;
    size_t symbol_cap;

    AsmInstruction *instructions;
    size_t inst_count;
    size_t inst_cap;

    IncludeEntry *includes;
    size_t include_count;
    size_t include_cap;

    char *listing;
    size_t listing_len;
    size_t listing_cap;

    uint16_t current_address;
    char error[512];
} Assembler;

void assembler_init(Assembler *as);
void assembler_free(Assembler *as);
void assembler_set_include_files(Assembler *as, const IncludeEntry *entries, size_t count);
int assembler_assemble(Assembler *as, const char *source, char **hex_out);
const char *assembler_get_listing(const Assembler *as);

char *asm_read_file(const char *path);
int asm_write_file(const char *path, const char *content);

#ifdef __cplusplus
}
#endif

#endif
