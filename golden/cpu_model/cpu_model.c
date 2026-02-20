#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "cpu_model.h"
#include <stdlib.h>
#include <string.h>

uint8_t g_iteration = 0; 	

// Create CPU instance
void* cpu_create() {
    cpu_t* cpu = (cpu_t*)malloc(sizeof(cpu_t));
    int i;
	memset(cpu, 0, sizeof(cpu_t));

	for(i = 0; i < 1024; i++)
		cpu->IRAM[i] = NOP_Instruction;
    return (void*)cpu;
}

// Reset CPU
void cpu_reset(void* handle) {
    cpu_t* cpu = (cpu_t*)handle;
    g_iteration = 0;
	cpu->pc = -1;
    memset(cpu->regs, 0, sizeof(cpu->regs));
}

// Load instruction into memory
void cpu_load_instr(void* handle, int addr, uint32_t instr) {
    cpu_t* cpu = (cpu_t*)handle;
	if(instr == 0x0)
		cpu->IRAM[addr] = NOP_Instruction;
	else
    	cpu->IRAM[addr] = instr;				// Or addr/4
}

// This is a pipelined processor
// IF -> ID -> EX -> MEM -> WB

// Fetch instruction
// Returns instr and nextPc
pipeFetch_t *instruction_fetch(void *handle) {
	if(handle == NULL){
		fprintf(stderr, "Failed to access CPU\n");
		return NULL;
	}
	cpu_t *cpu = (cpu_t*) handle;
	pipeFetch_t *pipeFetch;
	pipeFetch = (pipeFetch_t*)malloc(sizeof(pipeFetch_t));
	if(pipeFetch == NULL){
		fprintf(stderr, "Failed to allocate memory for pipeFetch\n");
		return NULL;
	}
	pipeFetch->instr = cpu->IRAM[cpu->pc];
	// In this implementation is ok to add 1 and not 4 since 
	// we're grouping the PC as group of 4 bytes
#ifdef DEBUG
	printf("[FETCH] Instr: %#010x\n", pipeFetch->instr);
#endif
	strcpy(pipeFetch->instr_str, identify_instruction(pipeFetch->instr));
	pipeFetch->nextPC = (cpu->pc+1)*4;
	printf("[FETCH] %s\n", pipeFetch->instr_str);
	return pipeFetch;	
}

