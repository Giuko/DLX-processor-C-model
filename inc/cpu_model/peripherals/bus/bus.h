#ifndef BUS_H
#define BUS_H

#include <cpu_model/peripherals/memory/memory.h>

#define IRAM_BASE	0x00000000
#define DRAM_BASE	0x10000000
#define	UART1_BASE	0x20000000

typedef struct {
	memory_t iram;
	memory_t dram;
} bus_t;

int bus_read(bus_t *bus, uint32_t addr, uint32_t *out);
int bus_write(bus_t *bus, uint32_t addr, uint32_t val);
int bus_init(bus_t *bus);
int bus_reset(bus_t *bus);
void bus_free(bus_t *bus);
#endif //BUS_H
