#ifndef BME280_H
#define BME280_H

#include "stm32l0xx_hal.h"

#define BME280_ADDRESS (0x77 << 1)  // or 0x76 if cbs with pull down

typedef struct {
    int16_t temp_integer;
    int16_t temp_fraction;

    int16_t pressure_integer;
    int16_t pressure_fraction;

    int16_t humidity_integer;
    int16_t humidity_fraction;
} BME280_Data;

uint8_t BME280_init(I2C_HandleTypeDef *hi2c);
void BME280_read_data(I2C_HandleTypeDef *hi2c);

int16_t BME280_get_temperature_integer(void);
int16_t BME280_get_temperature_fraction(void);

int16_t BME280_get_pressure_integer(void);
int16_t BME280_get_pressure_fraction(void);

int16_t BME280_get_humidity_integer(void);
int16_t BME280_get_humidity_fraction(void);

#endif // BME280_H
