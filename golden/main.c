#include "cpu_model.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

void print_state(void *handle){
	cpu_t *cpu;
	uint32_t currentPC;
	uint32_t regs[32]; 
	int i;
	cpu = (cpu_t*)handle;
	currentPC = cpu_get_pc(cpu);
	
	for(i = 0; i < 32; i++)
		regs[i] = cpu_get_reg(cpu, i);

	printf("Current PC: %X\n", currentPC*4);
	for(i = 0; i < 32; i++)
		printf("\tR%-2d: 0x%010x\n", i, regs[i]);
	printf("\n\n\n\n");
}

void press_and_continue(void *handle){
	struct termios oldt, newt;
    int ch;
	// Get current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // Disable canonical mode and echo
    newt.c_lflag &= ~(ICANON | ECHO);

    // Apply new settings
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Press s to check state or any key to continue...\n");

    ch = getchar();  // Read one character immediately
	if(ch == 's' || ch == 'S')
		print_state(handle);

    // Restore original settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int main(int argc, char *argv[]) {
	cpu_t *cpu;
	FILE *fd;
	size_t program_size = 0;
	uint32_t *program;
	uint32_t temp;
	int i;
	char *filename;
	int num_of_row_to_execute;	// Used to limit the number of the code to execute


	if(argc < 3){
		fprintf(stderr, "Wrong usage: %s <filename> <num_of_row_to_execute>", argv[1]);
		exit(-1);
	}
	filename = argv[1];
	num_of_row_to_execute = atoi(argv[2]);

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
	
	// If num of row to execute is -1 (DEFAULT) or higher than the maximum number of row
	// the program is executed completely
	if(num_of_row_to_execute == -1 || num_of_row_to_execute > (int)program_size)
		num_of_row_to_execute = (int)program_size;


	for(i = 0; i < num_of_row_to_execute; i++) {
		printf("I%-2d: %#010x      %s\n", i, program[i], identify_instruction(program[i]));
		cpu_load_instr(cpu, i, program[i]);
	
	}
	printf("############################################\n");
	press_and_continue(cpu);


	for(i = 0; i < 4+num_of_row_to_execute; i++){
		printf("\n\n\n######## STEP %3d ########\n", i);
		cpu_step(cpu);
		press_and_continue(cpu);
	}

	printf("############################################\n");
	printf("################### STATE ##################\n");
	print_state(cpu);
	free(program);
	memory_destroy(cpu);

	return 0;
}
