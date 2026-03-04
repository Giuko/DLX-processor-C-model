#include "cpu_model/peripherals/memory/memory.h"
#include <cpu_model/peripherals/bus/bus.h>
#include <cpu_model/cpu_model.h>
#include <stdio.h>

int bus_write(bus_t *bus, uint32_t addr, uint32_t val){
	

	if(addr == UART1_BASE)	// TODO correct it
		return mem_write(&bus->iram, addr, val);
	
	if (addr >= DRAM_BASE && addr < UART1_BASE)
		return mem_write(&bus->dram, addr, val);
	
	if (addr >= IRAM_BASE && addr < DRAM_BASE)
		return mem_write(&bus->iram, addr, val);
	

	fprintf(stderr, "[BUS] Writing to an address not mapped: 0x%08x\n", addr);
	return -1;
}

int bus_read(bus_t *bus, uint32_t addr, uint32_t *out){
	
	if (addr >= DRAM_BASE && addr < UART1_BASE)
		return mem_read(&bus->dram, addr, out);
	
	if (addr >= IRAM_BASE && addr < DRAM_BASE)
		return mem_read(&bus->iram, addr, out);

	// TODO: implement UART 

	fprintf(stderr, "[BUS] Reading to an address not mapped: 0x%08x\n", addr);
	return -1;
}

int bus_init(bus_t *bus){
	if(mem_init(&bus->iram, IRAM_DEPTH, IRAM_BASE))
		return -1;
	if(mem_init(&bus->dram, DRAM_DEPTH, DRAM_BASE))
		return -1;
	return 0;
}

int bus_reset(bus_t *bus){

	mem_free(&bus->dram);
	if(mem_init(&bus->dram, DRAM_DEPTH, DRAM_BASE))
		return -1;
	return 0;
}

void bus_free(bus_t *bus){
	mem_free(&bus->iram);
	mem_free(&bus->dram);
}
