#include <cpu_model/cpu_model.h>
#include <compiler/compiler.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE_LEN   1024
#define MAX_CONSTANTS  256
#define MAX_LABELS     1024
#define MAX_DATA_BYTES 65536


//////////////////////////////
// Section
//////////////////////////////

// Returns the section we're going to read
// Possible outputs: SEC_TEXT, SEC_RODATA
Section get_section_marker(const char *p) {
    if (strcmp(p, ".text")   == 0) return SEC_TEXT;
    if (strcmp(p, ".code")   == 0) return SEC_TEXT;
    if (strcmp(p, ".rodata") == 0) return SEC_RODATA;
    return SEC_NONE;
}

///////////////////////////////////////
// Segment
///////////////////////////////////////

// RODATA segment
// Data is placed sequentially from RODATA_BASE upward.
// Labels get their final address at parse time: RODATA_BASE + byte_offset.
uint8_t rodata_seg[MAX_DATA_BYTES];
int     rodata_size = 0;

int rodata_append(const uint8_t *bytes, int len) {
    if (rodata_size + len > MAX_DATA_BYTES) {
        fprintf(stderr, "[RODATA] Segment full (max %d bytes)\n", MAX_DATA_BYTES);
        exit(1);
    }
    int off = rodata_size;
    memcpy(rodata_seg + rodata_size, bytes, len);
    rodata_size += len;
    return off;
}

// Parse a .rodata data line:
//   label db "string" [, byte ...]   — string with optional trailing bytes
//   label db 'c'                     — single character
//   label db 42                      — single numeric byte
//   label dw 0x1234                  — 32-bit word, little-endian
//
// Returns 1 if line contained a data directive, 0 otherwise.
int parse_rodata_line(const char *line) {
    char buf[MAX_LINE_LEN];
    strncpy(buf, line, MAX_LINE_LEN - 1);
    buf[MAX_LINE_LEN - 1] = '\0';

    char *p = buf;
    while (*p && isspace(*p)) p++;
    if (!*p || *p == ';' || *p == '#') return 0;

    // Locate keyword "db" or "dw" (must be surrounded by spaces)
    char *db_pos = strstr(p, " db ");
    char *dw_pos = strstr(p, " dw ");
    char *kw     = NULL;
    int   is_db  = 0;

    if      (db_pos && (!dw_pos || db_pos <= dw_pos)) { kw = db_pos; is_db = 1; }
    else if (dw_pos)                                   { kw = dw_pos; is_db = 0; }
    else return 0;

    // Extract label name (everything before the keyword, trimmed)
    char label_name[64] = "";
    int llen = kw - p;
    if (llen > 0 && llen < 64) {
        strncpy(label_name, p, llen);
        label_name[llen] = '\0';
        int k = (int)strlen(label_name) - 1;
        while (k >= 0 && isspace(label_name[k])) label_name[k--] = '\0';
    }

    // Move past keyword
    p = kw + 4;
    while (*p && isspace(*p)) p++;

    uint8_t bytes[MAX_LINE_LEN];
    int     nbytes = 0;

    if (is_db) {
        if (*p == '"') {
            // String literal
            p++;
            while (*p && *p != '"') {
                if (*p == '\\') {
                    p++;
                    switch (*p) {
                        case 'n':  bytes[nbytes++] = '\n'; break;
                        case 'r':  bytes[nbytes++] = '\r'; break;
                        case 't':  bytes[nbytes++] = '\t'; break;
                        case '0':  bytes[nbytes++] =  0;   break;
                        case '\\': bytes[nbytes++] = '\\'; break;
                        default:   bytes[nbytes++] = (uint8_t)*p; break;
                    }
                } else {
                    bytes[nbytes++] = (uint8_t)*p;
                }
                p++;
            }
            if (*p == '"') p++;  // skip closing quote

            // Optional trailing bytes:  , 10, 0
            while (*p) {
                while (*p && (isspace(*p) || *p == ',')) p++;
                if (!*p || *p == ';') break;
                char tmp[64]; int ti = 0;
                while (*p && *p != ',' && *p != ';' && ti < 63) tmp[ti++] = *p++;
                tmp[ti] = '\0';
                int v = 0;
                if (eval_expr(tmp, &v))
                    bytes[nbytes++] = (uint8_t)(v & 0xFF);
            }

        } else if (*p == '\'') {
            // Character literal  db 'A'  or  db '\n'
            p++;
            if (*p == '\\') {
                p++;
                switch (*p) {
                    case 'n': bytes[nbytes++] = '\n'; break;
                    case 'r': bytes[nbytes++] = '\r'; break;
                    case 't': bytes[nbytes++] = '\t'; break;
                    case '0': bytes[nbytes++] =  0;   break;
                    default:  bytes[nbytes++] = (uint8_t)*p; break;
                }
                p++;
            } else {
                bytes[nbytes++] = (uint8_t)*p++;
            }

        } else {
            // Numeric byte  db 42  or  db 0xFF
            int v = 0;
            eval_expr(p, &v);
            bytes[nbytes++] = (uint8_t)(v & 0xFF);
        }

    } else {
        // dw: 32-bit word stored little-endian
        int v = 0;
        eval_expr(p, &v);
        bytes[0] = (uint8_t)( v        & 0xFF);
        bytes[1] = (uint8_t)((v >>  8) & 0xFF);
        bytes[2] = (uint8_t)((v >> 16) & 0xFF);
        bytes[3] = (uint8_t)((v >> 24) & 0xFF);
        nbytes   = 4;
    }

    // Append to segment and register label with its final address
    int offset = rodata_append(bytes, nbytes);
    if (label_name[0])
        label_add(label_name, RODATA_BASE + offset, SEC_RODATA);

    return 1;
}

