#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_TX_OFFSET		0x0
#define UART_RX_OFFSET		0x4
#define UART_STATUS_OFFSET	0x8

typedef struct {
	int server_fd;	// Listening socket
	int client_fd;	// Active connection
	uint16_t port;
} uart_t;

void uart_init(uart_t *uart, uint16_t port);
void uart_accept(uart_t *uart);
void uart_write(uart_t *uart, uint8_t byte);
uint8_t uart_read(uart_t *uart);
uint32_t uart_status(uart_t *uart);
void uart_free(uart_t *uart);


#endif //UART_H
