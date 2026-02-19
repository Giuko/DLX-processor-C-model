#include "cpu_model.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void print_state(void *handle){
	cpu_t *cpu;
	uint32_t currentPC;
	uint32_t regs[32]; 
	cpu = (cpu_t*)handle;
	currentPC = cpu_get_pc(cpu);
	
	for(int i = 0; i < 32; i++)
		regs[i] = cpu_get_reg(cpu, i);

	printf("Current PC: %X\n", currentPC*4);
	for(int i = 0; i < 32; i++)
		printf("\tR%-2d: 0x%010x\n", i, regs[i]);
	printf("\n\n\n\n");
}

int main(int argc, char *argv[]) {
	cpu_t *cpu;
	FILE *fd;
	size_t program_size = 0;
	uint32_t *program;
	uint32_t temp;
	int i;
	char *filename;

	if(argc < 2){
		fprintf(stderr, "Wrong usage: %s <filename>", argv[1]);
		exit(-1);
	}
	filename = argv[1];

	fd = fopen(filename, "r");
	if(fd == NULL) {
		fprintf(stderr, "fopen() failed | filename: %s\n", filename);
		exit(-2);
	}

	// Check program size
	program_size = 0;
	while(fscanf(fd, "%x", &temp) == 1)
		program_size++;
	rewind(fd);

	program = (uint32_t*)malloc(sizeof(uint32_t)*program_size);

	i = 0;
	while(i < (int)program_size && (fscanf(fd, "%x", &program[i]) == 1))
		i++;

	fclose(fd);

	cpu = (cpu_t*)cpu_create();
	if(cpu == NULL) {
		fprintf(stderr, "cpu_create() failed\n");
		free(program);
		exit(-3);
	}
	cpu_reset(cpu);
		

	printf("################## PROGRAM #################\n");
	//program_size = 3;
	for(int i = 0; i < (int)program_size; i++) {
		printf("I%-2d: %#010x\n", i, program[i]);
		cpu_load_instr(cpu, i, program[i]);
	
	}
	printf("############################################\n");

	for(int i = 0; i < 4+(int)program_size; i++){
		printf("\n\n\n######## STEP %3d ########\n", i);
		cpu_step(cpu);
	}

	print_state(cpu);
	free(program);
	memory_destroy(cpu);

	return 0;
}
