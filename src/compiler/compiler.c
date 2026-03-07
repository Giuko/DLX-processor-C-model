#include <cpu_model/cpu_model.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE_LEN 1024
#define MAX_CONSTANTS 256

// ─── Constants table ─────────────────────────────────────────────────────────
typedef struct {
    char     name[64];
    int      value;
} Constant;

static Constant constants[MAX_CONSTANTS];
static int      num_constants = 0;

static void constant_define(const char *name, int value) {
    for (int i = 0; i < num_constants; i++) {
        if (strcmp(constants[i].name, name) == 0) {
            constants[i].value = value;
            return;
        }
    }
    if (num_constants >= MAX_CONSTANTS) {
        fprintf(stderr, "[CONST] Too many constants (max %d)\n", MAX_CONSTANTS);
        exit(1);
    }
    strncpy(constants[num_constants].name, name, sizeof(constants[0].name) - 1);
    constants[num_constants].value = value;
    num_constants++;
}

static int constant_find(const char *name, int *out) {
    for (int i = 0; i < num_constants; i++) {
        if (strcmp(constants[i].name, name) == 0) {
            *out = constants[i].value;
            return 1;
        }
    }
    return 0;
}

// ─── Simple expression evaluator ─────────────────────────────────────────────
// Supports: integer literals, hex (0x...), constant names,
// and binary operators + - * / (left-to-right, no precedence).
//
// Examples in .asm:
//   .define BASE  0x100
//   .define SIZE  16
//   .define END   BASE+SIZE        ; resolves to 0x110
//   addi R1, R0, #BASE+4           ; imm = 0x104
//   addi R2, R0, #SIZE*2           ; imm = 32

static int eval_atom(const char *s, int *out) {
    while (*s && isspace(*s)) s++;
    if (*s == '\0') return 0;

    if (isdigit(*s) || (*s == '-' && isdigit(*(s+1)))) {
        char *end;
        *out = (int)strtol(s, &end, 0);
        return (end != s);
    }

    if (isalpha(*s) || *s == '_') {
        char name[64];
        int i = 0;
        while ((*s && (isalnum(*s) || *s == '_')) && i < 63)
            name[i++] = *s++;
        name[i] = '\0';
        return constant_find(name, out);
    }

    return 0;
}

static int eval_expr(const char *expr, int *out) {
    char buf[256];
    strncpy(buf, expr, sizeof(buf) - 1);
    buf[sizeof(buf)-1] = '\0';

    char *p = buf;
    int result = 0;
    char op = '+';

    while (*p) {
        while (*p && isspace(*p)) p++;
        if (*p == '\0') break;

        char atom_buf[64];
        int i = 0;
        if (*p == '-' && isdigit(*(p+1))) atom_buf[i++] = *p++;
        while (*p && !(*p=='+' || *p=='-' || *p=='*' || *p=='/') && i < 63)
            atom_buf[i++] = *p++;
        atom_buf[i] = '\0';

        int atom_val = 0;
        if (!eval_atom(atom_buf, &atom_val)) {
            fprintf(stderr, "[CONST] Cannot evaluate token: '%s'\n", atom_buf);
            return 0;
        }

        switch (op) {
            case '+': result += atom_val; break;
            case '-': result -= atom_val; break;
            case '*': result *= atom_val; break;
            case '/':
                if (atom_val == 0) { fprintf(stderr, "[CONST] Division by zero\n"); return 0; }
                result /= atom_val;
                break;
        }

        while (*p && isspace(*p)) p++;
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/')
            op = *p++;
        else
            break;
    }

    *out = result;
    return 1;
}

// Parse:  .define NAME expr
static void parse_define(const char *line) {
    const char *p = line;
    while (*p && !isspace(*p)) p++;   // skip ".define"
    while (*p &&  isspace(*p)) p++;   // skip spaces

    char name[64];
    int i = 0;
    while (*p && !isspace(*p) && i < 63) name[i++] = *p++;
    name[i] = '\0';

    while (*p && isspace(*p)) p++;

    int value = 0;
    if (!eval_expr(p, &value)) {
        fprintf(stderr, "[CONST] Failed to evaluate expression for '%s': '%s'\n", name, p);
        exit(1);
    }

    constant_define(name, value);
    printf("[CONST] .define %s = %d (0x%x)\n", name, value, (unsigned)value);
}

// Resolve an immediate token: "#EXPR", "EXPR", or plain integer.
static int parse_imm(const char *token) {
    if (token[0] == '#') token++;
    int value = 0;
    if (eval_expr(token, &value)) return value;
    fprintf(stderr, "[CONST] Cannot resolve immediate: '%s'\n", token);
    exit(1);
}

