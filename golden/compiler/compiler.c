#include "../cpu_model/cpu_model.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 1024


typedef struct {
	const char *name;
	uint8_t opcode;
	uint8_t func; // Only used for R-type instructions
} InstructionMap;

typedef struct {
    char name[32];
    int address; // instruction number / offset
} Label;

Label labels[MAX_LINE_LEN];
int num_labels = 0;
// Table mapping mnemonics to opcodes/func
InstructionMap instruction_set[] = {
	// R-type instructions
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

	// I-type instructions
	{"addi",  OPCODE_ADDI, 0},
	{"addui", OPCODE_ADDUI, 0},
	{"subi",  OPCODE_SUBI, 0},
	{"subui", OPCODE_SUBUI, 0},
	{"andi",  OPCODE_ANDI, 0},
	{"ori",   OPCODE_ORI, 0},
	{"xori",  OPCODE_XORI, 0},
	{"slli",  OPCODE_SLLI, 0},
	{"srli",  OPCODE_SRLI, 0},
	{"srai",  OPCODE_SRAI, 0},
	{"seqi",  OPCODE_SEQI, 0},
	{"snei",  OPCODE_SNEI, 0},
	{"slti",  OPCODE_SLTI, 0},
	{"sgti",  OPCODE_SGTI, 0},
	{"slei",  OPCODE_SLEI, 0},
	{"sgei",  OPCODE_SGEI, 0},
	{"sltui", OPCODE_SLTUI, 0},
	{"sgtui", OPCODE_SGTUI, 0},
	{"sleui", OPCODE_SLEUI, 0},
	{"sgeui", OPCODE_SGEUI, 0},

	// Jumps and branches
	{"j",    OPCODE_J, 0},
	{"jal",  OPCODE_JAL, 0},
	{"jr",   OPCODE_JR, 0},
	{"jalr", OPCODE_JALR, 0},
	{"beqz", OPCODE_BEQZ, 0},
	{"bnez", OPCODE_BNEZ, 0},

	// Memory
	{"lw",   OPCODE_LW, 0},
	{"sw",   OPCODE_SW, 0},

	// NOP
	{"nop", OPCODE_NOP, FUNC_NOP},

	{NULL, 0, 0} // Sentinel
};

// Convert a string to lowercase
void str_to_lower(char *str) {
	for (; *str; ++str) {
		*str = tolower(*str);
	}
}

// Parse the first token of a line (the opcode)
int parse_opcode(const char *line, uint8_t *opcode, uint8_t *func) {
	char buffer[32];
	int i = 0;

	// Skip leading spaces
	while (*line && isspace(*line)) line++;

	// Copy characters until space, comma, or end of line
	while (*line && !isspace(*line) && *line != ',' && i < (int)(sizeof(buffer)-1)) {
		buffer[i++] = *line++;
	}
	buffer[i] = '\0';

    // Convert to lowercase for comparison
	str_to_lower(buffer);

    // Match known opcodes
	for (int j = 0; instruction_set[j].name != NULL; j++) {
		if (strcmp(buffer, instruction_set[j].name) == 0) {
			*opcode = instruction_set[j].opcode;
			*func   = instruction_set[j].func;
			return 0;
		}
    }
	
    // Unknown opcode
	printf("Unknown OPCODE: %s\n", line);
	return -1;
}

int parse_register(const char *token) {
	if (token[0] != 'r' && token[0] != 'R') 
		return -1;
	int reg = atoi(token + 1);
	if (reg < 0 || reg > 31) 
		return -1;
	return reg;
}

int parse_imm(const char *token) {
	return (int)strtol(token+1, NULL, 0); // handles decimal and hex (0x)
}

int find_label_address(const char *name) {
	for (int i = 0; i < num_labels; i++) {
		if (strcmp(labels[i].name, name) == 0)
			return labels[i].address;	
	}
	return 0; // not found
}

