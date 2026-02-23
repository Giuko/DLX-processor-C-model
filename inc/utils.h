#ifndef UTILS_H
#define UTILS_H

// ANSI escape helpers
#define CLEAR_SCREEN()       printf("\033[2J\033[H")				// Clear terminal
#define MOVE_CURSOR(r, c)    printf("\033[%d;%dH", (r), (c))		// To write in a easy way in '2 different screens'
#define COLOR_RESET          "\033[0m"								// Reset color of terminal
#define COLOR_HIGHLIGHT      "\033[1;32m"							// Highlight current row
#define COLOR_DIM            "\033[2m"
#define COLOR_BOLD           "\033[1m"

// Define screens size
#define TOP_ROW       1
#define MAX_STEP_LINES 32
#define MAX_LINE_LEN  128
#define RIGHT_COL       45    // program panel
#define REG_COL         120   // registers panel
#define PANEL_HEIGHT    35
#define TOP_ROW          1
#define PANEL_HEIGHT 35  // max visible lines in the program panel

// Some useful constants to manage button pressed
#define OK 0
#define RESTART 1
#define QUIT 2

void capture_cpu_step(void *handle);
void draw_program_panel(void *handle); 
void draw_left_panel(int step);
void print_state(void *handle);
void draw_registers(void *handle);
int press_and_continue(void *handle, int step);
#endif