static int is_define(const char *line) {
    const char *p = line;
    while (*p && isspace(*p)) p++;
    return strncmp(p, ".define", 7) == 0 && (isspace(p[7]) || p[7] == '\0');
}

// ─── Instruction table ────────────────────────────────────────────────────────

typedef struct {
    const char *name;
    uint8_t opcode;
    uint8_t func;
} InstructionMap;

typedef struct {
    char name[32];
    int address;
} Label;

Label labels[MAX_LINE_LEN];
int num_labels = 0;

InstructionMap instruction_set[] = {
    {"add",  OPCODE_RTYPE, FUNC_ADD},
    {"addu", OPCODE_RTYPE, FUNC_ADDU},
    {"sub",  OPCODE_RTYPE, FUNC_SUB},
    {"subu", OPCODE_RTYPE, FUNC_SUBU},
    {"and",  OPCODE_RTYPE, FUNC_AND},
    {"or",   OPCODE_RTYPE, FUNC_OR},
    {"xor",  OPCODE_RTYPE, FUNC_XOR},
    {"seq",  OPCODE_RTYPE, FUNC_SEQ},
    {"sne",  OPCODE_RTYPE, FUNC_SNE},
    {"slt",  OPCODE_RTYPE, FUNC_SLT},
    {"sgt",  OPCODE_RTYPE, FUNC_SGT},
    {"sle",  OPCODE_RTYPE, FUNC_SLE},
    {"sge",  OPCODE_RTYPE, FUNC_SGE},
    {"sltu", OPCODE_RTYPE, FUNC_SLTU},
    {"sgtu", OPCODE_RTYPE, FUNC_SGTU},
    {"sleu", OPCODE_RTYPE, FUNC_SLEU},
    {"sgeu", OPCODE_RTYPE, FUNC_SGEU},
    {"sll",  OPCODE_RTYPE, FUNC_SLL},
    {"srl",  OPCODE_RTYPE, FUNC_SRL},
    {"sra",  OPCODE_RTYPE, FUNC_SRA},
    {"addi",  OPCODE_ADDI,  0},
    {"addui", OPCODE_ADDUI, 0},
    {"subi",  OPCODE_SUBI,  0},
    {"subui", OPCODE_SUBUI, 0},
    {"andi",  OPCODE_ANDI,  0},
    {"ori",   OPCODE_ORI,   0},
    {"xori",  OPCODE_XORI,  0},
    {"slli",  OPCODE_SLLI,  0},
    {"srli",  OPCODE_SRLI,  0},
    {"srai",  OPCODE_SRAI,  0},
    {"seqi",  OPCODE_SEQI,  0},
    {"snei",  OPCODE_SNEI,  0},
    {"slti",  OPCODE_SLTI,  0},
    {"sgti",  OPCODE_SGTI,  0},
    {"slei",  OPCODE_SLEI,  0},
    {"sgei",  OPCODE_SGEI,  0},
    {"sltui", OPCODE_SLTUI, 0},
    {"sgtui", OPCODE_SGTUI, 0},
    {"sleui", OPCODE_SLEUI, 0},
    {"sgeui", OPCODE_SGEUI, 0},
    {"j",    OPCODE_J,    0},
    {"jal",  OPCODE_JAL,  0},
    {"jr",   OPCODE_JR,   0},
    {"jalr", OPCODE_JALR, 0},
    {"beqz", OPCODE_BEQZ, 0},
    {"bnez", OPCODE_BNEZ, 0},
    {"lw",  OPCODE_LW, 0},
    {"sw",  OPCODE_SW, 0},
    {"nop", OPCODE_NOP, FUNC_NOP},
    {NULL, 0, 0}
};

