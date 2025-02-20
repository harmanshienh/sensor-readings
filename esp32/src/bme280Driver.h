#include <driver/i2c.h>

#define I2C_MASTER_SCL 22 //GPIO 22
#define I2C_MASTER_SDA 21 //GPIO 21
#define I2C_MASTER_NUM 0 //I2C 0
#define I2C_MASTER_CLOCK_FREQUENCY 400000 //Clock speed of 400kHz
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0

#define BME_SENSOR_ADDRESS 0x76

#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ

#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DISABLE 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1

//BME280 registers
//Control
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5

//Pressure
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_PRESS_LSB 0xF8
#define BME280_REG_PRESS_XLSB 0xF9

//Temperature
#define BME280_TEMP_CALIBRATION 0x88
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_TEMP_LSB 0xFB
#define BME280_REG_TEMP_XLSB 0xFC

//Humidity
#define BME280_HUM_CALIBRATION_1 0xA1
#define BME280_HUM_CALIBRATION_2 0xE1
#define BME280_REG_HUM_MSB 0xFD
#define BME280_REG_HUM_LSB 0xFE

//Other
#define BME280_REG_RESET 0xE0
#define BME280_REG_ID 0xD0

static const char *TAG = "BME280";

typedef struct {
    uint16_t dig_T1; //Linear offset, subtract from ADC reading
    int16_t dig_T2; //Scaling value
    int16_t dig_T3; //Nonlinear correction for extreme values
    uint8_t dig_H1; //Humidity calibration values H1-H6
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_data_t;

int32_t t_fine;

//Initialize ESP32 as I2C master
static esp_err_t i2c_master_init(void);

static esp_err_t bme280_init(void);

//Write a byte of data to specified register
static esp_err_t bme280_write_reg(uint8_t reg_addr, uint8_t data);

//Read data from a specified register
static esp_err_t bme280_read_reg(uint8_t reg_addr, uint8_t *data, size_t len);

//Obtain 16-bit calibration values
void bme280_calibrate_data(void);

//Taken from datasheet
static float bme280_compensate_temperature(uint32_t adc_t);

//Taken from datasheet
static float bme280_compensate_humidity(int32_t adc_h);

static float bme280_read_temperature(void);
static float bme280_read_humidity(void);