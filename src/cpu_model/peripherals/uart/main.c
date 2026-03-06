#include <cpu_model/peripherals/uart/uart.h>
#include <stdio.h>

int main() {
	uart_t uart;
	uart_init(&uart, 5555);
	uart_accept(&uart);

	const char *msg = "Hello World UART\n\n";
	for (int i = 0; msg[i] != '\0'; i++) {
		uart_write(&uart, (uint8_t)msg[i]);
	}

	uart_free(&uart);
	return 0;
}