void str_to_lower(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

int parse_opcode(const char *line, uint8_t *opcode, uint8_t *func) {
    char buffer[32];
    int i = 0;
    while (*line && isspace(*line)) line++;
    while (*line && !isspace(*line) && *line != ',' && i < (int)(sizeof(buffer)-1))
        buffer[i++] = *line++;
    buffer[i] = '\0';
    str_to_lower(buffer);
    for (int j = 0; instruction_set[j].name != NULL; j++) {
        if (strcmp(buffer, instruction_set[j].name) == 0) {
            *opcode = instruction_set[j].opcode;
            *func   = instruction_set[j].func;
            return 0;
        }
    }
    printf("Unknown OPCODE: %s\n", buffer);
    return -1;
}

int parse_register(const char *token) {
    if (token[0] != 'r' && token[0] != 'R') return -1;
    int reg = atoi(token + 1);
    if (reg < 0 || reg > 31) return -1;
    return reg;
}

int find_label_address(const char *name) {
    for (int i = 0; i < num_labels; i++)
        if (strcmp(labels[i].name, name) == 0)
            return labels[i].address;
    return 0;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    FILE *fd, *fd_out;
    char filename_out[64];
    char line[MAX_LINE_LEN];
    int  line_number = 0;
    uint32_t current_line_hex;
    uint8_t  opcode, func;
    char    *tokens[4];
    int      num_tokens = 0;
    char    *p;

    if (argc < 2) {
        fprintf(stderr, "Wrong usage: %s <asm_file>\n", argv[0]);
        exit(-1);
    }

    fd = fopen(argv[1], "r");
    if (fd == NULL) { fprintf(stderr, "fopen() failed to open %s\n", argv[1]); exit(1); }

    strcpy(filename_out, argv[1]);
    strcat(filename_out, ".mem");
    fd_out = fopen(filename_out, "w");
    if (fd_out == NULL) { fprintf(stderr, "fopen() failed to open %s\n", filename_out); exit(1); }

    // ── First pass: .define and labels ──────────────────────────────────────
    memset(line, 0, MAX_LINE_LEN);
    while (fgets(line, sizeof(line), fd)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;

        p = line;
        while (*p && isspace(*p)) p++;

        if (is_define(p)) { parse_define(p); continue; }

        char *colon = strchr(p, ':');
        if (colon != NULL && (colon == p || *(colon-1) != ' ')) {
            int len = colon - p;
            char label_name[32];
            strncpy(label_name, p, len);
            label_name[len] = '\0';
            labels[num_labels].address = line_number * 4;
            strcpy(labels[num_labels].name, label_name);
            num_labels++;
            printf("Label %s at 0x%x\n", labels[num_labels-1].name, labels[num_labels-1].address);
            continue;
        }

        line_number++;
    }

    // ── Second pass: emit instructions ──────────────────────────────────────
    rewind(fd);
    memset(line, 0, MAX_LINE_LEN);
    line_number = 0;

    while (fgets(line, sizeof(line), fd)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') continue;

        p = line;
        while (*p && isspace(*p)) p++;

        if (is_define(p)) continue;

        char *colon = strchr(p, ':');
        if (colon != NULL && (colon == p || *(colon-1) != ' ')) continue;

        printf("Line %d: %s\n", line_number, line);
        line_number++;

        char line_copy[MAX_LINE_LEN];
        strcpy(line_copy, line);
        num_tokens = 0;
        p = strtok(line_copy, " ,\t");
        while (p && num_tokens < 4) { tokens[num_tokens++] = p; p = strtok(NULL, " ,\t"); }

        if (parse_opcode(line, &opcode, &func)) {
            fprintf(stderr, "parse_opcode() failed\n");
            exit(2);
        }

        if (opcode == OPCODE_NOP) {
            current_line_hex = NOP_Instruction;

        } else if (opcode == OPCODE_RTYPE) {
            int rd  = parse_register(tokens[1]);
            int rs1 = parse_register(tokens[2]);
            int rs2 = parse_register(tokens[3]);
            current_line_hex = (opcode << 26) | (rs1 << 21) | (rs2 << 16) | (rd << 11) | func;

        } else if (opcode == OPCODE_J    || opcode == OPCODE_JAL  ||
                   opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ ||
                   opcode == OPCODE_JR   || opcode == OPCODE_JALR) {

            if (opcode == OPCODE_JR || opcode == OPCODE_JALR) {
                int rs1 = parse_register(tokens[1]);
                current_line_hex = (opcode << 26) | (rs1 << 21);
            } else if (opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ) {
                int rs1     = parse_register(tokens[1]);
                int address = find_label_address(tokens[2]);
#ifdef RELATIVE_JUMP
                int offset = address - (line_number * 4);
                current_line_hex = (opcode << 26) | (rs1 << 21) | (offset & 0xFFFF);
#else
                current_line_hex = (opcode << 26) | (rs1 << 21) | (address & 0xFFFF);
#endif
            } else {
                int address = find_label_address(tokens[1]);
#ifdef RELATIVE_JUMP
                int offset = address - (line_number * 4);
                current_line_hex = (opcode << 26) | (offset & 0x03FFFFFF);
#else
                current_line_hex = (opcode << 26) | (address & 0x03FFFFFF);
#endif
            }

        } else {
            // I-type: imm resolved via eval_expr, supports constants and expressions
            int rd  = parse_register(tokens[1]);
            int rs1 = parse_register(tokens[2]);
            int imm = parse_imm(tokens[3]);
            current_line_hex = (opcode << 26) | (rs1 << 21) | (rd << 16) | (imm & 0xFFFF);
        }

        fprintf(fd_out, "%08x\n", current_line_hex);
    }

    fclose(fd);
    fclose(fd_out);
    return 0;
}
