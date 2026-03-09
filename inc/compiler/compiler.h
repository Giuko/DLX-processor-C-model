#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>
#include <stdio.h>
 
//////////////////////////////
// Section
//////////////////////////////

// Returns the section we're going to read
// Possible outputs: SEC_TEXT, SEC_RODATA
typedef enum { 
	SEC_NONE, 
	SEC_TEXT, 
	SEC_RODATA 
} Section;
Section get_section_marker(const char *p);

///////////////////////////////////////
// Segment
///////////////////////////////////////

// Add rodata
int rodata_append(const uint8_t *bytes, int len);

// Parse a .rodata data line:
//   label db "string" [, byte ...]   — string with optional trailing bytes
//   label db 'c'                     — single character
//   label db 42                      — single numeric byte
//   label dw 0x1234                  — 32-bit word, little-endian
//
// Returns 1 if line contained a data directive, 0 otherwise.
int parse_rodata_line(const char *line);

///////////////////////////
// Constants
///////////////////////////
typedef struct { 
	char name[64]; 
	int value; 
} Constant;

// Store constant in the table
void constant_define(const char *name, int value);

// Get constant value
int constant_find(const char *name, int *out);

// Get constant value
int constant_find(const char *name, int *out);

// Returns true if .define is found
int is_define(const char *p);

// Save the value of the constant defined in the constatn table
void parse_define(const char *line);


///////////////////////////
// Label
///////////////////////////

typedef struct { 
	char name[64]; 
	int address; 
	Section section; 
} Label;

// Save label to label table
void label_add(const char *name, int address, Section sec);

// Get label address
int find_label_address(const char *name);

//////////////////////////////
// Register Aliasing
//////////////////////////////
typedef struct { 
	char name[32]; 
	int reg_num; 
} RegAlias;

// Lookup a register alias (case-insensitive)
// Returns register number or -1.
int reg_alias_find(const char *name);

// Register an alias defined by .reg directive.
void reg_alias_define(const char *name, int reg_num);

// .reg directive parser
// Syntax:  .reg  alias_name  rN
//          .reg  alias_name  N      (bare number also accepted)
int is_reg_directive(const char *p);
 
// Parse .red directive
void parse_reg_directive(const char *line);

// parse_register (replaces the old one)
// Accepts:  rN  RN  alias  ALIAS  (case-insensitive for aliases)
int parse_register(const char *token);



/////////////////////////////////////////////////////////////
// Expression evaluator  (left-to-right, operators: + - * /)
// General parser
/////////////////////////////////////////////////////////////

//
int eval_atom(const char *s, int *out);

// Resolve expression
int eval_expr(const char *expr, int *out);

// Obtain immediate value
// #<decimal>
// 0x<hexadeximal>
int parse_imm(const char *token);

// Helper function to put the string in lowercase
void str_to_lower(char *s);


///////////////////////
// Instruction table
///////////////////////
typedef struct { 
	const char *name; 
	uint8_t opcode; 
	uint8_t func; 
} InstructionMap;

// Extract opcode
int parse_opcode(const char *line, uint8_t *op, uint8_t *fn);


///////////////////////
// Code refactoring
//////////////////////

#define OPCODE_PUSH 0xFC
#define OPCODE_POP 	0xFD
#define OPCODE_DEC 	0xFE
#define OPCODE_INC 	0xFF

int code_refactoring(char *s, FILE *fd);
#endif //COMPILER_H