// Decode instruction
// Will works as control unit too
// Returns control useful for next pipes too
pipeDecode_t* instruction_decode(void *handle, pipeFetch_t *pipeFetch) {
	if(handle == NULL){
		fprintf(stderr, "Failed to access CPU\n");
		memory_destroy(pipeFetch);
		return NULL;
	}
	if (pipeFetch == NULL) {
		fprintf(stderr, "Failed to access pipeFetch or not reached yet\n");
		return NULL;
	}
	cpu_t *cpu = (cpu_t*)handle;
	pipeDecode_t *pipeDecode;
	uint32_t instr = pipeFetch->instr;
	uint32_t nextPC  = pipeFetch->nextPC;
	uint32_t opcode;
	uint16_t func;
	uint8_t  rd		 = 0;
	uint8_t  rs1	 = 0;
	uint8_t  rs2	 = 0;
	uint32_t rs1_val = 0;
	uint32_t rs2_val = 0;
	uint32_t imm	 = 0;

	// Controls
	uint16_t 		ALU_opcode   = FUNC_NOP;
	jump_t   		jmp_eqz_neqz = nop;
	bool			writeRF 	 = false;
	bool			writeMem	 = false;
	bool 			readMem		 = false;
	bool			useImm		 = false;



	pipeDecode = (pipeDecode_t*)malloc(sizeof(pipeDecode_t));
	if(pipeDecode == NULL){
		fprintf(stderr, "Failed to allocate memory for pipeDecode\n");
		return NULL;
	}
	memset(pipeDecode, 0, sizeof(pipeDecode_t));

	// Get opcode
	opcode = (instr >> (32-6)) & 0x3F;
#ifdef DEBUG
	printf("[DECODE] OPCODE = 0x%02x\n", opcode);
#endif
	// Decode instruction
	if (opcode == OPCODE_NOP) {
		// Do nothing
		printf("[DECODE] NOP\n");
		return pipeDecode;
	} else if (opcode == 0x00) {	
		// R-Type
		// | opcode (6) | rs1 (5) | rs2 (5) | rd (5) | func (11) |
		
		rs1  = (instr >> (32-11)) & 0x1F;
		rs2  = (instr >> (32-16)) & 0x1F;
		rd   = (instr >> (32-21)) & 0x1F;
		func = instr & 0x7FF;

		// Acces RF to read the registers
		if(rs1 == 0)
			rs1_val = 0;
		else
			rs1_val = cpu->regs[rs1];
		
		if(rs2 == 0)
			rs2_val = 0;
		else
			rs2_val = cpu->regs[rs2];

#ifdef DEBUG
		printf("[DECODE]: RTYPE | rs1: R%-2d [%d] | rs2: R%-2d [%d] | r: R%-2d | func: %#06x\n", rs1, rs1_val, rs2, rs2_val, rd, func);
#endif

		writeRF = true;

		// Get controls signal to use to the next steps
		
		// To make it simple, the ALU_opcode is the same as func
		ALU_opcode = func;

	} else if (opcode == OPCODE_J || opcode == OPCODE_JAL || opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ) {	
		// J-Type
		// | opcode (6) | immediate (26) |
		// Branch
		// | opcode (6) | rs1 (5) | xxxx (5) | immediate (16) | [Itype]
		if(opcode == OPCODE_J || opcode == OPCODE_JAL)	{
			imm = instr & 0x03FFFFFF;
			// Sign extension 
			if(imm >> 25)
				imm |= 0xFC000000;
		} else {
			imm = instr & 0xFFFF;
			
			// Sign extension
			if(imm >> 15)
				imm |= 0xFFFF0000;
		}
#ifdef DEBUG
		printf("[DECODE]: JTYPE | imm: %#010x\n", imm);
#endif

		useImm = true;
		ALU_opcode = FUNC_ADDU;
		
		// Get controls signal to use to the next steps
		switch (opcode) {
			case OPCODE_J:
				jmp_eqz_neqz = jump;
				break;
			case OPCODE_JAL:
				jmp_eqz_neqz = jump_link;
				rd = 31;				// Store the NPC to the register 31
				writeRF = true;
				break;
			case OPCODE_BEQZ:
				rs1 = (instr >> (32-11)) & 0x1F;
				rs1_val = cpu->regs[rs1];
				jmp_eqz_neqz = eqz;
				break;
			case OPCODE_BNEZ:
				rs1 = (instr >> (32-11)) & 0x1F;
				rs1_val = cpu->regs[rs1];
				jmp_eqz_neqz = neqz;
				break;
			default:
				// If not a branch/jump instr
				// Should never goes here
				fprintf(stderr, "[Decode] I-Type - shouldn't be here\n");
				memory_destroy(pipeFetch);	// Free used mem
				return NULL;
				break;
		}
	} else { 
		// I-Type
		// | opcode (6) | rs1 (5) | rd (5) | immediate (16) |
		
		rs1 = (instr >> (32-11)) & 0x1F;
		rd  = (instr >> (32-16)) & 0x1F;
		imm = instr & 0xFFFF;
		
		// Sign extension
		if(imm >> 15)
			imm |= 0xFFFF0000;
		
		// Acces RF to read the registers
		rs1_val = cpu->regs[rs1];
		
#ifdef DEBUG
		printf("[DECODE]: ITYPE | rs1: R%-2d [%d] | r: R%-2d | imm: %#010x\n", rs1, rs1_val, rd, imm);
#endif

		useImm  = true;
		writeRF = true;

		// Get controls signal to use to the next steps
		switch (opcode) {
			case OPCODE_ADDI:
				ALU_opcode = FUNC_ADD;
				break;
			case OPCODE_ADDUI:
				ALU_opcode = FUNC_ADDU;
				break;
			case OPCODE_SUBI:
				ALU_opcode = FUNC_SUB;
				break;
			case OPCODE_SUBUI:
				ALU_opcode = FUNC_SUBU;
				break;
			case OPCODE_ANDI:
				ALU_opcode = FUNC_AND;
				break;
			case OPCODE_ORI:
				ALU_opcode = FUNC_OR;
				break;
			case OPCODE_XORI:
				ALU_opcode = FUNC_XOR;
				break;
			case OPCODE_SLLI:
				ALU_opcode = FUNC_SLL;
				break;
			case OPCODE_SRLI:
				ALU_opcode = FUNC_SRL;
				break;
			case OPCODE_SRAI:
				ALU_opcode = FUNC_SRA;
				break;
			case OPCODE_SEQI:
				ALU_opcode = FUNC_SEQ;
				break;
			case OPCODE_SNEI:
				ALU_opcode = FUNC_SNE;
				break;
			case OPCODE_SLTI:
				ALU_opcode = FUNC_SLT;
				break;
			case OPCODE_SGTI:
				ALU_opcode = FUNC_SGT;
				break;
			case OPCODE_SLEI:
				ALU_opcode = FUNC_SLE;
				break;
			case OPCODE_SGEI:
				ALU_opcode = FUNC_SGE;
				break;
			case OPCODE_SLTUI:
				ALU_opcode = FUNC_SLTU;
				break;
			case OPCODE_SGTUI:
				ALU_opcode = FUNC_SGTU;
				break;
			case OPCODE_SLEUI:
				ALU_opcode = FUNC_SLEU;
				break;
			case OPCODE_SGEUI:
				ALU_opcode = FUNC_SGEU;
				break;
			case OPCODE_LW:
				readMem = true;
				break;
			case OPCODE_SW:
				// The only I-Type instruction that doesn't 
				// need to save into the registers
				writeRF  = false;
				writeMem = true;
				rs2_val = cpu->regs[rd];		// In case of a store, mem[rs1_val + offset] = rs2_val (R[rd]) 
				break;
			default:
				// Should never goes here
				fprintf(stderr, "[Decode] I-Type - shouldn't be here\n");
				memory_destroy(pipeFetch);	// Free used mem
				return NULL;
		}	
	}



	pipeDecode->nextPC 	= nextPC;
	pipeDecode->rd 		= rd;
	pipeDecode->rs1_val = rs1_val;
	pipeDecode->rs2_val = rs2_val;
	pipeDecode->imm 	= imm;

	pipeDecode->jmp_eqz_neqz	= jmp_eqz_neqz;
	pipeDecode->ALU_opcode		= ALU_opcode;
	pipeDecode->writeRF 		= writeRF;
	pipeDecode->useImm			= useImm;
	pipeDecode->writeMem		= writeMem;
	pipeDecode->readMem			= readMem;
	
	strcpy(pipeDecode->instr_str, pipeFetch->instr_str);
	printf("[DECODE] %s\n", pipeDecode->instr_str);
	// Previous pipe is now useless
	memory_destroy(pipeFetch);
	

	return pipeDecode;
}