///////////////////////////
// Constants
///////////////////////////
Constant constants[MAX_CONSTANTS];
int      num_constants = 0;

// Store constant in the table
void constant_define(const char *name, int value) {
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
    strncpy(constants[num_constants].name, name, 63);
    constants[num_constants].name[63] = '\0';
    constants[num_constants].value    = value;
    num_constants++;
}

// Get constant value
int constant_find(const char *name, int *out) {
    for (int i = 0; i < num_constants; i++) {
        if (strcmp(constants[i].name, name) == 0) {
            *out = constants[i].value;
            return 1;
        }
    }
    return 0;
}

// Returns true if .define is found
int is_define(const char *p) {
    return strncmp(p, ".define", 7) == 0 && (isspace(p[7]) || !p[7]);
}

// Save the value of the constant defined in the constatn table
void parse_define(const char *line) {
    const char *p = line;
    while (*p && !isspace(*p)) p++;   // skip ".define"
    while (*p &&  isspace(*p)) p++;   // skip spaces

    char name[64];
    int i = 0;

	// Extract the label
    while (*p && !isspace(*p) && i < 63) 
		name[i++] = *p++;
    name[i] = '\0';

	// Extract the value
    while (*p && isspace(*p)) 
		p++;

    int value = 0;
    if (!eval_expr(p, &value)) {
        fprintf(stderr, "[CONST] Bad expression for '%s': '%s'\n", name, p);
        exit(1);
    }
    constant_define(name, value);
    printf("[CONST] .define %s = %d (0x%x)\n", name, value, (unsigned)value);
}

////////////////////////////////////////////////////////////
// Label
////////////////////////////////////////////////////////////
Label labels[MAX_LABELS];
int   num_labels = 0;

// Save label to label table
void label_add(const char *name, int address, Section sec) {
    if (num_labels >= MAX_LABELS) {
        fprintf(stderr, "[LABEL] Too many labels (max %d)\n", MAX_LABELS);
        exit(1);
    }
    strncpy(labels[num_labels].name, name, 63);
    labels[num_labels].name[63]  = '\0';
    labels[num_labels].address   = address;
    labels[num_labels].section   = sec;
    num_labels++;
    printf("Label '%s' at 0x%08x (%s)\n", name, (unsigned)address,
           sec == SEC_RODATA ? "RODATA" : "TEXT");
}

