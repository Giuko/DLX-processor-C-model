#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <cpu_model/cpu_model.h>
#include <stdlib.h>
#include <string.h>

// Print only if DEBUG is defined
void print_debug(char *s){
#ifdef DEBUG
	printf(s);
#endif
	return;
}

// Create CPU instance
void* cpu_create() {

#ifndef DELAYSLOT3
#ifndef DELAYSLOT2
#ifndef DELAYSLOT1
	fprintf(stderr, "[CRITICAL ERROR] DELAYSLOT not defined correctly\n");
	exit(-1);
#endif
#endif
#endif
	cpu_t* cpu = (cpu_t*)malloc(sizeof(cpu_t));
	if(cpu == NULL){
		fprintf(stderr, "[CPU CREATE] malloc() failed\n");
		return NULL;
	}
    int i;
	memset(cpu, 0, sizeof(cpu_t));

	for(i = 0; i < IRAM_DEPTH; i++){
		cpu->IRAM[i] = NOP_Instruction;
	}
	for(i = 0; i < DRAM_DEPTH; i++){
		cpu->DRAM[i] = 0x0;
	}
    return (void*)cpu;
}

// Reset CPU
void cpu_reset(void* handle) {
	int i;
    cpu_t* cpu = (cpu_t*)handle;
    
	if(cpu == NULL){
		fprintf(stderr, "[CPU RESET] CPU is NULL\n");
		return;
	}

	cpu->iteration = 0;
	cpu->pc = -1;
	for(i = 0; i < DRAM_DEPTH; i++){
		cpu->DRAM[i] = 0x0;
	}
    
	free(cpu->pipeFetch);
	free(cpu->pipeDecode);
	free(cpu->pipeEx);
	free(cpu->pipeMem);

	cpu->pipeFetch	= NULL;
	cpu->pipeDecode	= NULL;
	cpu->pipeEx		= NULL;
	cpu->pipeMem	= NULL;

	memset(cpu->regs, 0, sizeof(cpu->regs));
}

// Load instruction into memory
void cpu_load_instr(void* handle, uint32_t addr, uint32_t instr) {
    cpu_t* cpu = (cpu_t*)handle;
	if(addr >= IRAM_DEPTH){
		fprintf(stderr, "[IRAM] Address not correct: %d\n", addr);
		return;
	}
	if(instr == 0x0)
		cpu->IRAM[addr] = NOP_Instruction;
	else
    	cpu->IRAM[addr] = instr;				// Or addr/4
}

// Get register value
uint32_t cpu_get_reg(void* handle, int idx) {
    cpu_t* cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "[cpu_get_reg] CPU is NULL\n");
		return 0;
	}
	if(idx < 0 || idx >= REGS_NUM) {
		fprintf(stderr, "[WARNING] Register index is wrong: %d\n", idx);
		return 0;
	}
    return cpu->regs[idx];
}

// Write register value
void cpu_write_reg(void* handle, int idx, uint32_t data) {
    cpu_t* cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "[cpu_get_reg] CPU is NULL\n");
		return;
	}
	if(idx < 0 || idx >= REGS_NUM) {
		fprintf(stderr, "[WARNING] Register index is wrong: %d\n", idx);
		return;
	}
    cpu->regs[idx] = data;
}

// Get PC
uint32_t cpu_get_pc(void* handle) {
    cpu_t* cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "[cpu_get_pc] CPU is NULL\n");
		return 0;
	}
    return cpu->pc;
}


// Get a data from the memory
uint32_t cpu_get_mem_data(void *handle, uint32_t addr){	
	cpu_t *cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "[cpu_get_mem_access] CPU is NULL\n");
		return 0;
	}
	
	if(addr >= DRAM_DEPTH){
		fprintf(stderr, "[WARNING] DRAM wrong address: %d\n", addr);
		return 0;
	}

	return cpu->DRAM[addr];
}


// Write data to memory
void cpu_write_mem_data(void *handle, uint32_t addr, uint32_t data){	
	cpu_t *cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "[cpu_get_mem_access] CPU is NULL\n");
		return;
	}
	
	if(addr >= DRAM_DEPTH){
		fprintf(stderr, "[WARNING] DRAM wrong address: %d\n", addr);
		return;
	}

	cpu->DRAM[addr] = data;
}

uint32_t cpu_get_instr(void *handle, uint32_t addr){
	cpu_t* cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "[cpu_get_pc] CPU is NULL\n");
		return 0;
	}
	if(addr >= IRAM_DEPTH) {
		fprintf(stderr, "[WARNING] IRAM wrong address: %d\n", addr);
		return 0;
	}
	return cpu->IRAM[addr];
}

