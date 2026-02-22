#ifndef CPU_MODEL_H
#define CPU_MODEL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NOP_Instruction 0x54000000

#define FUNC_NOP	0x00
#define FUNC_SLL  	0x04
#define FUNC_SRL  	0x06
#define FUNC_SRA  	0x07
#define FUNC_ADD  	0x20
#define FUNC_ADDU 	0x21
#define FUNC_SUB  	0x22
#define FUNC_SUBU 	0x23
#define FUNC_AND  	0x24
#define FUNC_OR   	0x25
#define FUNC_XOR  	0x26
#define FUNC_SEQ  	0x28
#define FUNC_SNE  	0x29
#define FUNC_SLT  	0x2a
#define FUNC_SGT  	0x2b
#define FUNC_SLE  	0x2c
#define FUNC_SGE  	0x2d
#define FUNC_SLTU 	0x3a
#define FUNC_SGTU 	0x3b
#define FUNC_SLEU 	0x3c
#define FUNC_SGEU 	0x3d

#define OPCODE_RTYPE    0x00
#define OPCODE_J        0x02
#define OPCODE_JAL      0x03
#define OPCODE_BEQZ     0x04
#define OPCODE_BNEZ     0x05
#define OPCODE_ADDI     0x08
#define OPCODE_ADDUI    0x09
#define OPCODE_SUBI     0x0A
#define OPCODE_SUBUI    0x0B
#define OPCODE_ANDI     0x0C
#define OPCODE_ORI      0x0D
#define OPCODE_XORI     0x0E
#define OPCODE_JR       0x12
#define OPCODE_JALR     0x13
#define OPCODE_SLLI     0x14
#define OPCODE_NOP      0x15
#define OPCODE_SRLI     0x16
#define OPCODE_SRAI     0x17
#define OPCODE_SEQI     0x18
#define OPCODE_SNEI     0x19
#define OPCODE_SLTI     0x1A
#define OPCODE_SGTI     0x1B
#define OPCODE_SLEI     0x1C
#define OPCODE_SGEI     0x1D
#define OPCODE_LW       0x23
#define OPCODE_SW       0x2B
#define OPCODE_SLTUI    0x3A
#define OPCODE_SGTUI    0x3B
#define OPCODE_SLEUI    0x3C
#define OPCODE_SGEUI    0x3D

typedef enum {
	nop,
	jump,
	jump_link,
	eqz,
	neqz
} jump_t;

// CPU State
typedef struct {
    uint32_t pc;
    uint32_t regs[32];
    uint32_t DRAM[1024];
    uint32_t IRAM[1024];
} cpu_t;
typedef struct {
	uint32_t instr;
	uint32_t nextPC;

	// Extra
	char instr_str[64];
} pipeFetch_t;

// Set correct variable to use
typedef struct {
	// New signals to feed the next steps
	uint32_t rs1_val;
	uint32_t rs2_val;
	uint8_t  rd;
	uint32_t imm;

	// Controls
	uint16_t 	ALU_opcode;
	jump_t 		jmp_eqz_neqz;
	bool 		writeRF;
	bool 		useImm;
	bool 		writeMem;
	bool 		readMem;
	bool		useRegisterToJump;

	// Propagate old signals
	uint32_t nextPC;
	
	// Extra
	char instr_str[64];
} pipeDecode_t;

typedef struct{
	// New signals to feed the next steps
	uint32_t 	ALU_out;	// It can be: 
							// 	the address for MEM
							//	the new PC
							//	output of a computation
	bool		jump;		// If true PC = computedPC

	// Controls
	bool 		writeRF;
	bool 		writeMem;
	bool 		readMem;

	// Propagate old signals
	uint32_t nextPC;
	uint32_t rs1_val;		// To be used as jump register
	uint32_t rs2_val;		// To be used as DRAM_data
	uint8_t  rd;
	bool		useRegisterToJump;
	
	// Extra
	char instr_str[64];
} pipeEx_t;

typedef struct{
	// New signals to feed the next steps
	uint32_t DRAM_out;

	// Controls
	bool	writeRF;
	bool	readMem;
	bool	jump;			// If true PC = computedPC
	
	// Propagate old signals
	uint32_t rs1_val;		// To be used as jump register
	uint32_t ALU_out; 
	uint32_t nextPC;
	uint8_t  rd;
	bool		useRegisterToJump;
	
	// Extra
	char instr_str[64];

} pipeMem_t;

// Create CPU instance
void* cpu_create();

// Reset CPU
void cpu_reset(void* handle); 

// Load instruction into memory
void cpu_load_instr(void* handle, int addr, uint32_t instr);

// Fetch instruction
pipeFetch_t *instruction_fetch(void *handle);

// Decode instruction
// Will works as control unit too
// Returns control useful for next pipes too
pipeDecode_t* instruction_decode(void *handle, pipeFetch_t *pipeFetch); 

// Ex stage
pipeEx_t* instruction_exe(void *handle, pipeDecode_t *pipeDecode);

// Mem stage
pipeMem_t* instruction_mem(void *handle, pipeEx_t *pipeEx);

// WriteBack stage
void instruction_WB(void *handle, pipeMem_t *pipeMem);

// Execute one step
void cpu_step(void* handle);

// Get register value
uint32_t cpu_get_reg(void* handle, int idx);

// Get PC
uint32_t cpu_get_pc(void* handle);

// Destroy instance
void memory_destroy(void* handle);

// Return the string of the instruction
char *identify_instruction(uint32_t instr);

#endif //CPU_MODEL_H