// Get label address
int find_label_address(const char *name) {
    for (int i = 0; i < num_labels; i++)
        if (strcmp(labels[i].name, name) == 0)
            return labels[i].address;
    fprintf(stderr, "[LABEL] '%s' not found\n", name);
    return 0;
}

//////////////////////////////
// Register Aliasing
//////////////////////////////
#define NREG_ALIASES_MAX 64
RegAlias reg_aliases[NREG_ALIASES_MAX] = {
    // Special
    {"zero", 0},    // hardwired zero
    {"sp",   29},   // stack pointer
    {"ra",   31},   // return address (used by JAL)
    // Return value
    {"v0",   2},    // return value
    // Arguments
    {"a0",   4},    // argument 0  — first function argument
    {"a1",   5},    // argument 1
    {"a2",   6},    // argument 2
    {"a3",   7},    // argument 3
    // Temporaries (caller-saved, free to clobber across calls)
    {"t0",   8},
    {"t1",   9},
    {"t2",   10},
    {"t3",   11},
    {"t4",   12},
    {"t5",   13},
    {"t6",   14},
    {"t7",   15},
    // Saved registers (callee must preserve)
    {"s0",   16},
    {"s1",   17},
    {"s2",   18},
    {"s3",   19},
    {"s4",   20},
    {"s5",   21},
    {"s6",   22},
    {"s7",   23},
    // Frame pointer
    {"fp",   28},
    {"",     -1}    // sentinel — must stay last
};
int num_reg_aliases = 26;   // predefined count — custom ones appended after

// Lookup a register alias (case-insensitive)
// Returns register number or -1.
int reg_alias_find(const char *name) {
    char lower[32];
    int i = 0;
    while (name[i] && i < 31) { lower[i] = (char)tolower(name[i]); i++; }
    lower[i] = '\0';
    for (int j = 0; j < num_reg_aliases; j++){
        if (strcmp(reg_aliases[j].name, lower) == 0)
            return reg_aliases[j].reg_num;
	}
    return -1;
}

// Register an alias defined by .reg directive.
void reg_alias_define(const char *name, int reg_num) {
    char lower[32];
    int i = 0;
    while (name[i] && i < 31) { lower[i] = (char)tolower(name[i]); i++; }
    lower[i] = '\0';

    // Overwrite if already exists
    for (int j = 0; j < num_reg_aliases; j++) {
        if (strcmp(reg_aliases[j].name, lower) == 0) {
            reg_aliases[j].reg_num = reg_num;
            printf("[REG] .reg %s = r%d (updated)\n", lower, reg_num);
            return;
        }
    }
    // Append new (sentinel is at index num_reg_aliases)
    if (num_reg_aliases >= 63) {
        fprintf(stderr, "[REG] Too many register aliases\n"); exit(1);
    }
    strncpy(reg_aliases[num_reg_aliases].name, lower, 31);
    reg_aliases[num_reg_aliases].reg_num = reg_num;
    num_reg_aliases++;
    // Keep sentinel valid
    reg_aliases[num_reg_aliases].name[0] = '\0';
    reg_aliases[num_reg_aliases].reg_num = -1;

    printf("[REG] .reg %s = r%d\n", lower, reg_num);
}

// .reg directive parser
// Syntax:  .reg  alias_name  rN
//          .reg  alias_name  N      (bare number also accepted)
int is_reg_directive(const char *p) {
    return strncmp(p, ".reg", 4) == 0 && (isspace(p[4]) || !p[4]);
}

