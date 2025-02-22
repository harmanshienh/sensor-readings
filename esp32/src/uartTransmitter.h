#ifndef UART_TRANSMITTER_H
#define UART_TRANSMITTER_H

#include <driver/uart.h>

#define UART_TAG "UART_TX"
extern uart_port_t uart_num;
extern size_t uart_buffer_size;
extern float temp;
extern float humidity;

esp_err_t uart_transmitter_init(void);
void uart_transmit_data(void);

#endif