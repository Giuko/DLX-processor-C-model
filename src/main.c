#include "cpu_model/cpu_model.h"
#include "cpu_model/peripherals/bus/bus.h"
#include "extra/utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

// Used by draw_program_panel() in utils.c
extern uint32_t *g_program;
extern int       g_program_size;

int main(int argc, char *argv[]) {
    cpu_t *cpu;
    FILE  *fd;
    char  *filename;
    int    num_of_row_to_execute;
    int    ch_pressed;

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

    cpu = (cpu_t *)cpu_create();
    if (cpu == NULL) {
        fprintf(stderr, "cpu_create() failed\n");
        fclose(fd);
        exit(-3);
    }
    cpu_reset(cpu);

    // Load @TEXT → IRAM and @RODATA → RODATA memory via cpu_load_program()
    int text_count = cpu_load_program(cpu, fd);
    fclose(fd);

    if (text_count <= 0) {
        fprintf(stderr, "cpu_load_program() failed or empty program\n");
        free(cpu);
        exit(-4);
    }

    // Build g_program for the visual panel (TEXT instructions only)
    // Re-open and re-read only the @TEXT words
    fd = fopen(filename, "r");
    if (fd == NULL) {
        fprintf(stderr, "fopen() failed on second open\n");
        free(cpu);
        exit(-5);
    }

    g_program = (uint32_t *)malloc(sizeof(uint32_t) * (size_t)text_count);
    if (g_program == NULL) {
        fprintf(stderr, "malloc() failed\n");
        fclose(fd); free(cpu); exit(-6);
    }

    {
        typedef enum { SEC_NONE, SEC_TEXT, SEC_RODATA } Section;
        Section  sec = SEC_NONE;
        char     line[64];
        int      idx = 0;
        while (fgets(line, sizeof(line), fd) && idx < text_count) {
            line[strcspn(line, "\r\n")] = 0;
            if (strcmp(line, "@TEXT")   == 0) { 
				sec = SEC_TEXT;   
				continue; 
			}
            if (strcmp(line, "@RODATA") == 0) { 
				sec = SEC_RODATA; 
				continue; 
			}
            if (line[0] == '\0') continue;
            if (sec == SEC_TEXT) {
                uint32_t word;
                if (sscanf(line, "%x", &word) == 1)
                    g_program[idx++] = word;
            }
        }
    }
    fclose(fd);

    if (num_of_row_to_execute == -1 || num_of_row_to_execute > text_count)
        num_of_row_to_execute = text_count;
    g_program_size = num_of_row_to_execute;

    // Step 0: initial state before any execution
    ch_pressed = press_and_continue(cpu, 0);
    if (ch_pressed != QUIT) {
        bool flag = true;
        for (int i = 1; flag; i++) {
            capture_cpu_step(cpu);
            ch_pressed = press_and_continue(cpu, i);
            if (ch_pressed == RESTART) {
                cpu_reset(cpu);
			} else if (ch_pressed == QUIT) {
                flag = false;
			}else if (ch_pressed == CONTINUE) {
				// Executing each instruction
				for(i = 0; i < IRAM_SIZE; i++){
            		cpu_step(cpu);
				}
				capture_cpu_step(cpu);
			}
        }
    }

    free(g_program);
    free(cpu);
    return 0;
}