// Parse .red directive
void parse_reg_directive(const char *line) {
    const char *p = line;
    while (*p && !isspace(*p)) p++;   // skip ".reg"
    while (*p &&  isspace(*p)) p++;   // skip spaces

    // Read alias name
    char alias[32]; int i = 0;
    while (*p && !isspace(*p) && i < 31) alias[i++] = *p++;
    alias[i] = '\0';

    while (*p && isspace(*p)) p++;

    // Read register: rN or N
    if (*p == 'r' || *p == 'R') p++;
    int reg_num = atoi(p);
    if (reg_num < 0 || reg_num > 31) {
        fprintf(stderr, "[REG] Invalid register number in: %s\n", line); exit(1);
    }

    reg_alias_define(alias, reg_num);
}

// parse_register (replaces the old one)
// Accepts:  rN  RN  alias  ALIAS  (case-insensitive for aliases)
int parse_register(const char *token) {
    if (!token || !*token) return -1;

    // rN or RN form
    if (token[0] == 'r' || token[0] == 'R') {
        // Make sure what follows is all digits
        int all_digits = 1;
        for (int i = 1; token[i]; i++)
            if (!isdigit(token[i])) { all_digits = 0; break; }
        if (all_digits && token[1]) {
            int r = atoi(token + 1);
            return (r >= 0 && r <= 31) ? r : -1;
        }
    }

    // Alias lookup (case-insensitive)
    int r = reg_alias_find(token);
    if (r >= 0) return r;

    fprintf(stderr, "[REG] Unknown register: '%s'\n", token);
    return -1;
}

/////////////////////////////////////////////////////////////
// Expression evaluator  (left-to-right, operators: + - * /)
/////////////////////////////////////////////////////////////

// Find label/constant value
int eval_atom(const char *s, int *out) {
    while (*s && isspace(*s)) s++;
    if (!*s) return 0;

    // Numeric literal (decimal or hex)
    if (isdigit(*s) || (*s == '-' && isdigit(s[1]))) {
        char *end;
        *out = (int)strtol(s, &end, 0);
        return (end != s);
    }

    // Name: try constants first, then labels
    if (isalpha(*s) || *s == '_') {
        char name[64];
        int i = 0;
        while ((*s && (isalnum(*s) || *s == '_')) && i < 63)
            name[i++] = *s++;
        name[i] = '\0';
        if (constant_find(name, out)) return 1;
        for (int j = 0; j < num_labels; j++)
            if (strcmp(labels[j].name, name) == 0) {
                *out = labels[j].address;
                return 1;
            }
        return 0;
    }
    return 0;
}

