#include <stdio.h>
#include <esp_log.h>
#include "bme280Driver.h"

bme280_calib_data_t calib;
int32_t t_fine;

esp_err_t i2c_master_init(void) {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t esp_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_CLOCK_FREQUENCY
    };

    //Initialize I2C 0 with the above configuration
    esp_err_t err = i2c_param_config(i2c_master_port, &esp_conf);
    if (err != ESP_OK) {
        return err;
    }

    //Install the I2C driver and return ESP_OK or ESP_FAIL
    return i2c_driver_install(
        i2c_master_port, 
        esp_conf.mode, 
        I2C_MASTER_RX_BUF_DISABLE, 
        I2C_MASTER_TX_BUF_DISABLE, 
        0
    );
}

esp_err_t bme280_init(void) {
    uint8_t chip_id;
    esp_err_t ret = bme280_read_reg(BME280_REG_ID, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(BME_TAG, "Failed to read Chip ID");
        return ret;
    }
    if (chip_id != 0x60) {
        ESP_LOGE(BME_TAG, "Unexpected Chip ID: 0x%02x", chip_id);
        return ESP_ERR_INVALID_RESPONSE;
    }

    ret = bme280_write_reg(BME280_REG_CTRL_HUM, 0x01);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = bme280_write_reg(BME280_REG_CTRL_MEAS, 0x27);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = bme280_write_reg(BME280_REG_CONFIG, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

esp_err_t bme280_write_reg(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2] = {reg_addr, data};

    //Create an I2C command buffer
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    //Initialize communication
    i2c_master_start(cmd);

    //Address the BME280 sensor, set to WRITE mode
    //Shift address left 1 bit to make room for R/W bit
    i2c_master_write_byte(cmd, (BME_SENSOR_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);

    //Send data over to address within BME280 specified in write_buf
    //Address frame comes first
    i2c_master_write(cmd, write_buf, sizeof(write_buf), ACK_CHECK_EN);

    //Stop condition, halts communication
    i2c_master_stop(cmd);

    //Execute queued R/W instructions from above then delete buffer
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

esp_err_t bme280_read_reg(uint8_t reg_addr, uint8_t *data, size_t len) {
    //Create an I2C command buffer
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    //Initialize communication
    i2c_master_start(cmd);

    //Address the BME280 sensor, set to WRITE mode to allow reading from specific registers
    //Shift address left 1 bit to make room for R/W bit
    //Want to send address of register within BME280
    i2c_master_write_byte(cmd, (BME_SENSOR_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);

    //Restart communication, but read instead of write
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME_SENSOR_ADDRESS << 1) | READ_BIT, ACK_CHECK_EN);

    //Read all bits up to the last bit 
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, ACK_VAL);
    }

    //Read last byte and send a NACK, indicate communication is done
    i2c_master_read_byte(cmd, data + len - 1, NACK_VAL);
    i2c_master_stop(cmd);

    //Execute queued R/W instructions from above then delete buffer
    int ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

void bme280_calibrate_data(void) {
    //Read calibration values from specified register
    uint8_t tempData[6];
    uint8_t humData[8];
    bme280_read_reg(BME280_TEMP_CALIBRATION, tempData, 6);
    bme280_read_reg(BME280_HUM_CALIBRATION_1, humData, 1);
    bme280_read_reg(BME280_HUM_CALIBRATION_2, &humData[1], 7);

    //Temperature data
    calib.dig_T1 = (tempData[1] << 8) | tempData[0];
    calib.dig_T2 = (tempData[3] << 8) | tempData[2];
    calib.dig_T3 = (tempData[5] << 8) | tempData[4];

    //Humidity data
    calib.dig_H1 = humData[0];
    calib.dig_H2 = (int16_t)(humData[2] << 8) | humData[1];
    calib.dig_H3 = humData[3];
    calib.dig_H4 = (int16_t)(humData[4] << 4) | (humData[5] & 0x0F);
    calib.dig_H5 = (int16_t)(humData[6] << 4) | ((humData[5] >> 4) & 0x0F);
    calib.dig_H6 = (int8_t)humData[7]; 
}

float bme280_compensate_temperature(uint32_t adc_t) {
    bme280_calibrate_data();
    int32_t linearCorrection, nonLinearCorrection, temperature;

    int32_t offset = (int32_t)calib.dig_T1;
    int32_t linearScale = (int32_t)calib.dig_T2;
    int32_t nonLinearScale = (int32_t)calib.dig_T3;

    linearCorrection = (((adc_t >> 3) - (offset << 1))) * linearScale >> 11;
    nonLinearCorrection = ((adc_t >> 4) - offset) * ((adc_t >> 4) - offset) >> 12 * nonLinearScale >> 14;
    
    t_fine = linearCorrection + nonLinearCorrection;

    temperature = (t_fine * 5 + 128) >> 8; 

    return temperature / 100.0f;
}

float bme280_compensate_humidity(int32_t adc_h) {
    int32_t v_x1_u32r;

    v_x1_u32r = (t_fine - ((int32_t)76800));

    v_x1_u32r = (((adc_h << 14) - (((int32_t)calib.dig_H4) << 20) - 
                 (((int32_t)calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15;

    v_x1_u32r *= (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) * 
                     (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) + 
                     ((int32_t)32768))) >> 10) + 
                     ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14);

    v_x1_u32r -= (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                   ((int32_t)calib.dig_H1)) >> 4);

    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;

    uint32_t humidity = (uint32_t)(v_x1_u32r >> 12);

    return ((float)humidity/1024.0);
}

float bme280_read_temperature(void) {
    uint8_t data[3];
    if (bme280_read_reg(BME280_REG_TEMP_MSB, data, 3) != ESP_OK) {
        return 0.0f;
    }

    uint32_t adc_t = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
    return bme280_compensate_temperature(adc_t);
}

float bme280_read_humidity(void) {
    uint8_t data[2];
    if (bme280_read_reg(BME280_REG_HUM_MSB, data, 2) != ESP_OK) {
        return 0.0f;
    }

    int32_t adc_h = ((uint32_t)data[0] << 8 | (uint32_t)data[1]);
    return bme280_compensate_humidity(adc_h);
}