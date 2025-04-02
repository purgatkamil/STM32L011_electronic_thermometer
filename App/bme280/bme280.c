/**
 * @file bme280.c
 * @brief Driver for Bosch BME280 environmental sensor (temperature, pressure, humidity)
 * 
 * This implementation provides support for I2C communication with the BME280 sensor.
 * It performs initialization, data acquisition, and compensation using fixed-point 
 * arithmetic based on Bosch’s official datasheet (see section 4.2.3).
 * 
 * Key features:
 * - All measured values (temperature, pressure, humidity) are stored in a dedicated
 *   handler (`BME280_Data`) with integer and fractional parts separated. This avoids
 *   the use of floating-point numbers.
 * 
 * - The entire implementation is optimized for flash size — formulas and logic are 
 *   written in a compact and less verbose way to reduce memory footprint, even at the
 *   cost of some readability.
 * 
 * - Compensation algorithms follow Bosch-recommended fixed-point integer versions,
 *   ensuring accuracy while remaining efficient on low-power MCUs.
 * 
 * This driver is suitable for applications where memory and power efficiency are
 * prioritized over abstraction or extensibility.
 */

#include "bme280.h"
#include <stdio.h>

#define PRESSURE_OFFSET 200

static uint16_t dig_T1;
static int16_t dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t dig_H1, dig_H3;
static int16_t dig_H2, dig_H4, dig_H5;
static int8_t dig_H6;

static int32_t t_fine;
static BME280_Data bme_data;

/**
 * @brief Read calibration coefficients from BME280 non-volatile memory.
 *
 * This function reads the factory-programmed calibration parameters from the
 * sensor's memory (addresses 0x88 to 0xA1 and 0xE1 to 0xE7) and stores them in
 * global static variables. These coefficients are later used to compute the
 * compensated temperature, pressure, and humidity values as described in
 * section 4.2.2 of the datasheet.
 *
 * @param hi2c Pointer to HAL I2C handle
 */
static void BME280_read_calibration(I2C_HandleTypeDef *hi2c)
{
    uint8_t calib1[26], calib2[7];

    HAL_I2C_Mem_Read(hi2c, BME280_ADDRESS, 0x88, 1, calib1, 26, HAL_MAX_DELAY);
    HAL_I2C_Mem_Read(hi2c, BME280_ADDRESS, 0xE1, 1, calib2, 7, HAL_MAX_DELAY);

    dig_T1 = (calib1[1] << 8) | calib1[0];
    dig_T2 = (calib1[3] << 8) | calib1[2];
    dig_T3 = (calib1[5] << 8) | calib1[4];
    dig_P1 = (calib1[7] << 8) | calib1[6];
    dig_P2 = (calib1[9] << 8) | calib1[8];
    dig_P3 = (calib1[11] << 8) | calib1[10];
    dig_P4 = (calib1[13] << 8) | calib1[12];
    dig_P5 = (calib1[15] << 8) | calib1[14];
    dig_P6 = (calib1[17] << 8) | calib1[16];
    dig_P7 = (calib1[19] << 8) | calib1[18];
    dig_P8 = (calib1[21] << 8) | calib1[20];
    dig_P9 = (calib1[23] << 8) | calib1[22];

    dig_H1 = calib1[25];
    dig_H2 = (calib2[1] << 8) | calib2[0];
    dig_H3 = calib2[2];
    dig_H4 = (calib2[3] << 4) | (calib2[4] & 0x0F);
    dig_H5 = (calib2[5] << 4) | (calib2[4] >> 4);
    dig_H6 = (int8_t)calib2[6];
}

uint8_t BME280_init(I2C_HandleTypeDef *hi2c)
{
    uint8_t id = 0;
    HAL_I2C_Mem_Read(hi2c, BME280_ADDRESS, 0xD0, 1, &id, 1, HAL_MAX_DELAY);
    if (id != 0x60) return 0;

    uint8_t reset_cmd = 0xB6;
    HAL_I2C_Mem_Write(hi2c, BME280_ADDRESS, 0xE0, 1, &reset_cmd, 1, HAL_MAX_DELAY);
    HAL_Delay(100);

    // 1. Ustaw oversampling wilgotności ×4
    uint8_t hum_ctrl = 0x03;  // osrs_h = 011
    HAL_I2C_Mem_Write(hi2c, BME280_ADDRESS, 0xF2, 1, &hum_ctrl, 1, HAL_MAX_DELAY);

    // 2. Ustaw filtr IIR (coeff 16) i standby time 0.5ms
    uint8_t config = 0x10;  // filter = 100, t_sb = 000
    HAL_I2C_Mem_Write(hi2c, BME280_ADDRESS, 0xF5, 1, &config, 1, HAL_MAX_DELAY);

    // 3. Ustaw oversampling temp ×2, pressure ×16, tryb normalny
    uint8_t ctrl_meas = 0x57;  // osrs_t = 010, osrs_p = 101, mode = 11
    HAL_I2C_Mem_Write(hi2c, BME280_ADDRESS, 0xF4, 1, &ctrl_meas, 1, HAL_MAX_DELAY);

    BME280_read_calibration(hi2c);
    return 1;
}

void BME280_read_data(I2C_HandleTypeDef *hi2c)
{
    uint8_t buf[8];
    HAL_I2C_Mem_Read(hi2c, BME280_ADDRESS, 0xF7, 1, buf, 8, HAL_MAX_DELAY);

    int32_t adc_P = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = (buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
    int32_t adc_H = (buf[6] << 8) | buf[7];

    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
            ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    bme_data.temp_integer = T / 100;
    bme_data.temp_fraction = T % 100;

    int64_t p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;

    if (var1 == 0)
    {
        bme_data.pressure_integer = 0;
        bme_data.pressure_fraction = 0;
    }
    else
    {
        p = 1048576 - adc_P;
        p = (((p << 31) - var2) * 3125) / var1;
        var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (((int64_t)dig_P8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
        int32_t pressure_pa = (int32_t)(p / 256);
        bme_data.pressure_integer = (pressure_pa / 100) - PRESSURE_OFFSET;
        bme_data.pressure_fraction = pressure_pa % 100;
    }

    int32_t v_x1_u32r;
    v_x1_u32r = t_fine - ((int32_t)76800);
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) -
                   (((int32_t)dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));

    v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4);
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    int32_t humidity = v_x1_u32r >> 12; // in 1024ths of %RH
    bme_data.humidity_integer = humidity / 1024;
    bme_data.humidity_fraction = (humidity % 1024) * 100 / 1024;
}

int16_t BME280_get_temperature_integer(void)
{
    return bme_data.temp_integer;
}

int16_t BME280_get_temperature_fraction(void)
{
    return bme_data.temp_fraction;
}

int16_t BME280_get_pressure_integer(void)
{
    return bme_data.pressure_integer;
}

int16_t BME280_get_pressure_fraction(void)
{
    return bme_data.pressure_fraction;
}

int16_t BME280_get_humidity_integer(void)
{
    return bme_data.humidity_integer;
}

int16_t BME280_get_humidity_fraction(void)
{
    return bme_data.humidity_fraction;
}
