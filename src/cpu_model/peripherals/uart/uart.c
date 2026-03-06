#include "cpu_model/cpu_model.h"
#include <cpu_model/peripherals/uart/uart.h>

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

void uart_init(uart_t *uart, uint16_t port){
	uart->port 		= port;
	uart->client_fd = -1;

	uart->server_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	int opt = 1;
	setsockopt(uart->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr = {
		.sin_family 		= AF_INET,
		.sin_addr.s_addr 	= INADDR_ANY,
		.sin_port 			= htons(port),
	};

	if(bind(uart->server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
		perror("bind");
		exit(-1);
	}
	listen(uart->server_fd, 1);

	printf("[UART] Listening in port %d — connect with: nc localhost %d\n", port, port);
}

void uart_accept(uart_t *uart){
	printf("[UART] Waiting connection...\n");
	uart->client_fd = accept(uart->server_fd, NULL, NULL);
	printf("[UART] Client connected!\n");
}

void uart_write(uart_t *uart, uint8_t byte){
	if(uart->client_fd < 0){
		fprintf(stderr, "[UART] client not set correctly");
		return;
	}
	char s[64];
	sprintf(s, "[UART] Writing to UART: %c\n", byte);
	print_debug(s);
	send(uart->client_fd, &byte, 1, 0);
}

uint8_t uart_read(uart_t *uart){
	if(uart->client_fd < 0)
		return 0;

	uint8_t byte = 0;
	int n = recv(uart->client_fd, &byte, 1, MSG_DONTWAIT);
	return (n==1) ? byte : 0;
}

uint32_t uart_status(uart_t *uart){
	if(uart->client_fd < 0) 
		return 0;

	uint8_t tmp;
	int n = recv(uart->client_fd, &tmp, 1, MSG_PEEK | MSG_DONTWAIT);
	uint32_t rx_ready = (n == 1) ? 0x2 : 0x0;

	return 0x1 | rx_ready;		// bit[0]: TX ready, bit[1]: RX ready
}

void uart_free(uart_t *uart){
	if (uart->client_fd >= 0) 
		close(uart->client_fd);
	close(uart->server_fd);
}
