#include <stdio.h>
#include <esp_log.h>
#include "uartTransmitter.h"
#include "bme280Driver.h"

uart_port_t uart_num = UART_NUM_2;
size_t uart_buffer_size = (1024 * 2);

esp_err_t uart_transmitter_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    //Configure UART parameters
    esp_err_t err = uart_param_config(uart_num, &uart_config);
    if (err != ESP_OK) {
        return err;
    }

    //Configurations for transmitter (ESP)
    err = uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        return err;
    }

    return uart_driver_install(
        uart_num,
        uart_buffer_size,
        uart_buffer_size,
        10,
        NULL,
        0
    );
}

void uart_transmit_data(void) {
    uart_write_bytes(uart_num, (const char*)&temp, sizeof(temp));
    uart_write_bytes(uart_num, (const char*)&humidity, sizeof(humidity));
}