#include <cpu_model/peripherals/memory/memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int mem_init(memory_t *mem, uint32_t size, uint32_t base_addr){
	mem->size = size;
	mem->base_addr = base_addr;
	mem->data = (uint32_t *)malloc(size*sizeof(uint32_t));
	if(mem->data == NULL){
		fprintf(stderr, "[MEMORY] malloc() failed\n");
		return -1;
	}

	memset(mem->data, 0, size*sizeof(uint32_t));
	return 0;
}

int mem_read(memory_t *mem, uint32_t addr, uint32_t *out){
	uint32_t idx = (addr - mem->base_addr);
	if(idx >= mem->size){
		fprintf(stderr, "[MEMORY] Reading from memory with wrong address: 0x%08x\n", addr);
		return -1;
	}

	*out = mem->data[idx];
	return 0;
}

int mem_write(memory_t *mem, uint32_t addr, uint32_t val){
	uint32_t idx = (addr - mem->base_addr);
	if(idx >= mem->size){
		fprintf(stderr, "[MEMORY] Writing from memory with wrong address: 0x%08x\n", addr);
		return -1;
	}
	
	mem->data[idx] = val;
	return 0;
}

void mem_free(memory_t *mem){
	free(mem->data);
	return;
}
