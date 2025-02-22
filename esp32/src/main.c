#include <stdio.h>
#include <esp_log.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "bme280Driver.h"
#include "uartTransmitter.h"

float temp = 0.0f;
float humidity = 0.0f;

void app_main() {
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(BME_TAG, "I2C initialized successfully!");

    ESP_ERROR_CHECK(bme280_init());
    ESP_LOGI(BME_TAG, "BME280 initialized successfully!");

    ESP_ERROR_CHECK(uart_transmitter_init());
    ESP_LOGI(UART_TAG, "UART initialized successfully!");

    while (1) {
        temp = bme280_read_temperature();
        humidity = bme280_read_humidity();
        ESP_LOGI(BME_TAG, "Temperature: %.2f°C, Humidity: %.2f%%", temp, humidity);
        uart_transmit_data();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}