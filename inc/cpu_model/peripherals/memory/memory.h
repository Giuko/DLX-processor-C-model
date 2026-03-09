#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

typedef struct {
	uint8_t *data;
	uint32_t size;			// In words
	uint32_t base_addr;		// To be used for memory map
} memory_t;

typedef struct {
	memory_t iram;
	memory_t dram;
} mem_system_t;

// Initialize memory
// Returns 0 when OK
int mem_init(memory_t *mem, uint32_t size, uint32_t base_addr);

// Reads from memory
// Returns 0 when OK
int mem_read(memory_t *mem, uint32_t addr, uint32_t *out);

// Writes to memory
// Returns 0 when OK
int mem_write(memory_t *mem, uint32_t addr, uint32_t val);

// Free memory
void mem_free(memory_t *mem);

#endif //MEMORY_H
