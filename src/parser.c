#include "megap_asm.h"

Instruction* parser_parse_line(Token **tokens, int *token_count) {
    /* TODO: Implementar parser completo */
    /* Esta es una estructura base */
    
    Instruction *inst = malloc(sizeof(Instruction));
    inst->label = NULL;
    inst->instruction = NULL;
    inst->operand_count = 0;
    inst->line = 0;

    for (int i = 0; i < 3; i++) {
        inst->operands[i] = NULL;
    }

    return inst;
}

void free_instruction(Instruction *inst) {
    if (inst) {
        free(inst->label);
        free(inst->instruction);
        for (int i = 0; i < inst->operand_count; i++) {
            free(inst->operands[i]);
        }
        free(inst);
    }
}
