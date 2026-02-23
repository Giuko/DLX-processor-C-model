#include "cpu_model/cpu_model.h"
#include "inc/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

extern uint32_t *g_program;
extern int g_program_size;



int main(int argc, char *argv[]) {
    cpu_t    *cpu;
    FILE     *fd;
    size_t    program_size = 0;
    uint32_t *program;
    uint32_t  temp;
    int       i;
    char     *filename;
    int       num_of_row_to_execute;
	int		  ch_pressed;

    if (argc < 3) {
        fprintf(stderr, "Wrong usage: %s <filename> <num_of_row_to_execute>\n", argv[0]);
        exit(-1);
    }

    filename              = argv[1];
    num_of_row_to_execute = atoi(argv[2]);

    fd = fopen(filename, "r");
    if (fd == NULL) {
        fprintf(stderr, "fopen() failed | filename: %s\n", filename);
        exit(-2);
    }

    while (fscanf(fd, "%x", &temp) == 1)
        program_size++;
    rewind(fd);

    program = (uint32_t *)malloc(sizeof(uint32_t) * program_size);
    i = 0;
    while (i < (int)program_size && fscanf(fd, "%x", &program[i]) == 1)
        i++;
    fclose(fd);

    if (num_of_row_to_execute == -1 || num_of_row_to_execute > (int)program_size)
        num_of_row_to_execute = (int)program_size;

    g_program      = program;
    g_program_size = num_of_row_to_execute;

    cpu = (cpu_t *)cpu_create();
    if (cpu == NULL) {
        fprintf(stderr, "cpu_create() failed\n");
        free(program);
        exit(-3);
    }
    cpu_reset(cpu);

    for (i = 0; i < num_of_row_to_execute; i++)
        cpu_load_instr(cpu, i, program[i]);

    // Step 0: initial state before any execution
    ch_pressed = press_and_continue(cpu, 0);
	if (ch_pressed != QUIT) {
		bool flag = true;
		for (i = 1; flag; i++) {
			capture_cpu_step(cpu);          // run cpu_step() and grab its stdout
			ch_pressed = press_and_continue(cpu, i);
			if(ch_pressed == RESTART)
				cpu_reset(cpu);
			else if(ch_pressed == QUIT)
				flag = false;
		}
	}
	free(program);
	memory_destroy(cpu);
	return 0;
}
