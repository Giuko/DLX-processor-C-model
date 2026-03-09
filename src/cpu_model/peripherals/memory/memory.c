#include <cpu_model/peripherals/memory/memory.h>
#include <cpu_model/peripherals/bus/bus.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mem_init(memory_t *mem, uint32_t size, uint32_t base_addr){
	mem->size = size;
	mem->base_addr = base_addr;
	mem->data = (uint8_t *)malloc(size*4*sizeof(uint8_t));
	if(mem->data == NULL){
		fprintf(stderr, "[MEMORY] malloc() failed\n");
		return -1;
	}

	memset(mem->data, 0, size*sizeof(uint32_t));
	return 0;
}

int mem_read(memory_t *mem, uint32_t addr, uint32_t *out){
	uint32_t idx = (addr - mem->base_addr);
	uint32_t val;
	int i;
	if(idx >= mem->size){
		fprintf(stderr, "[MEMORY] Reading from memory with wrong address: 0x%08x\n", addr);
		return -1;
	}
	val = 0;
	// Mem data is a 8-bit value, extact always a word from the memory
	for(i = 0; i < 4; i++)
		val = (val << 8) + mem->data[idx*4+i];
	*out = val;
	return 0;
}

int mem_write(memory_t *mem, uint32_t addr, uint32_t val){
	uint32_t idx = (addr - mem->base_addr);
	int i;
	if(idx >= mem->size){
		fprintf(stderr, "[MEMORY] Writing from memory with wrong address: 0x%08x\n", addr);
		return -1;
	}
	
	for(i = 0; i < 4; i++)
		mem->data[idx*4+i] = (val>>(8*(3-i))) & 0xff;
	
	return 0;
}

void mem_free(memory_t *mem){
	free(mem->data);
	return;
}