// Resolve expression
int eval_expr(const char *expr, int *out) {
    char buf[256];
    strncpy(buf, expr, 255);
    buf[255] = '\0';

    char *p   = buf;
    int result = 0;
    char op    = '+';

    while (*p) {
        while (*p && isspace(*p)) 
			p++;
        if (!*p)
			break;

        // Collect atom characters
        char atom[64];
        int i = 0;
        if (*p == '-' && isdigit(p[1])) atom[i++] = *p++;
        while (*p && *p != '+' && *p != '-' && *p != '*' && *p != '/' && i < 63)
            atom[i++] = *p++;
        atom[i] = '\0';

        int v = 0;
        if (!eval_atom(atom, &v)) {
            fprintf(stderr, "[EXPR] Cannot evaluate: '%s'\n", atom);
            return 0;
        }

        switch (op) {
            case '+': result += v; break;
            case '-': result -= v; break;
            case '*': result *= v; break;
            case '/':
                if (!v) { fprintf(stderr, "[EXPR] Division by zero\n"); return 0; }
                result /= v;
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

// Obtain immediate value
// #<decimal>
// 0x<hexadeximal>
int parse_imm(const char *token) {
    if (*token == '#') token++;
    int v = 0;
    if (eval_expr(token, &v)) return v;
    fprintf(stderr, "[IMM] Cannot resolve: '%s'\n", token);
    exit(1);
}


///////////////////////
// Instruction table
///////////////////////

InstructionMap instruction_set[] = {
    // R-type
    {"add",  OPCODE_RTYPE, FUNC_ADD},  {"addu", OPCODE_RTYPE, FUNC_ADDU},
    {"sub",  OPCODE_RTYPE, FUNC_SUB},  {"subu", OPCODE_RTYPE, FUNC_SUBU},
    {"and",  OPCODE_RTYPE, FUNC_AND},  {"or",   OPCODE_RTYPE, FUNC_OR},
    {"xor",  OPCODE_RTYPE, FUNC_XOR},  {"seq",  OPCODE_RTYPE, FUNC_SEQ},
    {"sne",  OPCODE_RTYPE, FUNC_SNE},  {"slt",  OPCODE_RTYPE, FUNC_SLT},
    {"sgt",  OPCODE_RTYPE, FUNC_SGT},  {"sle",  OPCODE_RTYPE, FUNC_SLE},
    {"sge",  OPCODE_RTYPE, FUNC_SGE},  {"sltu", OPCODE_RTYPE, FUNC_SLTU},
    {"sgtu", OPCODE_RTYPE, FUNC_SGTU}, {"sleu", OPCODE_RTYPE, FUNC_SLEU},
    {"sgeu", OPCODE_RTYPE, FUNC_SGEU}, {"sll",  OPCODE_RTYPE, FUNC_SLL},
    {"srl",  OPCODE_RTYPE, FUNC_SRL},  {"sra",  OPCODE_RTYPE, FUNC_SRA},
    // I-type
    {"addi",  OPCODE_ADDI,  0}, {"addui", OPCODE_ADDUI, 0},
    {"subi",  OPCODE_SUBI,  0}, {"subui", OPCODE_SUBUI, 0},
    {"andi",  OPCODE_ANDI,  0}, {"ori",   OPCODE_ORI,   0},
    {"xori",  OPCODE_XORI,  0}, {"slli",  OPCODE_SLLI,  0},
    {"srli",  OPCODE_SRLI,  0}, {"srai",  OPCODE_SRAI,  0},
    {"seqi",  OPCODE_SEQI,  0}, {"snei",  OPCODE_SNEI,  0},
    {"slti",  OPCODE_SLTI,  0}, {"sgti",  OPCODE_SGTI,  0},
    {"slei",  OPCODE_SLEI,  0}, {"sgei",  OPCODE_SGEI,  0},
    {"sltui", OPCODE_SLTUI, 0}, {"sgtui", OPCODE_SGTUI, 0},
    {"sleui", OPCODE_SLEUI, 0}, {"sgeui", OPCODE_SGEUI, 0},
    // Jumps / branches
    {"j",    OPCODE_J,    0}, {"jal",  OPCODE_JAL,  0},
    {"jr",   OPCODE_JR,   0}, {"jalr", OPCODE_JALR, 0},
    {"beqz", OPCODE_BEQZ, 0}, {"bnez", OPCODE_BNEZ, 0},
    // Memory
    {"lb",  OPCODE_LB,  0}, {"lh",  OPCODE_LH,  0},
    {"lw",  OPCODE_LW,  0}, {"lbu", OPCODE_LBU, 0},
    {"lhu", OPCODE_LHU, 0}, {"sb",  OPCODE_SB,  0},
    {"sh",  OPCODE_SH,  0}, {"sw",  OPCODE_SW,  0},
    // NOP
    {"nop", OPCODE_NOP, FUNC_NOP},
	// REFACTORING OPCODE
    {"push", OPCODE_PUSH, 0},
    {"pop", OPCODE_POP, 0},
    {"inc", OPCODE_INC, 0},
    {NULL, 0, 0}
};

void str_to_lower(char *s) { for (; *s; s++) *s = tolower(*s); }

int parse_opcode(const char *line, uint8_t *op, uint8_t *fn) {
    char buf[32];
    int  i = 0;
    while (*line && isspace(*line)) line++;
    while (*line && !isspace(*line) && *line != ',' && i < 31) buf[i++] = *line++;
    buf[i] = '\0';
    str_to_lower(buf);
    for (int j = 0; instruction_set[j].name; j++) {
        if (strcmp(buf, instruction_set[j].name) == 0) {
            *op = instruction_set[j].opcode;
            *fn = instruction_set[j].func;
            return 0;
        }
    }
    fprintf(stderr, "Unknown OPCODE: %s\n", buf);
    return -1;
}




////////////////////////////////////////////////////////////////
// .include support
//
// Syntax:  .include "filename.asm"
//          .include <filename.asm>
////////////////////////////////////////////////////////////////

// Returns 1 if the line is a .include directive, and extracts the filename.
// out_filename must be at least MAX_LINE_LEN bytes.
static int is_include(const char *p, char *out_filename) {
    if (strncmp(p, ".include", 8) != 0) return 0;
    if (!isspace((unsigned char)p[8]) && p[8] != '\0') return 0;

    p += 8;
    while (*p && isspace((unsigned char)*p)) p++;

    char delim_open, delim_close;
    if (*p == '"') { 
		delim_open = '"';  
		delim_close = '"';  
	} else if (*p == '<') { 
		delim_open = '<';  
		delim_close = '>'; 
	} else 
		return 0;
    (void)delim_open;
    p++;  // skip opening delimiter

    int i = 0;
    while (*p && *p != delim_close && i < MAX_LINE_LEN - 1)
        out_filename[i++] = *p++;
    out_filename[i] = '\0';

    return (i > 0);
}

// Inline the content of filename into fd_out, recursively resolving
// further .include directives inside it.
// depth guards against circular includes.
static void include_file(const char *filename, FILE *fd_out, int depth) {
    if (depth > 16) {
        fprintf(stderr, "[INCLUDE] Max include depth reached for '%s'\n", filename);
        exit(1);
    }
	char buffer[64];
	sprintf(buffer, "programs/%s", filename);
    FILE *f = fopen(buffer, "r");
    if (!f) {
        fprintf(stderr, "[INCLUDE] Cannot open '%s'\n", buffer);
        exit(1);
    }

    printf("[INCLUDE] %s\n", buffer);

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p && isspace((unsigned char)*p)) p++;

        char inc_filename[MAX_LINE_LEN];
        if (is_include(p, inc_filename)) {
            include_file(inc_filename, fd_out, depth + 1);
        } else {
            fputs(line, fd_out);
        }
    }

    fclose(f);
}


///////////////////////
// Code refactoring
//////////////////////

// Generate a refactored filename.dlx code 
int code_refactoring(char *s, FILE *fd) {
    char  line[MAX_LINE_LEN];
    char  out_name[256];
    FILE *fd_out;

    if (fd == NULL) {
        fprintf(stderr, "[REFACTORING] File is NULL\n");
        return 1;
    }

    snprintf(out_name, sizeof(out_name), "%s.dlx", s);
    fd_out = fopen(out_name, "w");
    if (!fd_out) {
        fprintf(stderr, "[ERROR] Cannot open output '%s'\n", out_name);
        return 1;
    }

    while (fgets(line, sizeof(line), fd)) {
        char tmp[MAX_LINE_LEN];
        strcpy(tmp, line);

        // Trim leading spaces for matching
        char *p = tmp;
        while (*p && isspace((unsigned char)*p)) p++;

        // .include
        char inc_filename[MAX_LINE_LEN];
        if (is_include(p, inc_filename)) {
            include_file(inc_filename, fd_out, 0);
            continue;
        }

        // Tokenize for pseudo-instruction detection
        char tok_buf[MAX_LINE_LEN];
        strcpy(tok_buf, line);
        char *tokens[4];
        int   num_tokens = 0;
        char *tok = strtok(tok_buf, " ,\t\n");
        while (tok && num_tokens < 4) { tokens[num_tokens++] = tok; tok = strtok(NULL, " ,\t\n"); }

        if (num_tokens == 0) { fputs(line, fd_out); continue; }

        // inc rX  →  addi rX, rX, #1
        if (!strcmp(tokens[0], "inc") && num_tokens == 2) {
            fprintf(fd_out, "addi %s, %s, #1\n", tokens[1], tokens[1]);
        }
        // dec rX  →  subi rX, rX, #1 
        else if (!strcmp(tokens[0], "dec") && num_tokens == 2) {
            fprintf(fd_out, "subi %s, %s, #1\n", tokens[1], tokens[1]);
        }
        // push rX  →  sw rX, sp, #0 / subi sp, sp, #1
        else if (!strcmp(tokens[0], "push") && num_tokens == 2) {
            fprintf(fd_out, "sw %s, sp, #0\n",      tokens[1]);
            fprintf(fd_out, "subi sp, sp, #1\n");
        }
        // pop rX  →  addi sp, sp, #1 / lw rX, sp, #0
        else if (!strcmp(tokens[0], "pop") && num_tokens == 2) {
            fprintf(fd_out, "addi sp, sp, #1\n");
            fprintf(fd_out, "lw %s, sp, #0\n", tokens[1]);
        }
        // pass through
        else {
            fputs(line, fd_out);
        }
    }

    fclose(fd_out);
    printf("[REFACTORING] %s → %s\n", s, out_name);
    return 0;
}

//////////////
// Main
//////////////
int main(int argc, char *argv[]) {
	char buffer[64];
    if (argc < 2) { 
		fprintf(stderr, "Usage: %s <asm_file>\n", argv[0]); 
		exit(-1); 
	}

    FILE *fd = fopen(argv[1], "r");
    if (!fd) { 
		fprintf(stderr, "[ERROR] Cannot open '%s'\n", argv[1]); 
		exit(1); 
	}

	if(code_refactoring(argv[1], fd)) {
		exit(1);
	}
	fclose(fd);

    snprintf(buffer, sizeof(buffer), "%s.dlx", argv[1]);
    fd = fopen(buffer, "r");
    if (!fd) { 
		fprintf(stderr, "[ERROR] Cannot open '%s'\n", buffer); 
		exit(1); 
	}

    char out_name[256];
    snprintf(out_name, sizeof(out_name), "%s.mem", argv[1]);
    FILE *fd_out = fopen(out_name, "w");
    if (!fd_out) { 
		fprintf(stderr, "[ERROR] Cannot open output '%s'\n", out_name);
		exit(1); 
	}


    char line[MAX_LINE_LEN];

    //////////////////////////////////////////////////////////////
    // Pass 1 — collect .define, .rodata data, TEXT/RODATA labels
    //////////////////////////////////////////////////////////////
    Section cur_section = SEC_NONE;
    int     text_line   = 0;

    while (fgets(line, sizeof(line), fd)) {
        line[strcspn(line, "\r\n")] = 0;
        char *p = line;
        while (*p && isspace(*p)) p++;
        if (!*p || *p == ';' || *p == '#') 
			continue;

        // Get section
        Section sm = get_section_marker(p);
        if (sm != SEC_NONE) { 
			cur_section = sm; 
			continue; 
		}

        // .define (allowed anywhere)
		// Get constants
        if (is_define(p)) {
			parse_define(p); 
			continue; 
		}

		if (is_reg_directive(p)) { 
			parse_reg_directive(p); 
			continue; 
		}

        if (cur_section == SEC_RODATA) {
            parse_rodata_line(p);
            continue;
        }

        // TEXT section: collect labels, count instructions
        char *colon = strchr(p, ':');
        if (colon && (colon == p || *(colon - 1) != ' ')) {
            int  len = colon - p;
            char lname[64];
            strncpy(lname, p, len);
            lname[len] = '\0';
            label_add(lname, text_line * 4, SEC_TEXT);
            continue;
        }
        text_line++;
    }

    //////////////////////////////////////////////////////////////
    // Pass 2 — emit @TEXT instructions
    //////////////////////////////////////////////////////////////
    rewind(fd);
    fprintf(fd_out, "@TEXT\n");

    cur_section      = SEC_NONE;
    int  line_number = 0;
    char *tokens[4];
    int   num_tokens;

    while (fgets(line, sizeof(line), fd)) {
        line[strcspn(line, "\r\n")] = 0;
        char *p = line;
        while (*p && isspace(*p)) p++;
        if (!*p || *p == ';' || *p == '#') continue;

        Section sm = get_section_marker(p);
        if (sm != SEC_NONE) { cur_section = sm; continue; }

        if (is_define(p))              continue;
		if (is_reg_directive(p)) continue;
        if (cur_section == SEC_RODATA) continue;

        // Skip label lines
        char *colon = strchr(p, ':');
        if (colon && (colon == p || *(colon - 1) != ' ')) continue;

        line_number++;

        // Tokenize a copy of the line
        char lc[MAX_LINE_LEN];
        strcpy(lc, line);
        num_tokens = 0;
        char *tok = strtok(lc, " ,\t");
        while (tok && num_tokens < 4) { tokens[num_tokens++] = tok; tok = strtok(NULL, " ,\t"); }

        uint8_t opcode, func;
        if (parse_opcode(line, &opcode, &func)) {
            fprintf(stderr, "[ERROR] parse_opcode failed: %s\n", line);
            exit(2);
        }

        uint32_t hex = 0;

        if (opcode == OPCODE_NOP) {
            hex = NOP_Instruction;

        } else if (opcode == OPCODE_RTYPE) {
            int rd  = parse_register(tokens[1]);
            int rs1 = parse_register(tokens[2]);
            int rs2 = parse_register(tokens[3]);
            hex = (opcode << 26) | (rs1 << 21) | (rs2 << 16) | (rd << 11) | func;

        } else if (opcode == OPCODE_J    || opcode == OPCODE_JAL  ||
                   opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ ||
                   opcode == OPCODE_JR   || opcode == OPCODE_JALR) {

            if (opcode == OPCODE_JR || opcode == OPCODE_JALR) {
                int rs1 = parse_register(tokens[1]);
                hex = (opcode << 26) | (rs1 << 21);

            } else if (opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ) {
                int rs1  = parse_register(tokens[1]);
                int addr = find_label_address(tokens[2]);
#ifdef RELATIVE_JUMP
                hex = (opcode << 26) | (rs1 << 21) | ((addr - (line_number * 4)) & 0xFFFF);
#else
                hex = (opcode << 26) | (rs1 << 21) | (addr & 0xFFFF);
#endif
            } else {
                int addr = find_label_address(tokens[1]);
#ifdef RELATIVE_JUMP
                hex = (opcode << 26) | ((addr - (line_number * 4)) & 0x03FFFFFF);
#else
                hex = (opcode << 26) | (addr & 0x03FFFFFF);
#endif
            }

        } else {
            // I-type: opcode RS1 RD imm
            int rd  = parse_register(tokens[1]);
            int rs1 = parse_register(tokens[2]);
            int imm = parse_imm(tokens[3]);
            hex = (opcode << 26) | (rs1 << 21) | (rd << 16) | (imm & 0xFFFF);
        }

        fprintf(fd_out, "%08x\n", hex);
    }

    //////////////////////////////////////////////////////////////
    // Emit @RODATA section
    // Words are padded to a 4-byte boundary and stored little-endian.
    //////////////////////////////////////////////////////////////
    if (rodata_size > 0) {
        fprintf(fd_out, "@RODATA\n");
        int padded = (rodata_size + 3) & ~3;
        for (int i = 0; i < padded; i += 4) {
            uint32_t word = 0;
            for (int b = 0; b < 4; b++)
                if (i + b < rodata_size)
                    word |= ((uint32_t)rodata_seg[i + b]) << (b * 8);
            fprintf(fd_out, "%08x\n", word);
        }
    }

    fclose(fd);
    fclose(fd_out);

    printf("[OK] %s\n", out_name);
    printf("     TEXT:   %d instructions\n", line_number);
    printf("     RODATA: %d bytes, %d words, base 0x%08x\n",
           rodata_size, (rodata_size + 3) / 4, (unsigned)RODATA_BASE);
    return 0;
}
