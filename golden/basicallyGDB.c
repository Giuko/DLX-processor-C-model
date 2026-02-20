#include "cpu_model/cpu_model.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// ANSI escape helpers
#define CLEAR_SCREEN()       printf("\033[2J\033[H")				// Clear terminal
#define MOVE_CURSOR(r, c)    printf("\033[%d;%dH", (r), (c))		// To write in a easy way in '2 different screens'
#define COLOR_RESET          "\033[0m"								// Reset color of terminal
#define COLOR_HIGHLIGHT      "\033[1;32m"							// Highlight current row
#define COLOR_DIM            "\033[2m"
#define COLOR_BOLD           "\033[1m"

// Define screens size
#define RIGHT_COL    55
#define TOP_ROW       1
#define MAX_STEP_LINES 32
#define MAX_LINE_LEN  128

// Some useful constants to manage button pressed
#define OK 0
#define RESTART 1
#define QUIT 2

// Used to print the program to execute in the model of the CPU
static uint32_t *g_program      = NULL;
static int       g_program_size = 0;

// Captured output from cpu_step()
static char g_step_output[MAX_STEP_LINES][MAX_LINE_LEN];
static int  g_step_line_count = 0;

// Capture stdout into g_step_output by redirecting the fd
void capture_cpu_step(void *handle) {
    int saved_stdout;
    int pipefd[2];
    char buf[MAX_STEP_LINES * MAX_LINE_LEN];
    ssize_t n;

    g_step_line_count = 0;
    memset(g_step_output, 0, sizeof(g_step_output));

    // Create pipe and redirect stdout into it
    if (pipe(pipefd) != 0) {
        cpu_step(handle);   // fallback: just run normally
        return;
    }

	fflush(stdout);
    saved_stdout = dup(STDOUT_FILENO);	// Duplicate STDOUT
    dup2(pipefd[1], STDOUT_FILENO);		
    close(pipefd[1]);

    cpu_step(handle);
	fflush(stdout);

    // Restore stdout
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);

    // Read captured output
    n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);
    if (n <= 0) return;
    buf[n] = '\0';

    // Split into lines
    char *line = strtok(buf, "\n");
    while (line != NULL && g_step_line_count < MAX_STEP_LINES) {
        strncpy(g_step_output[g_step_line_count], line, MAX_LINE_LEN - 1);
        g_step_line_count++;
        line = strtok(NULL, "\n");
    }
}

void draw_program_panel(void *handle) {
	cpu_t    *cpu = (cpu_t *)handle;
	uint32_t  currentPC = cpu_get_pc(cpu);
	int i;

	// Vertical separator
	for (i = 0; i < 40; i++) {
		MOVE_CURSOR(TOP_ROW + i, RIGHT_COL - 2);
		printf(COLOR_DIM "|" COLOR_RESET);
	}

	// Header
    MOVE_CURSOR(TOP_ROW, RIGHT_COL);
    printf(COLOR_BOLD "  ADDR      OPCODE      DISASM" COLOR_RESET);
    MOVE_CURSOR(TOP_ROW + 1, RIGHT_COL);
    printf(COLOR_DIM "  --------------------------------" COLOR_RESET);

	for (i = 0; i < g_program_size; i++) {
		MOVE_CURSOR(TOP_ROW + 2 + i, RIGHT_COL);
		uint32_t byte_addr = (uint32_t)i * 4;
		if ((uint32_t)i == currentPC)
			printf(COLOR_HIGHLIGHT "--> 0x%04x  %#010x  %-20s" COLOR_RESET, byte_addr, g_program[i], identify_instruction(g_program[i]));
		else
			printf("    0x%04x  %#010x  %-20s", byte_addr, g_program[i], identify_instruction(g_program[i]));
	}
}

void draw_left_panel(int step) {
	int i;

	MOVE_CURSOR(1, 1);
	printf(COLOR_BOLD "=== STEP %d ===" COLOR_RESET "                        ", step);

	MOVE_CURSOR(3, 1);
	printf(COLOR_BOLD "Pipeline activity:" COLOR_RESET "                      ");

	for (i = 0; i < g_step_line_count; i++) {
		MOVE_CURSOR(4 + i, 1);
		printf("%-50s", g_step_output[i]);   // pad to avoid leftover chars
	}

	// Clear any leftover lines from a previous step that had more output
	for (i = g_step_line_count; i < MAX_STEP_LINES; i++) {
		MOVE_CURSOR(4 + i, 1);
		printf("%-50s", "");
	}

	MOVE_CURSOR(38, 1);
	printf("Press [s] state [r] restart [q] quit  [any] next step   ");
}

void print_state(void *handle) {
    cpu_t    *cpu = (cpu_t *)handle;
    uint32_t  currentPC = cpu_get_pc(cpu);
    uint32_t  regs[32];
    int i;

    for (i = 0; i < 32; i++)
        regs[i] = cpu_get_reg(cpu, i);

    MOVE_CURSOR(1, 1);
    printf(COLOR_BOLD "=== CPU STATE ===" COLOR_RESET "                    ");
    MOVE_CURSOR(2, 1);
    printf("PC: 0x%08x (byte: 0x%08x)  ", currentPC, currentPC * 4);
    MOVE_CURSOR(3, 1);
    printf("                                   ");
    for (i = 0; i < 32; i++) {
		MOVE_CURSOR(4 + i, 1);
		printf("R%-2d: 0x%08x    ", i, regs[i]);
    }
}


int press_and_continue(void *handle, int step) {
    struct termios oldt, newt;
    int ch;

    CLEAR_SCREEN();
    draw_program_panel(handle);
    draw_left_panel(step);
    fflush(stdout);

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (ch == 's' || ch == 'S') {
        CLEAR_SCREEN();          // wipe everything
        draw_program_panel(handle);  // redraw right side
        print_state(handle);         // draw state on left
        MOVE_CURSOR(38, 1);
        printf("Press any key to continue...");
        fflush(stdout);
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
    if (ch == 'q')
    	CLEAR_SCREEN();

	if (ch == 'r')
		return RESTART;

	if(ch == 'q')
		return QUIT;
	
	return OK;
}

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