char *identify_instruction(uint32_t instr){
	char *instr_str;
	instr_str = (char*)malloc(64*sizeof(char));
	if(instr_str == NULL){
		fprintf(stderr, "malloc() failed");
		return NULL;
	}
	uint8_t opcode = (instr >> (32-6)) & 0x3F;
	uint8_t rs1, rs2, rd;
	uint16_t func;
	char *func_str;
	uint32_t imm;
	size_t len;

	switch (opcode){
		//case OPCODE_RTYPE:	  strcpy(instr_str, "RTYPE "); break;
		case OPCODE_J:        strcpy(instr_str, "J     "); break;
		case OPCODE_JAL:      strcpy(instr_str, "JAL   "); break;
		case OPCODE_BEQZ:     strcpy(instr_str, "BEQZ  "); break;
		case OPCODE_BNEZ:     strcpy(instr_str, "BNEZ  "); break;
		case OPCODE_ADDI:     strcpy(instr_str, "ADDI  "); break;
		case OPCODE_ADDUI:    strcpy(instr_str, "ADDUI "); break;
		case OPCODE_SUBI:     strcpy(instr_str, "SUBI  "); break;
		case OPCODE_SUBUI:    strcpy(instr_str, "SUBUI "); break;
		case OPCODE_ANDI:     strcpy(instr_str, "ANDI  "); break;
		case OPCODE_ORI:      strcpy(instr_str, "ORI   "); break;
		case OPCODE_XORI:     strcpy(instr_str, "XORI  "); break;
		case OPCODE_JR:       strcpy(instr_str, "JR    "); break;
		case OPCODE_JALR:     strcpy(instr_str, "JALR  "); break;
		case OPCODE_SLLI:     strcpy(instr_str, "SLLI  "); break;
		case OPCODE_NOP:      strcpy(instr_str, "NOP   "); break;
		case OPCODE_SRLI:     strcpy(instr_str, "SRLI  "); break;
		case OPCODE_SRAI:     strcpy(instr_str, "SRAI  "); break;
		case OPCODE_SEQI:     strcpy(instr_str, "SEQI  "); break;
		case OPCODE_SNEI:     strcpy(instr_str, "SNEI  "); break;
		case OPCODE_SLTI:     strcpy(instr_str, "SLTI  "); break;
		case OPCODE_SGTI:     strcpy(instr_str, "SGTI  "); break;
		case OPCODE_SLEI:     strcpy(instr_str, "SLEI  "); break;
		case OPCODE_SGEI:     strcpy(instr_str, "SGEI  "); break;
		case OPCODE_LW:       strcpy(instr_str, "LW    "); break;
		case OPCODE_SW:       strcpy(instr_str, "SW    "); break;
		case OPCODE_SLTUI:    strcpy(instr_str, "SLTUI "); break;
		case OPCODE_SGTUI:    strcpy(instr_str, "SGTUI "); break;
		case OPCODE_SLEUI:    strcpy(instr_str, "SLEUI "); break;
		case OPCODE_SGEUI:    strcpy(instr_str, "SGEUI "); break;

		default:
			strcpy(instr_str, "");
			break;
	}
	
	len = strlen(instr_str);

	if (opcode == OPCODE_RTYPE) {
		// RTYPE
		rs1  = (instr >> (32-11)) & 0x1F;
		rs2  = (instr >> (32-16)) & 0x1F;
		rd   = (instr >> (32-21)) & 0x1F;
		func = instr & 0x7FF;

		switch (func){
			case FUNC_NOP:  func_str = "NOP  "; break;
			case FUNC_SLL:  func_str = "SLL  "; break;
			case FUNC_SRL:  func_str = "SRL  "; break;
			case FUNC_SRA:  func_str = "SRA  "; break;
			case FUNC_ADD:  func_str = "ADD  "; break;
			case FUNC_ADDU: func_str = "ADDU "; break;
			case FUNC_SUB:  func_str = "SUB  "; break;
			case FUNC_SUBU: func_str = "SUBU "; break;
			case FUNC_AND:  func_str = "AND  "; break;
			case FUNC_OR:   func_str = "OR   "; break;
			case FUNC_XOR:  func_str = "XOR  "; break;
			case FUNC_SEQ:  func_str = "SEQ  "; break;
			case FUNC_SNE:  func_str = "SNE  "; break;
			case FUNC_SLT:  func_str = "SLT  "; break;
			case FUNC_SGT:  func_str = "SGT  "; break;
			case FUNC_SLE:  func_str = "SLE  "; break;
			case FUNC_SGE:  func_str = "SGE  "; break;
			case FUNC_SLTU: func_str = "SLTU "; break;
			case FUNC_SGTU: func_str = "SGTU "; break;
			case FUNC_SLEU: func_str = "SLEU "; break;
			case FUNC_SGEU: func_str = "SGEU "; break;
			default:        func_str = "UNKNOWN"; break;
		}

		//snprintf(instr_str+len, 64-len, " | rs1=%u | rs2=%u | rd=%u | func=%s",rs1, rs2, rd, func_str);
		snprintf(instr_str+len, 64-len, "%s  R%u, R%u, R%-2u ",func_str, rd, rs1, rs2);
	} else if(opcode == OPCODE_J || opcode == OPCODE_JAL || opcode == OPCODE_JR || opcode == OPCODE_JALR) { 
		// JTYPE
		imm = instr & 0x03FFFFFF;
		
		// Sign extension 
		if(imm >> 25)
			imm |= 0xFC000000;

		snprintf(instr_str+len, 64-len, " 0x%08x", imm);

	} else if(opcode != OPCODE_NOP) {
		// ITYPE
		rs1  = (instr >> (32-11)) & 0x1F;
		rd   = (instr >> (32-16)) & 0x1F;
		imm  = (instr & 0xFFFF); 
		// Sign extension
		if(imm >> 15)
			imm |= 0xFFFF0000;

		//snprintf(instr_str+len, 64-len, " rs1=%u | rd=%u | imm=0x%08x",rs1, rd, imm);
		if (opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ)
			snprintf(instr_str+len, 64-len, " R%u, 0x%08x", rs1, imm);
		else
			snprintf(instr_str+len, 64-len, " R%u, R%u, 0x%08x", rd, rs1, imm);
	}


	return instr_str;
}

