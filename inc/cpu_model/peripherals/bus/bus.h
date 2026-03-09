#ifndef BUS_H
#define BUS_H

#include <cpu_model/peripherals/memory/memory.h>
#include <cpu_model/peripherals/uart/uart.h>

// Heap ram, generic
#define DRAM_BASE	0x00000000
#define DRAM_SIZE	0x00001000
#define STACK_BASE 	(DRAM_BASE+DRAM_SIZE-1)

// Read only data 
#define RODATA_BASE	0x00001000
#define RODATA_SIZE	0x00001000

// Instruction Memory
#define IRAM_BASE	0x20000000
#define IRAM_SIZE	0x00001000

//////////////////////////////////
// Memory mapped peripherals
//////////////////////////////////
// UART1 address
#define	UART1_BASE	0x00100000

#define UART1_TX		(UART1_BASE + UART_TX_OFFSET)
#define UART1_RX		(UART1_BASE + UART_RX_OFFSET)
#define UART1_STATUS	(UART1_BASE + UART_STATUS_OFFSET)

typedef struct {
	memory_t	iram;
	memory_t	dram;
	memory_t	rodata;
	uart_t		*uart1;
} bus_t;

int bus_read(bus_t *bus, uint32_t addr, uint32_t *out);
int bus_write(bus_t *bus, uint32_t addr, uint32_t val);
int bus_init(bus_t *bus);
int bus_reset(bus_t *bus);
void bus_free(bus_t *bus);
#endif //BUS_H