int main(int argc, char *argv[]){
	FILE *fd, *fd_out;
	char filename_out[64];
	char line[MAX_LINE_LEN];
	int line_number = 0;
	uint32_t current_line_hex;
	uint8_t opcode, func;
	char *tokens[4];
	int num_tokens = 0;
	char *p;

	if(argc < 2){
		fprintf(stderr, "Wrong usage: %s <asm_file>\n", argv[0]);
		exit(-1);
	}

	fd = fopen(argv[1], "r");
	if(fd == NULL){
		fprintf(stderr, "fopen() failed to open %s\n", argv[1]);
		exit(1);
	}

	strcpy(filename_out, argv[1]);
	strcat(filename_out, ".mem");
	fd_out = fopen(filename_out, "w");
	if(fd_out == NULL){
		fprintf(stderr, "fopen() failed to open %s\n", filename_out);
		exit(1);
	}

	memset(line, 0, MAX_LINE_LEN);

	while (fgets(line, sizeof(line), fd)) {
		line_number++;
	
		// Remove newline character
		line[strcspn(line, "\r\n")] = 0;

		// Skip empty lines and comments
		if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
			continue;
		p = line;
		while (*p && isspace(*p)) p++;
		char *colon = strchr(p, ':');
		// Check if the line contains a colon : before any space or comment
		if (colon != NULL && (colon == p || *(colon-1) != ' ')) {
			// Found a label
			int len = colon - p;
			char label_name[32];
			strncpy(label_name, p, len);
			label_name[len] = '\0';
			labels[num_labels].address = (line_number-1)*4;
			strcpy(labels[num_labels].name, label_name);
			num_labels++;

			printf("Label %s at 0x%x\n", labels[num_labels-1].name, labels[num_labels-1].address);
			continue;
		}
	}
	rewind(fd);
	memset(line, 0, MAX_LINE_LEN);
	line_number=0;
	while (fgets(line, sizeof(line), fd)) {
		line_number++;
	
		// Remove newline character
		line[strcspn(line, "\r\n")] = 0;

		// Skip empty lines and comments
		if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
			continue;


		printf("Line %d: %s\n", line_number, line);
		p = line;
		while (*p && isspace(*p)) p++;
		
		char *colon = strchr(p, ':');
		// Check if the line contains a colon : before any space or comment
		if (colon != NULL && (colon == p || *(colon-1) != ' ')) {
			// Found a label
			/*
			int len = colon - p;
			char label_name[32];
			strncpy(label_name, p, len);
			label_name[len] = '\0';
			labels[num_labels].address = (line_number-1)*4;
			strcpy(labels[num_labels].name, label_name);
			num_labels++;

			printf("Label %s at 0x%x\n", labels[num_labels-1].name, labels[num_labels-1].address);
			*/
			continue;
		}

		// Tokenize by spaces and commas
		p = strtok(line, " ,\t");
		while (p && num_tokens < 4) {
			tokens[num_tokens++] = p;
			p = strtok(NULL, " ,\t");
		}
		num_tokens = 0;

		// Parsing logic
		if(parse_opcode(line, &opcode, &func)) {
			fprintf(stderr, "parse_opcode() failed\n");
			exit(2);
		}
		if(opcode == OPCODE_NOP) {
			current_line_hex = NOP_Instruction;
		}else if(opcode == OPCODE_RTYPE) {
			// RTYPE
			int rd = parse_register(tokens[1]);
			int rs1 = parse_register(tokens[2]);
			int rs2 = parse_register(tokens[3]);
			current_line_hex = (opcode << 26) | (rs1 << 21) | (rs2 << 16) | (rd << 11) | func;
		}else if(opcode == OPCODE_J || opcode == OPCODE_JAL || opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ || opcode == OPCODE_JR || opcode == OPCODE_JALR) {
			// JTYPE
			if (opcode == OPCODE_JR || opcode == OPCODE_JALR){
				int rs1 = parse_register(tokens[1]);
				current_line_hex = (opcode << 26) | (rs1 << 21);
			} else if (opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ){
				int rs1 = parse_register(tokens[1]);
				int address = find_label_address(tokens[2]);
#ifdef RELATIVE_JUMP
				int current = (line_number - 1) * 4;
				int offset = address - (current);
				current_line_hex = (opcode << 26) | (rs1 << 21) | (offset & 0xFFFF);
#else
				current_line_hex = (opcode << 26) | (rs1 << 21) | (address & 0xFFFF);
#endif
			} else {
				int address = find_label_address(tokens[1]);
#ifdef RELATIVE_JUMP
				int current = (line_number - 1) * 4;
				int offset = address - (current);
				current_line_hex = (opcode << 26) | (offset & 0x03FFFFFF);
#else
				current_line_hex = (opcode << 26) | (address & 0x03FFFFFF);
#endif
			}
		}else {
			// ITYPE
			int rd = parse_register(tokens[1]);
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
