#include <cpu_model/peripherals/memory/memory.h>
#include <cpu_model/peripherals/bus/bus.h>
#include <cpu_model/peripherals/uart/uart.h>
#include <cpu_model/cpu_model.h>
#include <stdio.h>

int bus_write(bus_t *bus, uint32_t addr, uint32_t val){
	char s[128];
	sprintf(s, "[BUS] Writing to the bus at addr: 0x%08x\n", addr);
	print_debug(s);
	/////////////////////////////////
	// Memories
	/////////////////////////////////
	if (addr >= DRAM_BASE && addr < DRAM_BASE+DRAM_SIZE)
		return mem_write(&bus->dram, addr, val);
	
	if (addr >= IRAM_BASE && addr < IRAM_BASE+IRAM_SIZE)
		return mem_write(&bus->iram, addr, val);

	// Read only data, no writing
	if (addr >= RODATA_BASE && addr < RODATA_BASE+RODATA_SIZE)
		return -1;

	/////////////////////////////////
	// UART1
	/////////////////////////////////
#ifdef USING_UART1
	if(addr == UART1_TX){
		print_debug("[BUS] Writing to UART1\n");
		uart_write(bus->uart1, (uint8_t)val);
		return 0;
	}
#endif
	
	fprintf(stderr, "[BUS] Writing to an address not mapped: 0x%08x\n", addr);
	return -1;
}

int bus_read(bus_t *bus, uint32_t addr, uint32_t *out){
	
	/////////////////////////////////
	// Memories
	/////////////////////////////////
	if (addr >= DRAM_BASE && addr < DRAM_BASE+DRAM_SIZE)
		return mem_read(&bus->dram, addr, out);
	
	if (addr >= IRAM_BASE && addr < IRAM_BASE+IRAM_SIZE)
		return mem_read(&bus->iram, addr, out);

	if (addr >= RODATA_BASE && addr < RODATA_BASE+RODATA_SIZE)
		return mem_read(&bus->rodata, addr, out);
	/////////////////////////////////
	// UART1
	/////////////////////////////////
#ifdef USING_UART1
	if (addr == UART1_RX){
		*out = uart_read(bus->uart1);
		return 0;
	}
	if (addr == UART1_STATUS){
		*out = uart_status(bus->uart1);
		return 0;
	}
#endif
	fprintf(stderr, "[BUS] Reading to an address not mapped: 0x%08x\n", addr);
	return -1;
}

int bus_init(bus_t *bus){
	if(mem_init(&bus->iram, IRAM_SIZE, IRAM_BASE))
		return -1;
	if(mem_init(&bus->dram, DRAM_SIZE, DRAM_BASE))
		return -1;

	if(mem_init(&bus->rodata, RODATA_SIZE, RODATA_BASE))
		return -1;

#ifdef USING_UART1
	bus->uart1 = (uart_t*)malloc(sizeof(uart_t));
	if(bus->uart1 == NULL){
		fprintf(stderr, "[BUS] malloc() failed to initilize uart\n");
		return -1;
	}
	uart_init(bus->uart1, 5555);
	uart_accept(bus->uart1);
#endif
	return 0;
}

int bus_reset(bus_t *bus){

	mem_free(&bus->dram);
	if(mem_init(&bus->dram, DRAM_SIZE, DRAM_BASE))
		return -1;
	return 0;
}

void bus_free(bus_t *bus){
	mem_free(&bus->iram);
	mem_free(&bus->dram);
	mem_free(&bus->rodata);
#ifdef USING_UART1
	uart_free(bus->uart1);
#endif
}