// Ex stage
pipeEx_t* instruction_exe(void *handle, pipeDecode_t *pipeDecode) {
	cpu_t *cpu = (cpu_t*)handle;
	if(cpu == NULL){
		fprintf(stderr, "Failed to access CPU\n");
		memory_destroy(pipeDecode);
		return NULL;
	}
	if (pipeDecode == NULL) {
		fprintf(stderr, "Failed to access pipeDecode or not reached yet\n");
		return NULL;
	}
//	cpu_t *cpu = (cpu_t*)handle;
	pipeEx_t *pipeEx;
	uint16_t ALU_opcode = pipeDecode->ALU_opcode;
#ifdef DEBUG
	printf("[EXE] ALU_OPCODE: 0x%x\n", pipeDecode->ALU_opcode);
#endif

	uint32_t 	ALU_out;
	bool		toJump = false;

	uint32_t operandA;
	uint32_t operandB;

	pipeEx = (pipeEx_t*)malloc(sizeof(pipeEx_t));
	if(pipeEx == NULL) {
		fprintf(stderr, "Failed to allocate memory for pipeEx\n");
		return NULL;
	}
	memset(pipeEx, 0, sizeof(pipeEx_t));


	if(pipeDecode->jmp_eqz_neqz != nop){		// TODO: check mux_a_sel what it is
#ifdef RELATIVE_JUMP
		operandA = pipeDecode->nextPC;		// We're using pc as multiply of 4 inside the datapath
#else		
		operandA = 0;
#endif
#ifdef DEBUG
#ifdef RELATIVE_JUMP
		printf("[EXE] Using next PC as operand A: 0x%08x\n", operandA);
#else		
		printf("[EXE] Jumping, 0x0 as operand A\n");
#endif
#endif
	}else {
		operandA = pipeDecode->rs1_val;
#ifdef DEBUG
		printf("[EXE] Using RS1 as operand A: 0x%08x\n", operandA);
#endif
	}
	if(pipeDecode->useImm){
		operandB = pipeDecode->imm;
#ifdef DEBUG
		printf("[EXE] Using immediate as operand B: 0x%08x\n", operandB);
#endif
	}else{
		operandB = pipeDecode->rs2_val;
#ifdef DEBUG
		printf("[EXE] Using RS2 as operand B: 0x%08x\n", operandB);
#endif
	}
	switch (ALU_opcode) {
		case 0:
#ifdef DEBUG
			printf("[EXE] NOP\n");
#endif
			break;
		case FUNC_SLL:
			ALU_out = operandA << operandB;
#ifdef DEBUG
			printf("[EXE] SLL\n");
#endif
			break;
		case FUNC_SRL:
			ALU_out = operandA >> operandB;
#ifdef DEBUG
			printf("[EXE] SRL\n");
#endif
			break;
		case FUNC_SRA:
			// with integer, the >> should be arithmetic
			ALU_out = (int32_t)((int32_t)operandA >> operandB);
#ifdef DEBUG
			printf("[EXE] SRA\n");
#endif
			break;
		case FUNC_ADD:
			ALU_out = (int32_t)((int32_t)operandA + (int32_t)operandB);
#ifdef DEBUG
			printf("[EXE] ADD\n");
#endif
			break;
		case FUNC_ADDU:
			ALU_out = (operandA + operandB);
#ifdef DEBUG
			printf("[EXE] ADDU\n");
#endif
			break;
		case FUNC_SUB:
			ALU_out = (int32_t)((int32_t)operandA - (int32_t)operandB);
#ifdef DEBUG
			printf("[EXE] SUB\n");
#endif
			break;
		case FUNC_SUBU:
			ALU_out = (operandA - operandB);
#ifdef DEBUG
			printf("[EXE] SUBU\n");
#endif
			break;
		case FUNC_AND:
			ALU_out = (operandA & operandB);
#ifdef DEBUG
			printf("[EXE] AND\n");
#endif
			break;
		case FUNC_OR:
			ALU_out = (operandA | operandB);
#ifdef DEBUG
			printf("[EXE] OR\n");
#endif
			break;
		case FUNC_XOR:
			ALU_out = (operandA ^ operandB);
#ifdef DEBUG
			printf("[EXE] XOR\n");
#endif
			break;
		case FUNC_SEQ:
			ALU_out = (operandA == operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SEQ\n");
#endif
			break;
		case FUNC_SNE:
			ALU_out = (operandA != operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SNE\n");
#endif
			break;
		case FUNC_SLT:
			ALU_out = ((int32_t)operandA < (int32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SLT\n");
#endif
			break;
		case FUNC_SGT:
			ALU_out = ((int32_t)operandA > (int32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SGT\n");
#endif
			break;
		case FUNC_SLE:
			ALU_out = ((int32_t)operandA <= (int32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SLE\n");
#endif
			break;
		case FUNC_SGE:
			ALU_out = ((int32_t)operandA >= (int32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SGE\n");
#endif
			break;
		case FUNC_SLTU:
			ALU_out = ((uint32_t)operandA < (uint32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SLTU\n");
#endif
			break;
		case FUNC_SGTU:
			ALU_out = ((uint32_t)operandA > (uint32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SGTU\n");
#endif
			break;
		case FUNC_SLEU:
			ALU_out = ((uint32_t)operandA <= (uint32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SLEU\n");
#endif
			break;
		case FUNC_SGEU:
			ALU_out = ((uint32_t)operandA >= (uint32_t)operandB) ? 1 : 0;
#ifdef DEBUG
			printf("[EXE] SGEU\n");
#endif
			break;
		default:
			// Should never goes here
			fprintf(stderr, "[EXE] shouldn't be here | ALU_opcode = %x\n", ALU_opcode);
			memory_destroy(pipeDecode);	// Free used mem
			return NULL;
	}		

	switch (pipeDecode->jmp_eqz_neqz) {
		case jump:
#ifdef DEBUG
			printf("[EXE] JUMP\n");
#endif
			toJump = true;
			break;
		case jump_link:
#ifdef DEBUG
			printf("[EXE] JUMP and LINK\n");
#endif
			toJump = true;
			break;
		case eqz:
#ifdef DEBUG
			printf("[EXE] BEQZ\n");
#endif
			toJump = (pipeDecode->rs1_val == 0x0);
			break;
		case neqz:
#ifdef DEBUG
			printf("[EXE] BNEZ\n");
#endif
			toJump = (pipeDecode->rs1_val != 0x0);
			break;
		default:
			break;
	}

	pipeEx->ALU_out = ALU_out;
	pipeEx->jump = toJump;
#ifdef DELAYSLOT1
	if(pipeEx->jump)
		cpu->pc = pipeEx->ALU_out/4;
	else
		cpu->pc++; 
#endif

	// Propagate old signals
	pipeEx->nextPC = pipeDecode->nextPC;
	pipeEx->rs2_val = pipeDecode->rs2_val;
	pipeEx->rd = pipeDecode->rd;

	// Contols
	pipeEx->writeRF = pipeDecode->writeRF;
	pipeEx->writeMem = pipeDecode->writeMem;
	pipeEx->readMem = pipeDecode->readMem;
	
	strcpy(pipeEx->instr_str, pipeDecode->instr_str);
	if(strlen(pipeEx->instr_str) == 0)
		printf("[EXE] NOP\n");
	else
		printf("[EXE] %s\n", pipeEx->instr_str);

	// Previous pipe is now useless
	memory_destroy(pipeDecode);
	
	return pipeEx;
}


// Mem stage
pipeMem_t* instruction_mem(void *handle, pipeEx_t *pipeEx){
	if(handle == NULL){
		fprintf(stderr, "Failed to access CPU\n");
		memory_destroy(pipeEx);
		return NULL;
	}
	cpu_t *cpu = (cpu_t*)handle;
	if(pipeEx == NULL){
		fprintf(stderr, "Failed to access pipeEx or not reached yet\n");
		return NULL;
	}
	pipeMem_t *pipeMem;

	uint32_t DRAM_out=0;

	pipeMem = (pipeMem_t*)malloc(sizeof(pipeMem_t));
	if(pipeMem == NULL) {
		fprintf(stderr, "Failed to allocate memory for pipeMem\n");
		return NULL;
	}
	memset(pipeMem, 0, sizeof(pipeMem_t));

	uint8_t DRAM_addr = pipeEx->ALU_out;
	uint8_t DRAM_data = pipeEx->rs2_val;
	
	if(pipeEx->readMem) {
		DRAM_out = cpu->DRAM[DRAM_addr];
#ifdef DEBUG
		printf("[MEM] Reading from memory\n");
#endif
	}else if(pipeEx->writeMem) {
		cpu->DRAM[DRAM_addr] = DRAM_data;
#ifdef DEBUG
		printf("[MEM] Writing to memory\n");
#endif
	}
	pipeMem->DRAM_out = DRAM_out;


	// TODO: check when the PC gets updated after the branch resolution
	if(pipeEx->jump){
		//  If jump == true then ALU_out will hold the new PC
		pipeMem->nextPC = pipeEx->ALU_out/4;
#ifdef DEBUG
		printf("[MEM] JUMPING at 0x%08x\n", pipeMem->nextPC*4);
#endif
	}else {
#ifdef DEBUG
		printf("[MEM] Incrementing PC\n");
#endif
	}
	// New signals to feed the next steps
	pipeMem->DRAM_out	= DRAM_out;

	// Propagate old signals
	pipeMem->ALU_out	= pipeEx->ALU_out;
	pipeMem->nextPC		= pipeEx->nextPC;
	pipeMem->rd			= pipeEx->rd;

	// Controls
	pipeMem->writeRF	= pipeEx->writeRF;
	pipeMem->readMem	= pipeEx->readMem;
	pipeMem->jump		= pipeEx->jump;

	strcpy(pipeMem->instr_str, pipeEx->instr_str);
	if(strlen(pipeMem->instr_str) == 0)
		printf("[MEM] NOP\n");
	else
		printf("[MEM] %s\n", pipeMem->instr_str);
	
	// Previous pipe is now useless
	memory_destroy(pipeEx);

#ifdef DELAYSLOT2
	if(pipeMem->jump)
		cpu->pc = pipeMem->ALU_out/4;
	else
		cpu->pc++; 
#endif
	return pipeMem;
}

void instruction_WB(void *handle, pipeMem_t *pipeMem){
	if(handle == NULL){
		fprintf(stderr, "Failed to access CPU\n");
		memory_destroy(pipeMem);
		return;
	}
	if(pipeMem == NULL){
		fprintf(stderr, "Failed to access pipeMem or not reached yet\n");
		return;
	}

	cpu_t *cpu = (cpu_t*)handle;
	uint32_t val_to_store;
#ifdef DELAYSLOT3
	if(pipeMem->jump)
		cpu->pc = pipeMem->ALU_out/4;
	else
		cpu->pc++; 
#endif
	if (pipeMem->writeRF) {
		if(pipeMem->jump) {
			// JAL instruction -- rd set to 31
			val_to_store = pipeMem->nextPC;
#ifdef DEBUG
			printf("[WB] Storing next PC: 0x%08x\n", val_to_store);
#endif
		}else if(pipeMem->readMem) {
			// LOAD instruction
			val_to_store = pipeMem->DRAM_out;
#ifdef DEBUG
			printf("[WB] Saving to R31 due to link\n");
			printf("[WB] Storing DRAM_out\n");
#endif
		}else {
			val_to_store = pipeMem->ALU_out;
#ifdef DEBUG
			printf("[WB] Storing ALU_out\n");
#endif
		}
		cpu->regs[pipeMem->rd] = val_to_store;	
#ifdef DEBUG
		printf("[WB] Storing to R%-2d\n", pipeMem->rd);
#endif
	}

	if(strlen(pipeMem->instr_str) == 0)
		printf("[WB] NOP\n");
	else
		printf("[WB] %s\n", pipeMem->instr_str);
	// Previous pipe is now useless
	memory_destroy(pipeMem);
}

// Execute one step
void cpu_step(void* handle) {
	cpu_t* cpu = (cpu_t*)handle;
	static pipeFetch_t 	*pipeFetch	= NULL;
	static pipeDecode_t *pipeDecode	= NULL;
	static pipeEx_t 	*pipeEx		= NULL;
	static pipeMem_t	*pipeMem	= NULL;

	// In reverse, in this way it will be feed
	// with the previous pipe
	// At the end of each function the used pipe will be
	// destroyed to avoid overuse of memory
	if(g_iteration > 3)
		instruction_WB(handle, pipeMem);
#ifdef DELAYSLOT3
	else
		cpu->pc++; // During normal operation is WB that will update the PC
#endif

	if(g_iteration > 2)
		pipeMem	= instruction_mem(handle, pipeEx);
#ifdef DELAYSLOT2
	else
		cpu->pc++; // During normal operation is WB that will update the PC
#endif

	if(g_iteration > 1)
		pipeEx = instruction_exe(handle, pipeDecode);
#ifdef DELAYSLOT1
	else
		cpu->pc++; // During normal operation is WB that will update the PC
#endif
	
	if(g_iteration > 0)
		pipeDecode= instruction_decode(handle, pipeFetch);
	
	pipeFetch	= instruction_fetch(handle);
	
	if(g_iteration < 4)
		g_iteration++;

}

// Get register value
uint32_t cpu_get_reg(void* handle, int idx) {
    cpu_t* cpu = (cpu_t*)handle;
    return cpu->regs[idx];
}

// Get PC
uint32_t cpu_get_pc(void* handle) {
    cpu_t* cpu = (cpu_t*)handle;
    return cpu->pc;
}

// Destroy instance
void memory_destroy(void* handle) {
    free(handle);
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
	} else if(opcode == OPCODE_J || opcode == OPCODE_JAL || opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ) { 
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
		snprintf(instr_str+len, 64-len, " R%u, R%u, 0x%08x", rd, rs1, imm);
	}


	return instr_str;
}

