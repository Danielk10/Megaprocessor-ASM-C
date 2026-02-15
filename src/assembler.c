#include "megap_asm.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_GROW(cap) ((cap) == 0 ? 16 : (cap) * 2)

static char *xstrdup(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static void set_error(Assembler *as, const char *msg) {
    snprintf(as->error, sizeof(as->error), "%s", msg);
}

static void set_errorf(Assembler *as, const char *fmt, const char *a, int b) {
    snprintf(as->error, sizeof(as->error), fmt, a, b);
}

static char *trim_copy(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

static void str_upper(char *s) {
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

static char *upper_copy(const char *s) {
    char *o = xstrdup(s);
    if (o) str_upper(o);
    return o;
}

static int split(const char *s, char delim, char ***out, size_t *count) {
    char **items = NULL;
    size_t n = 0;
    size_t cap = 0;
    size_t len = strlen(s);
    size_t start = 0;

    for (size_t i = 0; i <= len; ++i) {
        if (i == len || s[i] == delim) {
            size_t tok_len = i - start;
            char *tmp = (char *)malloc(tok_len + 1);
            if (!tmp) return 0;
            memcpy(tmp, s + start, tok_len);
            tmp[tok_len] = '\0';

            char *trimmed = trim_copy(tmp);
            free(tmp);
            if (!trimmed) return 0;

            if (n == cap) {
                cap = ARRAY_GROW(cap);
                char **ni = (char **)realloc(items, cap * sizeof(char *));
                if (!ni) return 0;
                items = ni;
            }
            items[n++] = trimmed;
            start = i + 1;
        }
    }

    *out = items;
    *count = n;
    return 1;
}

static int split_lines_raw(const char *s, char ***out, size_t *count) {
    char **items = NULL;
    size_t n = 0;
    size_t cap = 0;
    size_t len = strlen(s);
    size_t start = 0;

    for (size_t i = 0; i <= len; ++i) {
        if (i == len || s[i] == '\n') {
            size_t tok_len = i - start;
            char *tok = (char *)malloc(tok_len + 1);
            if (!tok) return 0;
            memcpy(tok, s + start, tok_len);
            tok[tok_len] = '\0';

            if (n == cap) {
                cap = ARRAY_GROW(cap);
                char **ni = (char **)realloc(items, cap * sizeof(char *));
                if (!ni) { free(tok); return 0; }
                items = ni;
            }
            items[n++] = tok;
            start = i + 1;
        }
    }

    *out = items;
    *count = n;
    return 1;
}

static void free_split(char **items, size_t count) {
    for (size_t i = 0; i < count; i++) free(items[i]);
    free(items);
}

static char *strip_comments_copy(const char *s, int *in_block_comment) {
    size_t n = strlen(s);
    char *out = (char *)malloc(n + 1);
    if (!out) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        if (in_block_comment && *in_block_comment) {
            if (s[i] == '*' && i + 1 < n && s[i + 1] == '/') {
                *in_block_comment = 0;
                i++;
            }
            continue;
        }
        if (s[i] == '/' && i + 1 < n && s[i + 1] == '*') {
            if (in_block_comment) *in_block_comment = 1;
            i++;
            continue;
        }
        if (s[i] == '/' && i + 1 < n && s[i + 1] == '/') break;
        if (s[i] == ';') break;
        out[j++] = s[i];
    }
    out[j] = '\0';
    return out;
}

static void normalize_empty_operands(char **ops, size_t *count) {
    if (ops && count && *count == 1 && ops[0][0] == '\0') *count = 0;
}

static int symbol_find(const Assembler *as, const char *upper) {
    for (size_t i = 0; i < as->symbol_count; i++) {
        if (strcmp(as->symbols[i].name, upper) == 0) return (int)i;
    }
    return -1;
}

static int symbol_set(Assembler *as, const char *name, int32_t value, int type, int is_defined) {
    char *upper = upper_copy(name);
    if (!upper) return 0;
    int idx = symbol_find(as, upper);
    if (idx >= 0) {
        free(upper);
        as->symbols[idx].value = value;
        as->symbols[idx].type = type;
        as->symbols[idx].is_defined = is_defined;
        return 1;
    }
    if (as->symbol_count == as->symbol_cap) {
        as->symbol_cap = ARRAY_GROW(as->symbol_cap);
        AsmSymbol *n = (AsmSymbol *)realloc(as->symbols, as->symbol_cap * sizeof(AsmSymbol));
        if (!n) {
            free(upper);
            return 0;
        }
        as->symbols = n;
    }
    AsmSymbol *s = &as->symbols[as->symbol_count++];
    s->name = upper;
    s->value = value;
    s->type = type;
    s->is_defined = is_defined;
    return 1;
}

static int include_find(const Assembler *as, const char *upper) {
    for (size_t i = 0; i < as->include_count; i++) {
        if (strcmp(as->includes[i].name_upper, upper) == 0) return (int)i;
    }
    return -1;
}

static int append_inst(Assembler *as, AsmInstruction *inst) {
    if (as->inst_count == as->inst_cap) {
        as->inst_cap = ARRAY_GROW(as->inst_cap);
        AsmInstruction *n = (AsmInstruction *)realloc(as->instructions, as->inst_cap * sizeof(AsmInstruction));
        if (!n) return 0;
        as->instructions = n;
    }
    as->instructions[as->inst_count++] = *inst;
    return 1;
}

static int push_byte(AsmInstruction *inst, uint8_t b) {
    if (inst->byte_count == inst->byte_cap) {
        inst->byte_cap = ARRAY_GROW(inst->byte_cap);
        uint8_t *n = (uint8_t *)realloc(inst->bytes, inst->byte_cap);
        if (!n) return 0;
        inst->bytes = n;
    }
    inst->bytes[inst->byte_count++] = b;
    return 1;
}

static int listing_append(Assembler *as, const char *s) {
    size_t n = strlen(s);
    if (as->listing_len + n + 1 > as->listing_cap) {
        size_t nc = as->listing_cap == 0 ? 1024 : as->listing_cap;
        while (as->listing_len + n + 1 > nc) nc *= 2;
        char *nb = (char *)realloc(as->listing, nc);
        if (!nb) return 0;
        as->listing = nb;
        as->listing_cap = nc;
    }
    memcpy(as->listing + as->listing_len, s, n + 1);
    as->listing_len += n;
    return 1;
}

static int parse_register(const char *token) {
    char *t = trim_copy(token ? token : "");
    if (!t) return -1;
    str_upper(t);
    size_t len = strlen(t);
    while (len && (t[len - 1] == ',' || t[len - 1] == ';' || t[len - 1] == ')' || t[len - 1] == ' ')) t[--len] = '\0';
    while (*t == '(' || *t == ' ') memmove(t, t + 1, strlen(t));
    int r = -1;
    if (!strcmp(t, "R0")) r = 0; else if (!strcmp(t, "R1")) r = 1; else if (!strcmp(t, "R2")) r = 2;
    else if (!strcmp(t, "R3")) r = 3; else if (!strcmp(t, "PS")) r = 4; else if (!strcmp(t, "SP")) r = 5;
    free(t);
    return r;
}

static uint8_t get_alu_opcode(const char *m, int ra, int rb) {
    if (ra < 0 || rb < 0 || ra > 3 || rb > 3) return 0xFF;
    uint8_t rc = (uint8_t)(rb * 4 + ra);
    if (!strcmp(m, "MOVE") || !strcmp(m, "SXT")) return rc;
    if (!strcmp(m, "AND") || !strcmp(m, "TEST")) return 0x10 + rc;
    if (!strcmp(m, "XOR") || !strcmp(m, "CLR")) return 0x20 + rc;
    if (!strcmp(m, "OR") || !strcmp(m, "INV")) return 0x30 + rc;
    if (!strcmp(m, "ADD")) return 0x40 + rc;
    if (!strcmp(m, "SUB") || !strcmp(m, "NEG")) return 0x60 + rc;
    if (!strcmp(m, "CMP") || !strcmp(m, "ABS")) return 0x70 + rc;
    return 0xFF;
}

static void skip_spaces(const char **p) { while (**p && isspace((unsigned char)**p)) (*p)++; }
static int32_t parse_expr(Assembler *as, const char **p);

static int32_t parse_factor(Assembler *as, const char **p) {
    skip_spaces(p);
    if (**p == '(') {
        (*p)++;
        int32_t x = parse_expr(as, p);
        skip_spaces(p);
        if (**p == ')') (*p)++;
        else set_error(as, "Missing ')' in expression");
        return x;
    }
    if (**p == '+' || **p == '-') {
        char op = *(*p)++;
        int32_t x = parse_factor(as, p);
        return op == '+' ? x : -x;
    }
    if (**p == '$') {
        (*p)++;
        return (int32_t)as->current_address;
    }
    if (isdigit((unsigned char)**p)) {
        int base = 10;
        if (**p == '0' && ((*p)[1] == 'x' || (*p)[1] == 'X')) { base = 16; (*p) += 2; }
        else if (**p == '0' && ((*p)[1] == 'b' || (*p)[1] == 'B')) { base = 2; (*p) += 2; }
        unsigned long v = 0;
        while (**p) {
            int d = -1;
            if (**p >= '0' && **p <= '9') d = **p - '0';
            else if (**p >= 'a' && **p <= 'f') d = 10 + **p - 'a';
            else if (**p >= 'A' && **p <= 'F') d = 10 + **p - 'A';
            if (d < 0 || d >= base) break;
            v = v * (unsigned long)base + (unsigned long)d;
            (*p)++;
        }
        return (int32_t)v;
    }
    if (isalpha((unsigned char)**p) || **p == '_') {
        char sym[256]; size_t n = 0;
        while (**p && (isalnum((unsigned char)**p) || **p == '_')) {
            if (n < sizeof(sym) - 1) sym[n++] = **p;
            (*p)++;
        }
        sym[n] = '\0';
        str_upper(sym);
        int idx = symbol_find(as, sym);
        if (idx >= 0 && as->symbols[idx].is_defined) return as->symbols[idx].value;
        set_error(as, "Undefined symbol");
        return 0;
    }
    set_error(as, "Unexpected character in expression");
    return 0;
}

static int32_t parse_term(Assembler *as, const char **p) {
    int32_t x = parse_factor(as, p);
    while (1) {
        skip_spaces(p);
        if (**p == '*') { (*p)++; x *= parse_factor(as, p); }
        else if (**p == '/') { (*p)++; int32_t y = parse_factor(as, p); if (y == 0) { set_error(as, "Division by zero in expression"); return 0; } x /= y; }
        else if (**p == '<' && (*p)[1] == '<') { (*p) += 2; x <<= parse_factor(as, p); }
        else if (**p == '>' && (*p)[1] == '>') { (*p) += 2; x >>= parse_factor(as, p); }
        else break;
    }
    return x;
}

static int32_t parse_expr(Assembler *as, const char **p) {
    int32_t x = parse_term(as, p);
    while (1) {
        skip_spaces(p);
        if (**p == '+') { (*p)++; x += parse_term(as, p); }
        else if (**p == '-') { (*p)++; x -= parse_term(as, p); }
        else break;
    }
    return x;
}

static int eval_expr(Assembler *as, const char *expr, int32_t *out) {
    as->error[0] = '\0';
    char *tmp = trim_copy(expr ? expr : "");
    if (!tmp) return 0;
    const char *p = tmp;
    *out = parse_expr(as, &p);
    skip_spaces(&p);
    int ok = (as->error[0] == '\0' && *p == '\0');
    free(tmp);
    return ok;
}

static uint8_t opcode_lookup(const char *m) {
    struct { const char *m; uint8_t o; } tbl[] = {
        {"POP",0xC0},{"PUSH",0xC8},{"RET",0xC6},{"RETI",0xC7},{"JSR",0xCF},{"TRAP",0xCD},
        {"BUC",0xE0},{"BUS",0xE1},{"BHI",0xE2},{"BLS",0xE3},{"BCC",0xE4},{"BCS",0xE5},{"BNE",0xE6},{"BEQ",0xE7},
        {"BVC",0xE8},{"BVS",0xE9},{"BPL",0xEA},{"BMI",0xEB},{"BGE",0xEC},{"BLT",0xED},{"BGT",0xEE},{"BLE",0xEF},
        {"JMP",0xF3},{"ANDI",0xF4},{"ORI",0xF5},{"ADDI",0xF6},{"SQRT",0xF7},{"MULU",0xF8},{"MULS",0xF9},{"DIVU",0xFA},
        {"DIVS",0xFB},{"ADDX",0xFC},{"SUBX",0xFD},{"NEGX",0xFE},{"NOP",0xFF}
    };
    for (size_t i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) if (!strcmp(m, tbl[i].m)) return tbl[i].o;
    return 0x00;
}

static int opcode_has(const char *m) {
    uint8_t o = opcode_lookup(m);
    return o != 0 || !strcmp(m, "POP");
}

static char *normalize_include_name(const char *tok) {
    char *t = trim_copy(tok);
    if (!t) return NULL;
    size_t n = strlen(t);
    if (n && t[n - 1] == ';') t[n - 1] = '\0';
    char *u = trim_copy(t);
    free(t);
    if (!u) return NULL;
    n = strlen(u);
    if (n >= 2 && ((u[0] == '"' && u[n - 1] == '"') || (u[0] == '\'' && u[n - 1] == '\''))) {
        memmove(u, u + 1, n - 2);
        u[n - 2] = '\0';
    }
    char *o = trim_copy(u);
    free(u);
    if (!o) return NULL;
    str_upper(o);
    return o;
}

static int preprocess_includes(Assembler *as, char **lines, size_t line_count, char ***out_lines, size_t *out_count, char **stack, size_t stack_n) {
    char **expanded = NULL; size_t exp_n = 0, exp_cap = 0;
    int in_block_comment = 0;
    for (size_t i = 0; i < line_count; i++) {
        char *raw = lines[i];
        char *parse = strip_comments_copy(raw ? raw : "", &in_block_comment);
        char *t = trim_copy(parse ? parse : ""); free(parse);
        if (!t) return 0;
        if (*t == '\0') {
            if (exp_n == exp_cap) { exp_cap = ARRAY_GROW(exp_cap); expanded = (char **)realloc(expanded, exp_cap * sizeof(char *)); }
            expanded[exp_n++] = xstrdup(raw);
            free(t);
            continue;
        }
        char mn[64] = {0};
        sscanf(t, "%63s", mn);
        str_upper(mn);
        if (!strcmp(mn, "INCLUDE")) {
            if (exp_n == exp_cap) { exp_cap = ARRAY_GROW(exp_cap); expanded = (char **)realloc(expanded, exp_cap * sizeof(char *)); }
            expanded[exp_n++] = xstrdup(raw);

            const char *sp = strstr(t, " ");
            char *iname = normalize_include_name(sp ? sp + 1 : "");
            if (!iname) { free(t); return 0; }
            if (include_find(as, iname) < 0) { set_error(as, "Include file not found"); free(iname); free(t); return 0; }
            for (size_t si = 0; si < stack_n; si++) if (!strcmp(stack[si], iname)) { set_error(as, "Recursive include detected"); free(iname); free(t); return 0; }

            int idx = include_find(as, iname);
            const char *content = as->includes[idx].content;
            char **inc_lines = NULL; size_t inc_n = 0;
            split_lines_raw(content, &inc_lines, &inc_n);
            char **new_stack = (char **)malloc((stack_n + 1) * sizeof(char *));
            for (size_t si = 0; si < stack_n; si++) new_stack[si] = stack[si];
            new_stack[stack_n] = iname;
            char **nested = NULL; size_t nested_n = 0;
            int ok = preprocess_includes(as, inc_lines, inc_n, &nested, &nested_n, new_stack, stack_n + 1);
            free(new_stack);
            free_split(inc_lines, inc_n);
            if (!ok) { free(iname); free(t); return 0; }
            for (size_t ni = 0; ni < nested_n; ni++) {
                if (exp_n == exp_cap) { exp_cap = ARRAY_GROW(exp_cap); expanded = (char **)realloc(expanded, exp_cap * sizeof(char *)); }
                expanded[exp_n++] = nested[ni];
            }
            if (exp_n == exp_cap) { exp_cap = ARRAY_GROW(exp_cap); expanded = (char **)realloc(expanded, exp_cap * sizeof(char *)); }
            expanded[exp_n++] = xstrdup("");
            free(nested);
            free(iname);
            free(t);
            continue;
        }
        if (exp_n == exp_cap) { exp_cap = ARRAY_GROW(exp_cap); expanded = (char **)realloc(expanded, exp_cap * sizeof(char *)); }
        expanded[exp_n++] = xstrdup(raw);
        free(t);
    }
    if (exp_n == 0 || expanded[exp_n - 1][0] != '\0') {
        if (exp_n == exp_cap) { exp_cap = ARRAY_GROW(exp_cap); expanded = (char **)realloc(expanded, exp_cap * sizeof(char *)); }
        expanded[exp_n++] = xstrdup("");
    }
    *out_lines = expanded;
    *out_count = exp_n;
    return 1;
}

static int encode_alu(Assembler *as, const char *mn, const char *op1, const char *op2, AsmInstruction *inst, int line) {
    int r1 = parse_register(op1);
    if (!strcmp(mn, "INC") || !strcmp(mn, "DEC")) {
        if (r1 < 0 || r1 > 3) { set_errorf(as, "Invalid register in %s at line %d", mn, line); return 0; }
        return push_byte(inst, (uint8_t)((!strcmp(mn, "INC") ? 0x54 : 0x5C) + r1));
    }
    if (!strcmp(mn, "ADDQ")) {
        int32_t val;
        const char *vs = op2;
        if (vs && *vs == '#') vs++;
        if (!eval_expr(as, vs, &val)) return 0;
        if (r1 < 0 || r1 > 3 || (val != 1 && val != 2 && val != -1 && val != -2)) { set_error(as, "ADDQ supports only #1/#2/#-1/#-2 for registers R0-R3"); return 0; }
        if (val == 1) return push_byte(inst, (uint8_t)(0x54 + r1));
        if (val == 2) return push_byte(inst, (uint8_t)(0x50 + r1));
        if (val == -1) return push_byte(inst, (uint8_t)(0x5C + r1));
        return push_byte(inst, (uint8_t)(0x58 + r1));
    }
    int r2 = parse_register(op2);
    if (!strcmp(mn, "SXT") || !strcmp(mn, "ABS") || !strcmp(mn, "INV") || !strcmp(mn, "NEG") || !strcmp(mn, "CLR") || !strcmp(mn, "TEST")) {
        if (r1 < 0 || r1 > 3) { set_errorf(as, "Invalid register in %s at line %d", mn, line); return 0; }
        return push_byte(inst, get_alu_opcode(mn, r1, r1));
    }
    if (r1 >= 0 && r2 >= 0) {
        if (!strcmp(mn, "MOVE") && r1 == 0 && r2 == 5) return push_byte(inst, 0xF0);
        if (!strcmp(mn, "MOVE") && r1 == 5 && r2 == 0) return push_byte(inst, 0xF1);
        uint8_t code = get_alu_opcode(mn, r1, r2);
        if (code == 0xFF) { set_error(as, "Invalid operands or mnemonic"); return 0; }
        return push_byte(inst, code);
    }
    set_error(as, "Invalid register(s)");
    return 0;
}

static int encode_bitop(Assembler *as, const char *mn, const char *op1, const char *op2, AsmInstruction *inst) {
    int r1 = parse_register(op1);
    if (r1 < 0 || r1 > 3) { set_error(as, "Invalid destination register for bit operation"); return 0; }
    uint8_t t = 0; if (!strcmp(mn, "BCHG")) t = 1; else if (!strcmp(mn, "BCLR")) t = 2; else if (!strcmp(mn, "BSET")) t = 3;
    int r2 = parse_register(op2);
    uint8_t b = (uint8_t)(t << 6);
    if (r2 >= 0) b |= 0x20 | (r2 & 0x03);
    else {
        int32_t bit;
        const char *vs = op2; if (vs && *vs == '#') vs++;
        if (!eval_expr(as, vs, &bit)) return 0;
        b |= (uint8_t)(bit & 0x1F);
    }
    return push_byte(inst, (uint8_t)(0xDC + r1)) && push_byte(inst, b);
}

static int starts_with(const char *s, const char *p) { return strncmp(s, p, strlen(p)) == 0; }

static int encode_ldst(Assembler *as, const char *mn, const char *op1, const char *op2, AsmInstruction *inst) {
    int is_load = (mn[0] == 'L');
    int is_byte = (strlen(mn) > 3 && mn[3] == 'B');
    int reg = parse_register(is_load ? op1 : op2);
    const char *addr_str = is_load ? op2 : op1;
    if (reg < 0) { set_error(as, "Invalid register in LD/ST"); return 0; }
    char *addr = trim_copy(addr_str ? addr_str : "");
    size_t n = strlen(addr);
    int wrapped = n > 0 && addr[0] == '(' && strchr(addr, ')');
    char inside[256] = {0};
    if (wrapped) {
        char *rp = strchr(addr, ')');
        size_t len = (size_t)(rp - addr - 1); if (len > 255) len = 255;
        memcpy(inside, addr + 1, len); inside[len] = '\0';
    }
    char inside_u[256]; snprintf(inside_u, sizeof(inside_u), "%s", inside); str_upper(inside_u);
    int ok = 1;
    if (wrapped && strstr(inside_u, "SP") && strchr(inside, '+')) {
        char *plus = strchr(inside, '+'); int32_t off;
        if (!eval_expr(as, plus + 1, &off)) ok = 0;
        if (ok) {
            uint8_t base = is_load ? (is_byte ? 0xA4 : 0xA0) : (is_byte ? 0xAC : 0xA8);
            ok = push_byte(inst, (uint8_t)(base + reg)) && push_byte(inst, (uint8_t)(off & 0xFF));
        }
    } else if (wrapped && strstr(inside, "++")) {
        int ptr = strstr(inside_u, "R2") ? 2 : 3;
        uint8_t base = 0x90; if (!is_load) base += 8; if (is_byte) base += 4; if (ptr == 3) base += 2; if (reg == 1) base += 1;
        ok = push_byte(inst, base);
    } else if (wrapped) {
        int ptr = strstr(inside_u, "R2") ? 2 : 3;
        uint8_t base = 0x80; if (!is_load) base += 8; if (is_byte) base += 4; if (ptr == 3) base += 2; if (reg == 1) base += 1;
        ok = push_byte(inst, base);
    } else if (addr[0] == '#') {
        int32_t v; if (!eval_expr(as, addr + 1, &v)) ok = 0;
        if (ok) {
            ok = push_byte(inst, (uint8_t)((is_byte ? 0xD4 : 0xD0) + reg)) && push_byte(inst, (uint8_t)(v & 0xFF));
            if (ok && !is_byte) ok = push_byte(inst, (uint8_t)((v >> 8) & 0xFF));
        }
    } else {
        int32_t a; if (!eval_expr(as, addr, &a)) ok = 0;
        if (ok) {
            uint8_t base = is_load ? (is_byte ? 0xB4 : 0xB0) : (is_byte ? 0xBC : 0xB8);
            ok = push_byte(inst, (uint8_t)(base + reg)) && push_byte(inst, (uint8_t)(a & 0xFF)) && push_byte(inst, (uint8_t)((a >> 8) & 0xFF));
        }
    }
    free(addr);
    return ok;
}

static int pass1(Assembler *as, char **lines, size_t n) {
    as->current_address = 0;
    int in_block_comment = 0;
    typedef struct { char *name; char *expr; int line; } Pending;
    Pending *pend = NULL; size_t pn = 0, pc = 0;
    for (size_t i = 0; i < n; i++) {
        int line = (int)i + 1;
        char *work = strip_comments_copy(lines[i], &in_block_comment);
        char *t = trim_copy(work); free(work);
        if (!t || !*t) { free(t); continue; }

        char l1[128] = {0}, l2[128] = {0};
        sscanf(t, "%127s %127s", l1, l2);
        str_upper(l2);
        if (!strcmp(l2, "EQU")) {
            char *eq = strstr(t, "EQU"); if (!eq) eq = strstr(t, "equ");
            int32_t v;
            if (eval_expr(as, eq ? eq + 3 : "", &v)) symbol_set(as, l1, v, 1, 1);
            else {
                symbol_set(as, l1, 0, 1, 0);
                if (pn == pc) { pc = ARRAY_GROW(pc); pend = (Pending *)realloc(pend, pc * sizeof(Pending)); }
                pend[pn].name = upper_copy(l1);
                pend[pn].expr = trim_copy(eq ? eq + 3 : "");
                pend[pn].line = line;
                pn++;
            }
            free(t); continue;
        }

        char *label_name = NULL;
        char *colon = strchr(t, ':');
        if (colon) {
            *colon = '\0';
            label_name = trim_copy(t);
            if (label_name && *label_name) symbol_set(as, label_name, as->current_address, 0, 1);
            char *rem = trim_copy(colon + 1);
            free(t); t = rem;
        }
        if (!t || !*t) { free(label_name); free(t); continue; }
        char mn[64] = {0}; sscanf(t, "%63s", mn); str_upper(mn);
        size_t mlen = strlen(mn);
        if (mlen > 3 && !strcmp(mn + mlen - 3, ".WT")) mn[mlen - 3] = '\0';
        int size = 1;
        if (!strcmp(mn, "INCLUDE")) {}
        else if (!strcmp(mn, "ORG")) {
            char *rest = strchr(t, ' '); int32_t v;
            if (!eval_expr(as, rest ? rest + 1 : "", &v)) { set_errorf(as, "Invalid ORG expression at line %d", "", line); free(label_name); free(t); return 0; }
            as->current_address = (uint16_t)v;
            if (label_name && *label_name) symbol_set(as, label_name, as->current_address, 0, 1);
            free(label_name); free(t); continue;
        } else if (!strcmp(mn, "DB") || !strcmp(mn, "DW") || !strcmp(mn, "DL")) {
            char *rest = strchr(t, ' ');
            char **v = NULL; size_t vn = 0; split(rest ? rest + 1 : "", ',', &v, &vn);
            size = (int)(vn ? vn : 1) * (!strcmp(mn, "DW") ? 2 : !strcmp(mn, "DL") ? 4 : 1);
            free_split(v, vn);
        } else if (!strcmp(mn, "DM")) {
            char *rest = trim_copy(strchr(t, ' ') ? strchr(t, ' ') + 1 : "");
            size = 1;
            if (rest) {
                size_t rn = strlen(rest);
                if (rn >= 2 && ((rest[0] == '"' && rest[rn - 1] == '"') || (rest[0] == '\'' && rest[rn - 1] == '\''))) size = (int)(rn - 2 + 1);
            }
            free(rest);
        } else if (!strcmp(mn, "DS")) {
            char *rest = strchr(t, ' '); char **v = NULL; size_t vn = 0; split(rest ? rest + 1 : "", ',', &v, &vn);
            int32_t ccount = 1;
            if (vn && v[0][0] && !eval_expr(as, v[0], &ccount)) { free_split(v, vn); free(label_name); free(t); return 0; }
            if (ccount < 0) { set_error(as, "Negative DS count"); free_split(v, vn); free(label_name); free(t); return 0; }
            size = (int)ccount;
            free_split(v, vn);
        } else if (strlen(mn) == 3 && mn[0] == 'B') size = 2;
        else if (!strcmp(mn, "JMP") || !strcmp(mn, "JSR")) { char *rest = strchr(t, ' '); size = (rest && strchr(rest, '(')) ? 1 : 3; }
        else if (!strcmp(mn, "LSR") || !strcmp(mn, "LSL") || !strcmp(mn, "ASL") || !strcmp(mn, "ASR") || !strcmp(mn, "ROL") || !strcmp(mn, "ROR") || !strcmp(mn, "ROXL") || !strcmp(mn, "ROXR") || !strcmp(mn, "BTST") || !strcmp(mn, "BCHG") || !strcmp(mn, "BCLR") || !strcmp(mn, "BSET") || !strcmp(mn, "ANDI") || !strcmp(mn, "ORI") || !strcmp(mn, "ADDI")) size = 2;
        else if (starts_with(mn, "LD.") || starts_with(mn, "ST.")) {
            char *rest = strchr(t, ' '); char **ops = NULL; size_t on = 0; split(rest ? rest + 1 : "", ',', &ops, &on);
            const char *addr = (mn[0] == 'L') ? (on > 1 ? ops[1] : "") : (on > 0 ? ops[0] : "");
            char *a = trim_copy(addr);
            int wrapped = a[0] == '(' && strchr(a, ')');
            if (wrapped && strstr(a, "SP") && strchr(a, '+')) size = 2;
            else if (wrapped && strstr(a, "++")) size = 1;
            else if (wrapped) size = 1;
            else if (a[0] == '#') size = (mn[3] == 'B') ? 2 : 3;
            else size = 3;
            free(a); free_split(ops, on);
        }
        as->current_address = (uint16_t)(as->current_address + size);
        free(label_name); free(t);
    }
    int progress = 1;
    while (progress && pn) {
        progress = 0;
        for (size_t i = 0; i < pn;) {
            int32_t v;
            if (eval_expr(as, pend[i].expr, &v)) {
                int idx = symbol_find(as, pend[i].name);
                if (idx >= 0) { as->symbols[idx].value = v; as->symbols[idx].is_defined = 1; }
                free(pend[i].name); free(pend[i].expr);
                pend[i] = pend[pn - 1]; pn--; progress = 1;
            } else i++;
        }
    }
    if (pn) { set_error(as, "Invalid EQU expression (Unresolved forward reference?)"); for (size_t i=0;i<pn;i++){free(pend[i].name);free(pend[i].expr);} free(pend); return 0; }
    free(pend);
    return 1;
}

static int generate_listing_line(Assembler *as, const AsmInstruction *inst) {
    char line[1024];
    int off = snprintf(line, sizeof(line), "%4d: ", inst->line_number);
    if (inst->byte_count == 0 && !inst->is_directive) off += snprintf(line + off, sizeof(line) - off, "               ");
    else if (inst->is_directive && inst->byte_count == 0) off += snprintf(line + off, sizeof(line) - off, "%04X           ", inst->address);
    else {
        off += snprintf(line + off, sizeof(line) - off, "%04X ", inst->address);
        for (size_t i = 0; i < 4 && i < inst->byte_count; i++) off += snprintf(line + off, sizeof(line) - off, "%02X ", inst->bytes[i]);
        for (size_t i = inst->byte_count; i < 4; i++) off += snprintf(line + off, sizeof(line) - off, "   ");
    }
    off += snprintf(line + off, sizeof(line) - off, "    %s\n", inst->original_line ? inst->original_line : "");
    if (!listing_append(as, line)) return 0;
    for (size_t i = 4; i < inst->byte_count; i += 4) {
        char ext[128]; int e = snprintf(ext, sizeof(ext), "          ");
        for (size_t j = 0; j < 4 && i + j < inst->byte_count; j++) e += snprintf(ext + e, sizeof(ext) - e, "%02X ", inst->bytes[i + j]);
        snprintf(ext + e, sizeof(ext) - e, "\n");
        if (!listing_append(as, ext)) return 0;
    }
    return 1;
}

static int pass2(Assembler *as, char **lines, size_t n) {
    as->current_address = 0;
    int in_block_comment = 0;
    for (size_t i = 0; i < n; i++) {
        int line_num = (int)i + 1;
        char *line = strip_comments_copy(lines[i], &in_block_comment);
        char *t = trim_copy(line);
        AsmInstruction inst; memset(&inst, 0, sizeof(inst));
        inst.line_number = line_num; inst.original_line = xstrdup(lines[i]); inst.address = as->current_address;
        if (!t || !*t) { append_inst(as, &inst); free(t); free(line); continue; }

        char a[128] = {0}, b[128] = {0}; sscanf(t, "%127s %127s", a, b); str_upper(b);
        if (!strcmp(b, "EQU")) {
            inst.is_directive = 1;
            char *equ_name = upper_copy(a);
            int idx = equ_name ? symbol_find(as, equ_name) : -1;
            if (idx >= 0) inst.address = (uint16_t)as->symbols[idx].value;
            free(equ_name);
            append_inst(as, &inst);
            free(t);
            free(line);
            continue;
        }

        char label_name[128] = {0};
        char *colon = strchr(t, ':');
        if (colon) {
            *colon = '\0';
            char *label = trim_copy(t);
            if (label && *label) {
                snprintf(label_name, sizeof(label_name), "%s", label);
                symbol_set(as, label_name, as->current_address, 0, 1);
            }
            free(label);
            char *r = trim_copy(colon + 1);
            free(t);
            t = r;
        }
        if (!t || !*t) { append_inst(as, &inst); free(t); free(line); continue; }
        char mn[64] = {0}; sscanf(t, "%63s", mn); str_upper(mn);
        int is_wt = 0; size_t mlen = strlen(mn); if (mlen > 3 && !strcmp(mn + mlen - 3, ".WT")) { mn[mlen - 3] = '\0'; is_wt = 1; }

        char *rest = strchr(t, ' ');
        char **ops = NULL; size_t on = 0; split(rest ? rest + 1 : "", ',', &ops, &on);
        normalize_empty_operands(ops, &on);
        const char *op1 = on > 0 ? ops[0] : "";
        const char *op2 = on > 1 ? ops[1] : "";

        if (!strcmp(mn, "INCLUDE")) inst.is_directive = 1;
        else if (!strcmp(mn, "ORG")) {
            int32_t v; if (!eval_expr(as, op1, &v)) { free_split(ops,on); free(t); free(line); return 0; }
            as->current_address = (uint16_t)v; inst.address = as->current_address; inst.is_directive = 1;
            if (label_name[0]) symbol_set(as, label_name, as->current_address, 0, 1);
        } else if (!strcmp(mn, "DB") || !strcmp(mn, "DW") || !strcmp(mn, "DL")) {
            if (on == 0) { push_byte(&inst, 0); if (!strcmp(mn, "DW") || !strcmp(mn, "DL")) push_byte(&inst, 0); if (!strcmp(mn, "DL")) { push_byte(&inst,0); push_byte(&inst,0);} }
            else for (size_t k=0;k<on;k++) { int32_t v; if (!eval_expr(as, ops[k], &v)) { free_split(ops,on); free(t); free(line); return 0;} push_byte(&inst,(uint8_t)(v&0xFF)); if (!strcmp(mn,"DW")||!strcmp(mn,"DL")) push_byte(&inst,(uint8_t)((v>>8)&0xFF)); if(!strcmp(mn,"DL")){push_byte(&inst,(uint8_t)((v>>16)&0xFF));push_byte(&inst,(uint8_t)((v>>24)&0xFF));}}
        } else if (!strcmp(mn, "DM")) {
            char *tr = trim_copy(rest ? rest + 1 : ""); size_t tn = strlen(tr);
            if (tn >= 2 && ((tr[0] == '"' && tr[tn - 1] == '"') || (tr[0] == '\'' && tr[tn - 1] == '\''))) { for (size_t q = 1; q < tn - 1; q++) push_byte(&inst, (uint8_t)tr[q]); push_byte(&inst, 0); }
            else push_byte(&inst, 0);
            free(tr);
        } else if (!strcmp(mn, "DS")) {
            int32_t count = 0, fill = 0;
            if (on > 0 && ops[0][0] && !eval_expr(as, ops[0], &count)) { free_split(ops,on); free(t); free(line); return 0; }
            if (on > 1 && ops[1][0] && !eval_expr(as, ops[1], &fill)) { free_split(ops,on); free(t); free(line); return 0; }
            for (int32_t q = 0; q < count; q++) push_byte(&inst, (uint8_t)(fill & 0xFF));
        } else if (opcode_has(mn) && mn[0] == 'B') {
            int32_t target; if (!eval_expr(as, op1, &target)) { free_split(ops,on); free(t); free(line); return 0; }
            int off = target - (as->current_address + 2); if (off < -128 || off > 127) { set_error(as, "Branch out of range"); free_split(ops,on); free(t); free(line); return 0; }
            push_byte(&inst, opcode_lookup(mn)); push_byte(&inst, (uint8_t)(off & 0xFF));
        } else if (!strcmp(mn, "JMP") || !strcmp(mn, "JSR")) {
            if (strchr(op1, '(')) push_byte(&inst, !strcmp(mn, "JMP") ? 0xF2 : 0xCE);
            else {
                int32_t target; if (!eval_expr(as, op1, &target)) { free_split(ops,on); free(t); free(line); return 0; }
                push_byte(&inst, !strcmp(mn, "JMP") ? 0xF3 : 0xCF); push_byte(&inst, (uint8_t)(target & 0xFF)); push_byte(&inst, (uint8_t)((target >> 8) & 0xFF));
            }
        } else if (starts_with(mn, "LD.") || starts_with(mn, "ST.")) {
            if (!encode_ldst(as, mn, op1, op2, &inst)) { free_split(ops,on); free(t); free(line); return 0; }
        } else if (!strcmp(mn, "MOVE") || !strcmp(mn, "AND") || !strcmp(mn, "XOR") || !strcmp(mn, "OR") || !strcmp(mn, "ADD") || !strcmp(mn, "SUB") || !strcmp(mn, "CMP") || !strcmp(mn, "TEST") || !strcmp(mn, "SXT") || !strcmp(mn, "ABS") || !strcmp(mn, "INV") || !strcmp(mn, "NEG") || !strcmp(mn, "CLR") || !strcmp(mn, "INC") || !strcmp(mn, "DEC") || !strcmp(mn, "ADDQ")) {
            if (!encode_alu(as, mn, op1, op2, &inst, line_num)) { free_split(ops,on); free(t); free(line); return 0; }
        } else if (!strcmp(mn, "PUSH") || !strcmp(mn, "POP")) {
            int r = parse_register(op1); if (r < 0) { char *ru = upper_copy(op1); if (!strcmp(ru, "PS")) r = 4; free(ru); }
            if (r < 0) { set_error(as, "Invalid register"); free_split(ops,on); free(t); free(line); return 0; }
            push_byte(&inst, (uint8_t)((!strcmp(mn, "POP") ? 0xC0 : 0xC8) + r));
        } else if (!strcmp(mn, "RET")) push_byte(&inst, 0xC6);
        else if (!strcmp(mn, "RETI")) push_byte(&inst, 0xC7);
        else if (!strcmp(mn, "TRAP")) push_byte(&inst, 0xCD);
        else if (!strcmp(mn, "ASL") || !strcmp(mn, "ASR") || !strcmp(mn, "LSL") || !strcmp(mn, "LSR") || !strcmp(mn, "ROL") || !strcmp(mn, "ROR") || !strcmp(mn, "ROXL") || !strcmp(mn, "ROXR")) {
            int r = parse_register(op1); if (r < 0) { set_error(as, "Invalid register in shift"); free_split(ops,on); free(t); free(line); return 0; }
            int type = 0; if (!strcmp(mn, "ASL") || !strcmp(mn, "ASR")) type = 2; else if (!strcmp(mn, "ROL") || !strcmp(mn, "ROR")) type = 4; else if (!strcmp(mn, "ROXL") || !strcmp(mn, "ROXR")) type = 6;
            int is_right = mn[strlen(mn)-1]=='R'; int src = parse_register(op2); int32_t sv=0; int is_reg = src>=0;
            if (!is_reg) { const char *vs=op2; if (vs[0]=='#') vs++; if (!eval_expr(as,vs,&sv)) { free_split(ops,on); free(t); free(line); return 0;} if (is_right) sv=-sv; }
            uint8_t opb = is_reg ? (uint8_t)(((type|1)<<5)|(src&0x03)) : (uint8_t)((type<<5)|(sv&0x1F));
            if (is_wt) opb |= 0x08;
            push_byte(&inst,(uint8_t)(0xD8+r)); push_byte(&inst,opb);
        } else if (!strcmp(mn, "BTST") || !strcmp(mn, "BCHG") || !strcmp(mn, "BCLR") || !strcmp(mn, "BSET")) {
            if (!encode_bitop(as,mn,op1,op2,&inst)) { free_split(ops,on); free(t); free(line); return 0; }
        } else if (!strcmp(mn, "ANDI") || !strcmp(mn, "ORI") || !strcmp(mn, "ADDI")) {
            push_byte(&inst, opcode_lookup(mn)); const char *vs = op2 && *op2 ? op2 : op1; if (vs[0]=='#') vs++; int32_t v; if (!eval_expr(as,vs,&v)) { free_split(ops,on); free(t); free(line); return 0; } push_byte(&inst,(uint8_t)(v&0xFF));
        } else if (opcode_has(mn)) push_byte(&inst, opcode_lookup(mn));
        else { set_error(as, "Unknown instruction"); free_split(ops,on); free(t); free(line); return 0; }

        as->current_address = (uint16_t)(as->current_address + inst.byte_count);
        append_inst(as, &inst);
        free_split(ops, on);
        free(t);
        free(line);
    }
    return 1;
}

void assembler_init(Assembler *as) {
    memset(as, 0, sizeof(*as));
}

void assembler_free(Assembler *as) {
    for (size_t i = 0; i < as->symbol_count; i++) free(as->symbols[i].name);
    free(as->symbols);
    for (size_t i = 0; i < as->inst_count; i++) { free(as->instructions[i].bytes); free(as->instructions[i].original_line); }
    free(as->instructions);
    for (size_t i = 0; i < as->include_count; i++) { free(as->includes[i].name_upper); free(as->includes[i].content); }
    free(as->includes);
    free(as->listing);
    memset(as, 0, sizeof(*as));
}

void assembler_set_include_files(Assembler *as, const IncludeEntry *entries, size_t count) {
    for (size_t i = 0; i < as->include_count; i++) { free(as->includes[i].name_upper); free(as->includes[i].content); }
    free(as->includes); as->includes = NULL; as->include_count = as->include_cap = 0;
    for (size_t i = 0; i < count; i++) {
        if (as->include_count == as->include_cap) {
            as->include_cap = ARRAY_GROW(as->include_cap);
            as->includes = (IncludeEntry *)realloc(as->includes, as->include_cap * sizeof(IncludeEntry));
        }
        as->includes[as->include_count].name_upper = upper_copy(entries[i].name_upper);
        as->includes[as->include_count].content = xstrdup(entries[i].content);
        as->include_count++;
    }
}

int assembler_assemble(Assembler *as, const char *source, char **hex_out) {
    *hex_out = NULL;
    as->error[0] = '\0';
    for (size_t i = 0; i < as->inst_count; i++) { free(as->instructions[i].bytes); free(as->instructions[i].original_line); }
    as->inst_count = 0;
    for (size_t i = 0; i < as->symbol_count; i++) free(as->symbols[i].name);
    as->symbol_count = 0;
    as->listing_len = 0; if (as->listing) as->listing[0] = '\0';

    char **lines = NULL; size_t line_n = 0;
    if (!split_lines_raw(source, &lines, &line_n)) return 0;
    char **expanded = NULL; size_t exp_n = 0;
    if (!preprocess_includes(as, lines, line_n, &expanded, &exp_n, NULL, 0)) { free_split(lines, line_n); return 0; }
    free_split(lines, line_n);

    {
        char **tmp = (char **)realloc(expanded, (exp_n + 1) * sizeof(char *));
        if (!tmp) { free_split(expanded, exp_n); return 0; }
        expanded = tmp;
        expanded[exp_n++] = xstrdup("");
    }

    if (!pass1(as, expanded, exp_n)) { free_split(expanded, exp_n); return 0; }
    if (!pass2(as, expanded, exp_n)) { free_split(expanded, exp_n); return 0; }
    free_split(expanded, exp_n);

    typedef struct { uint16_t a; uint8_t b; } Cell;
    Cell *img = NULL; size_t in = 0, ic = 0;
    for (size_t i = 0; i < as->inst_count; i++) for (size_t j = 0; j < as->instructions[i].byte_count; j++) {
        if (in == ic) { ic = ARRAY_GROW(ic); img = (Cell *)realloc(img, ic * sizeof(Cell)); }
        img[in].a = (uint16_t)(as->instructions[i].address + j);
        img[in].b = as->instructions[i].bytes[j];
        in++;
    }
    for (size_t i = 0; i < in; i++) for (size_t j = i + 1; j < in; j++) if (img[j].a < img[i].a) { Cell t = img[i]; img[i] = img[j]; img[j] = t; }

    size_t out_cap = in * 16 + 64;
    char *out = (char *)malloc(out_cap); size_t ol = 0;
    size_t idx = 0;
    while (idx < in) {
        uint16_t start = img[idx].a;
        uint8_t record[32]; size_t rn = 0;
        record[rn++] = img[idx].b;
        size_t j = idx + 1;
        while (j < in && rn < 0x20 && img[j].a == (uint16_t)(start + rn)) record[rn++] = img[j++].b;
        int checksum = (int)rn + (start >> 8) + (start & 0xFF);
        ol += snprintf(out + ol, out_cap - ol, ":%02X%04X00", (unsigned)rn, start);
        for (size_t k = 0; k < rn; k++) { ol += snprintf(out + ol, out_cap - ol, "%02X", record[k]); checksum += record[k]; }
        ol += snprintf(out + ol, out_cap - ol, "%02X\n", ((~checksum + 1) & 0xFF));
        idx = j;
    }
    ol += snprintf(out + ol, out_cap - ol, ":00000001FF\n");
    free(img);

    for (size_t i = 0; i < as->inst_count; i++) generate_listing_line(as, &as->instructions[i]);
    *hex_out = out;
    return 1;
}

const char *assembler_get_listing(const Assembler *as) {
    return as->listing ? as->listing : "";
}

char *asm_read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, (size_t)sz, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

int asm_write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fwrite(content, 1, strlen(content), f);
    fclose(f);
    return 1;
}
