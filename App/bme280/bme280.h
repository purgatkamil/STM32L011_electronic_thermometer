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

/**
 * @brief Initialize the BME280 sensor with specific configuration.
 *
 * This function performs the following:
 * - Verifies the sensor's ID (should be 0x60)
 * - Issues a software reset
 * - Configures oversampling (temp ×2, pressure ×16, humidity ×4)
 * - Sets IIR filter coefficient = 16 and standby time = 0.5ms
 * - Activates normal mode operation
 *
 * These settings correspond to indoor navigation mode, which provides
 * high resolution and low noise, suitable for altitude change detection.
 *
 * @param hi2c Pointer to HAL I2C handle
 * @return 1 if initialization succeeded, 0 otherwise
 */
uint8_t BME280_init(I2C_HandleTypeDef *hi2c);

/**
 * @brief Read and compensate temperature, pressure, and humidity data.
 *
 * This function performs the full measurement cycle:
 * - Reads raw ADC values from the data registers (0xF7 - 0xFE)
 * - Applies Bosch's compensation formulas (from section 4.2.3 in datasheet)
 *   - Temperature compensation produces a value in 0.01 °C resolution.
 *   - Pressure is calculated using a 64-bit integer algorithm and returned in Pa.
 *   - Humidity is computed in Q22.10 fixed-point format and returned in %RH.
 *
 * These formulas use the calibration coefficients retrieved earlier.
 * 
 * @param hi2c Pointer to HAL I2C handle
 */
void BME280_read_data(I2C_HandleTypeDef *hi2c);

/**
 * @brief Get integer part of the last measured temperature.
 * 
 * @return Temperature in °C (integer part only)
 */
int16_t BME280_get_temperature_integer(void);

/**
 * @brief Get fractional part of the last measured temperature.
 * 
 * @return Temperature in °C (fractional part, 0–99)
 */
int16_t BME280_get_temperature_fraction(void);

/**
 * @brief Get integer part of the last measured pressure.
 * 
 * @return Pressure in hPa (integer part only)
 */
int16_t BME280_get_pressure_integer(void);

/**
 * @brief Get fractional part of the last measured pressure.
 * 
 * @return Pressure in hPa (fractional part, 0–99)
 */
int16_t BME280_get_pressure_fraction(void);

/**
 * @brief Get integer part of the last measured humidity.
 * 
 * @return Relative humidity in % (integer part)
 */
int16_t BME280_get_humidity_integer(void);

/**
 * @brief Get fractional part of the last measured humidity.
 * 
 * @return Relative humidity in % (fractional part, 0–99)
 */
int16_t BME280_get_humidity_fraction(void);

#endif // BME280_H
