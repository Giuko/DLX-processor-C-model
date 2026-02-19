#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "cpu_model.h"
#include <stdlib.h>
#include <string.h>

// Create CPU instance
void* cpu_create() {
    cpu_t* cpu = (cpu_t*)malloc(sizeof(cpu_t));
    memset(cpu, 0, sizeof(cpu_t));
	for(int i = 0; i < 1024; i++)
		cpu->IRAM[i] = NOP_Instruction;
    return (void*)cpu;
}

// Reset CPU
void cpu_reset(void* handle) {
    cpu_t* cpu = (cpu_t*)handle;
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
	printf("[FETCH] Instr: %#010x\n", pipeFetch->instr);
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
	printf("[DECODE] OPCODE = %x\n", opcode);

	// Decode instruction
	if (opcode == OPCODE_NOP) {
		// Do nothing
		printf("[DECODE]: NOP\n");
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

		printf("[DECODE]: RTYPE | rs1: R%-2d [%d] | rs2: R%-2d [%d] | r: R%-2d | func: %#06x\n", rs1, rs1_val, rs2, rs2_val, rd, func);

		writeRF = true;

		// Get controls signal to use to the next steps
		
		// To make it simple, the ALU_opcode is the same as func
		ALU_opcode = func;

	} else if (opcode == OPCODE_J || opcode == OPCODE_JAL || opcode == OPCODE_BEQZ || opcode == OPCODE_BNEZ) {	
		// J-Type
		// | opcode (6) | immediate (26) |
		imm = instr & 0x03FFFFFF;
		
		// Sign extension 
		if(imm >> 25)
			imm |= 0xFC000000;

		printf("[DECODE]: JTYPE | imm: %#010x\n", imm);
		
		useImm = true;
		
		// Get controls signal to use to the next steps
		switch (opcode) {
			case OPCODE_J:
				ALU_opcode = FUNC_ADD;
				jmp_eqz_neqz = jump;
				break;
			case OPCODE_JAL:
				ALU_opcode = FUNC_ADD;
				jmp_eqz_neqz = jump_link;
				rd = 31;				// Store the NPC to the register 31
				writeRF = true;
				break;
			case OPCODE_BEQZ:
				ALU_opcode = FUNC_ADD;
				jmp_eqz_neqz = eqz;
				break;
			case OPCODE_BNEZ:
				ALU_opcode = FUNC_ADD;
				jmp_eqz_neqz = neqz;
				break;
			default:
				// If not a branch/jump instr
				jmp_eqz_neqz = nop;
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
		
		printf("[DECODE]: ITYPE | rs1: R%-2d [%d] | r: R%-2d | imm: %#010x\n", rs1, rs1_val, rd, imm);
		
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
	
	// Previous pipe is now useless
	memory_destroy(pipeFetch);
	

	return pipeDecode;
}

// Ex stage
pipeEx_t* instruction_exe(void *handle, pipeDecode_t *pipeDecode) {
	if(handle == NULL){
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
	printf("[EXE] ALU_OPCODE: 0x%x\n", pipeDecode->ALU_opcode);

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


	if(pipeDecode->jmp_eqz_neqz != nop){	// TODO: check mux_a_sel what it is
		operandA = pipeDecode->nextPC;
		printf("[EXE] Using next PC as operand A\n");
	}else {
		operandA = pipeDecode->rs1_val;
		printf("[EXE] Using RS1 as operand A\n");
	}
	if(pipeDecode->useImm){
		operandB = pipeDecode->imm;
		printf("[EXE] Using immediate as operand A\n");
	}else{
		operandB = pipeDecode->rs2_val;
		printf("[EXE] Using RS2 as operand A\n");
	}
	switch (ALU_opcode) {
		case 0:
			printf("[EXE] NOP\n");
			break;
		case FUNC_SLL:
			ALU_out = operandA << operandB;
			printf("[EXE] SLL\n");
			break;
		case FUNC_SRL:
			ALU_out = operandA >> operandB;
			printf("[EXE] SRL\n");
			break;
		case FUNC_SRA:
			// with integer, the >> should be arithmetic
			ALU_out = (int32_t)((int32_t)operandA >> operandB);
			printf("[EXE] SRA\n");
			break;
		case FUNC_ADD:
			ALU_out = (int32_t)((int32_t)operandA + (int32_t)operandB);
			printf("[EXE] ADD\n");
			break;
		case FUNC_ADDU:
			ALU_out = (operandA + operandB);
			printf("[EXE] ADDU\n");
			break;
		case FUNC_SUB:
			ALU_out = (int32_t)((int32_t)operandA - (int32_t)operandB);
			printf("[EXE] SUB\n");
			break;
		case FUNC_SUBU:
			ALU_out = (operandA - operandB);
			printf("[EXE] SUBU\n");
			break;
		case FUNC_AND:
			ALU_out = (operandA & operandB);
			printf("[EXE] AND\n");
			break;
		case FUNC_OR:
			ALU_out = (operandA | operandB);
			printf("[EXE] OR\n");
			break;
		case FUNC_XOR:
			ALU_out = (operandA ^ operandB);
			printf("[EXE] XOR\n");
			break;
		case FUNC_SEQ:
			ALU_out = (operandA == operandB) ? 1 : 0;
			printf("[EXE] SEQ\n");
			break;
		case FUNC_SNE:
			ALU_out = (operandA != operandB) ? 1 : 0;
			printf("[EXE] SNE\n");
			break;
		case FUNC_SLT:
			ALU_out = ((int32_t)operandA < (int32_t)operandB) ? 1 : 0;
			printf("[EXE] SLT\n");
			break;
		case FUNC_SGT:
			ALU_out = ((int32_t)operandA > (int32_t)operandB) ? 1 : 0;
			printf("[EXE] SGT\n");
			break;
		case FUNC_SLE:
			ALU_out = ((int32_t)operandA <= (int32_t)operandB) ? 1 : 0;
			printf("[EXE] SLE\n");
			break;
		case FUNC_SGE:
			ALU_out = ((int32_t)operandA >= (int32_t)operandB) ? 1 : 0;
			printf("[EXE] SGE\n");
			break;
		case FUNC_SLTU:
			ALU_out = ((uint32_t)operandA < (uint32_t)operandB) ? 1 : 0;
			printf("[EXE] SLTU\n");
			break;
		case FUNC_SGTU:
			ALU_out = ((uint32_t)operandA > (uint32_t)operandB) ? 1 : 0;
			printf("[EXE] SGTU\n");
			break;
		case FUNC_SLEU:
			ALU_out = ((uint32_t)operandA <= (uint32_t)operandB) ? 1 : 0;
			printf("[EXE] SLEU\n");
			break;
		case FUNC_SGEU:
			ALU_out = ((uint32_t)operandA >= (uint32_t)operandB) ? 1 : 0;
			printf("[EXE] SGEU\n");
			break;
		default:
			// Should never goes here
			fprintf(stderr, "[EXE] shouldn't be here | ALU_opcode = %x\n", ALU_opcode);
			memory_destroy(pipeDecode);	// Free used mem
			return NULL;
	}		

	switch (pipeDecode->jmp_eqz_neqz) {
		case jump:
			printf("[EXE] JUMP\n");
			toJump = true;
			break;
		case jump_link:
			printf("[EXE] JUMP and LINK\n");
			toJump = true;
			break;
		case eqz:
			printf("[EXE] BEQZ\n");
			toJump = (pipeDecode->rs1_val == 0x0);
			break;
		case neqz:
			printf("[EXE] BNEZ\n");
			toJump = (pipeDecode->rs1_val != 0x0);
			break;
		default:
			break;
	}

	pipeEx->ALU_out = ALU_out;
	pipeEx->jump = toJump;

	// Propagate old signals
	pipeEx->nextPC = pipeDecode->nextPC;
	pipeEx->rs2_val = pipeDecode->rs2_val;
	pipeEx->rd = pipeDecode->rd;

	// Contols
	pipeEx->writeRF = pipeDecode->writeRF;
	pipeEx->writeMem = pipeDecode->writeMem;
	pipeEx->readMem = pipeDecode->readMem;
	
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
		printf("Incrementing PC, not reached Mem access at the moment\n");
		cpu->pc++;
		return NULL;
	}
	pipeMem_t *pipeMem;

	uint32_t DRAM_out=0;

	pipeMem = (pipeMem_t*)malloc(sizeof(pipeMem_t));
	if(pipeMem == NULL) {
		cpu->pc++;
		fprintf(stderr, "Failed to allocate memory for pipeMem\n");
		return NULL;
	}
	memset(pipeMem, 0, sizeof(pipeMem_t));

	uint8_t DRAM_addr = pipeEx->ALU_out;
	uint8_t DRAM_data = pipeEx->rs2_val;
	
	if(pipeEx->readMem) {
		DRAM_out = cpu->DRAM[DRAM_addr];
		printf("[MEM] Reading from memory\n");
	}else if(pipeEx->writeMem) {
		cpu->DRAM[DRAM_addr] = DRAM_data;
		printf("[MEM] Writing to memory\n");
	}
	pipeMem->DRAM_out = DRAM_out;


	// TODO: check when the PC gets updated after the branch resolution
	if(pipeEx->jump){
		//  If jump == true then ALU_out will hold the new PC
		cpu->pc = pipeEx->ALU_out;
		printf("[MEM] JUMPING\n");
	}else {
		cpu->pc++;
		printf("[MEM] Incrementing PC\n");
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

	// Previous pipe is now useless
	memory_destroy(pipeEx);
	
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

	if (pipeMem->writeRF) {
		if(pipeMem->jump) {
			// JAL instruction -- rd set to 31
			val_to_store = pipeMem->nextPC;
			printf("[WB] Storing next PC\n");
		}else if(pipeMem->readMem) {
			// LOAD instruction
			val_to_store = pipeMem->DRAM_out;
			printf("[WB] Saving to R31 due to link\n");
			printf("[WB] Storing DRAM_out\n");
		}else {
			val_to_store = pipeMem->ALU_out;
			printf("[WB] Storing ALU_out\n");
		}
		cpu->regs[pipeMem->rd] = val_to_store;	
		printf("[WB] Storing to R%-2d\n", pipeMem->rd);
	}

	// Previous pipe is now useless
	memory_destroy(pipeMem);
}

// Execute one step
void cpu_step(void* handle) {
	cpu_t* cpu = (cpu_t*)handle;
	static uint8_t iteration = 0; 	
	static pipeFetch_t 	*pipeFetch	= NULL;
	static pipeDecode_t *pipeDecode	= NULL;
	static pipeEx_t 	*pipeEx		= NULL;
	static pipeMem_t	*pipeMem	= NULL;

	// In reverse, in this way it will be feed
	// with the previous pipe
	// At the end of each function the used pipe will be
	// destroyed to avoid overuse of memory
	if(iteration > 3)
		instruction_WB(handle, pipeMem);

	if(iteration > 2)
		pipeMem	= instruction_mem(handle, pipeEx);
	else
		// During normal operation is MEM that will update the PC
		cpu->pc++;
	if(iteration > 1)
		pipeEx = instruction_exe(handle, pipeDecode);
	
	if(iteration > 0)
		pipeDecode= instruction_decode(handle, pipeFetch);
	
	pipeFetch	= instruction_fetch(handle);
	
	if(iteration < 4)
		iteration++;

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


