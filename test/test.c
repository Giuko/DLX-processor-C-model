#include <stdio.h>
#include <stdlib.h>
#include "../test/test.h"
#include "../cpu_model/cpu_model.h"
#include "../inc/utils.h"

// A known program is executed, so the comparison is done, 
// knowing the expected results

int basic_test(void *handle){
	cpu_t *cpu = handle;
	FILE *fd;
	uint32_t temp;
	uint32_t *program;
	uint32_t program_size;
	uint32_t val;
	int i;
    if (cpu == NULL) {
        fprintf(stderr, "[TEST] CPU is NULL\n");
        exit(1);
    }


	fd = fopen("./programs/testprogram.asm.mem", "r");
	if(fd == NULL){
		fprintf(stderr, "[TEST] fopen() failed to open: ../programs/testprogram2.asm\n");
		exit(1);
	}

    while (fscanf(fd, "%x", &temp) == 1)
        program_size++;
	printf("Program size: %d\n", program_size);
    rewind(fd);

    program = (uint32_t *)malloc(sizeof(uint32_t) * program_size);
    i = 0;
    while (i < (int)program_size && fscanf(fd, "%x", &program[i]) == 1)
        i++;
    fclose(fd);
	
	// Load program into the IRAM
    cpu_reset(cpu);
    for (i = 0; i < 1024; i++)
        cpu_load_instr(cpu, i, NOP_Instruction);
    for (i = 0; i < program_size; i++)
        cpu_load_instr(cpu, i, program[i]);

	// Executing each instruction
	i = 0;
	printf("#### STEP %-3d ####\n", ++i);
	cpu_step(cpu);
	printf("\n\n\n\n");
	while(cpu_get_pc(cpu) < (program_size+4)){
		printf("#### STEP %-3d ####\n", ++i);
		cpu_step(cpu);
		printf("\n\n\n\n");
	}
	CLEAR_SCREEN();	
    /* r0 — hardwired zero */
    val =          0; ASSERT(cpu_get_reg(cpu,  0) == val, "R0  = 0 (hardwired)");

    /* Section 9 */
    val =          1; ASSERT(cpu_get_reg(cpu,  1) == val, "R1  = 1");
    val =          2; ASSERT(cpu_get_reg(cpu,  2) == val, "R2  = 2");
    val =          1; ASSERT(cpu_get_reg(cpu,  3) == val, "R3  = 1 (sltu)");
    val =          1; ASSERT(cpu_get_reg(cpu,  4) == val, "R4  = 1 (sgtu)");

    /* Section 2 — ADD / SUB */
    val =         11; ASSERT(cpu_get_reg(cpu,  5) == val, "R5  = 11 (add)");
    val =          9; ASSERT(cpu_get_reg(cpu,  6) == val, "R6  = 9  (sub)");

    /* Section 7 — Load */
    val =         11; ASSERT(cpu_get_reg(cpu,  7) == val, "R7  = 11 (lw MEM[8])");
    val = 0xFFFFFFFF; ASSERT(cpu_get_reg(cpu,  8) == val, "R8  = 0xFFFFFFFF (lw MEM[12])");

    /* Section 3 — Logics R-type */
    val =         15; ASSERT(cpu_get_reg(cpu,  9) == val, "R9  = 15 (0x0F)");
    val =         15; ASSERT(cpu_get_reg(cpu, 10) == val, "R10 = 15 (and)");
    val =        255; ASSERT(cpu_get_reg(cpu, 11) == val, "R11 = 255 (or)");
    val =        240; ASSERT(cpu_get_reg(cpu, 12) == val, "R12 = 240 (xor 0xF0)");

    /* Section 4 — Logica immediata */
    val =         15; ASSERT(cpu_get_reg(cpu, 13) == val, "R13 = 15  (andi)");
    val =        170; ASSERT(cpu_get_reg(cpu, 14) == val, "R14 = 170 (ori 0xAA)");
    val =         85; ASSERT(cpu_get_reg(cpu, 15) == val, "R15 = 85  (xori 0x55)");

    /* Section 5 — Shift */
    val =          1; ASSERT(cpu_get_reg(cpu, 16) == val, "R16 = 1");
    val =        128; ASSERT(cpu_get_reg(cpu, 17) == val, "R17 = 128 (sll 1<<7)");
    val =          1; ASSERT(cpu_get_reg(cpu, 18) == val, "R18 = 1   (srl 128>>7)");
    val = 0xFFFFFFFF; ASSERT(cpu_get_reg(cpu, 19) == val, "R19 = -1  (0xFFFFFFFF)");
    val = 0xFFFFFFFF; ASSERT(cpu_get_reg(cpu, 20) == val, "R20 = -1  (sra -1>>7)");
    val =          8; ASSERT(cpu_get_reg(cpu, 21) == val, "R21 = 8   (slli 1<<3)");
    val =          1; ASSERT(cpu_get_reg(cpu, 22) == val, "R22 = 1   (srli 8>>3)");
    val = 0xFFFFFFFF; ASSERT(cpu_get_reg(cpu, 23) == val, "R23 = -1  (srai -1>>4)");

    /* Section 6 — Set */
    val =          5; ASSERT(cpu_get_reg(cpu, 24) == val, "R24 = 5");
    val =          5; ASSERT(cpu_get_reg(cpu, 25) == val, "R25 = 5");
    val =          3; ASSERT(cpu_get_reg(cpu, 26) == val, "R26 = 3");
    val =          1; ASSERT(cpu_get_reg(cpu, 27) == val, "R27 = 1 (seq 5==5)");
    val =          0; ASSERT(cpu_get_reg(cpu, 28) == val, "R28 = 0 (post loop)");
    val =          1; ASSERT(cpu_get_reg(cpu, 29) == val, "R29 = 1 (slt 3<5)");
    val =          1; ASSERT(cpu_get_reg(cpu, 30) == val, "R30 = 1 (sgt 5>3)");

    /* Section 8 — Loop counter */
    val =          2; ASSERT(cpu_get_reg(cpu, 31) == val, "R31 = 2 (loop count)");
	free(program);
	return 0;
}

int main(int argc, char *argv[]){
	FILE *fd			= NULL;
	int program_size 	= 0;
	uint32_t temp		= 0;
	uint32_t *program	= NULL;
	int i				= 0;
	cpu_t *cpu			= NULL;

    cpu = (cpu_t *)cpu_create();
    if (cpu == NULL) {
        fprintf(stderr, "cpu_create() failed\n");
        free(program);
        exit(-3);
    }
    cpu_reset(cpu);

	// Tests
	basic_test(cpu);

	printf("All test passed\n");


	memory_destroy(cpu);
	return 0;
}
