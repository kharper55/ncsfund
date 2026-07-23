#ifndef BMX280_H
#define BMX280_H

#include "app_i2c.h"
#include "util.h"
#include <zephyr/kernel.h>      // for k_msleep()
#include <zephyr/logging/log.h> // for logging capability

// Note: BME280 contains humidity sensor. BMP280 does not.
// BMX stands for either BME or BMP - these values are valid for either device
// Where a feature is only relevant for one device or the other, the register is referred to as BME280 or BMP280

// BME280: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
// BMP280: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf

typedef int32_t BMX280_S32_t;
typedef uint32_t BMX280_U32_t;
typedef long long int BMX280_S64_t;

#define BMX280_I2C true // True for I2C, false for SPI
#define BMX280_SPI !BMX280_I2C
//#define BMX280_SPI_PORT spi0

#define BMX280_SLAVE_ADDR 0x76 // Value dependent on SDO voltage. Addr = 0x76 |= SDO. Cannot be left floating.

// Contains the chip identification number chip_id[7:0], which is 0x58 for BMP280 and
// 0x60 for BME280. This number can be read as soon as the device finished the power-on-reset.
#define BMX280_REG_ID         0xD0 // Read only
#define BMP280_REG_ID_VAL     0x58
#define BME280_REG_ID_VAL     0x60

// Contains the soft reset word reset[7:0]. If the value 0xB6 is written to the register,
// the device is reset using the complete power-on-reset procedure. Writing other values than 0xB6 has
// no effect. The readout value is always 0x00.
#define BMX280_REG_RESET       0xE0 // Write only
#define BMX280_REG_RESET_VALUE 0xB6

// Contains two bits which indicate the status of the device. Bit 3 is automatically set to '1'
// whenever a conversion is running and back to '0' when results have been transferred to the data
// registers. Bit 0 is automatically set to '1'  when the NVM data are being copied to image 
// registers and back to ‘0’ when the copying is done. The data are copied at power-on-reset 
// and before every conversion.
#define BMX280_REG_STATUS         0xF3 // Read only
#define BMX280_REG_STATUS_MEAS_BIT_POS 3 
#define BMX280_REG_STATUS_IMG_BIT_POS  0
typedef enum {
    BMX280_WAITING               = 0,
    BMX280_MEASURING             = 1,
    BMX280_IMAGING               = 2,
    BMX280_MEASURING_AND_IMAGING = 3
} bmx280_status_t;

// Sets the data acquisition options of the device. Bits [7:5] controls oversampling for temperature.
// Bits [4:2] controls oversampling for pressure. Bits [1:0] control the power mode of the device.
#define BMX280_REG_CTRL_MEAS      0xF4 // R/W
#define BME280_REG_CTRL_HUM       0xF2

// Oversampling settings (bits [7:5] and [4:2])
#define BMX280_REG_CTRL_MEAS_T_BIT_POS 5 // Ensure only writing 3 bit data
#define BMX280_REG_CTRL_MEAS_P_BIT_POS 2 // Ensure only writing 3 bit data
#define BME280_REG_CTRL_MEAS_H_BIT_POS 0 // Ensure only writing 3 bit data
typedef enum {
    BMX280_SKIP_MEAS    = 0,
    BMX280_OVERSAMP_X1  = 1,
    BMX280_OVERSAMP_X2  = 2,
    BMX280_OVERSAMP_X4  = 3,
    BMX280_OVERSAMP_X8  = 4,
    BMX280_OVERSAMP_X16 = 5
} bmx280_osrs_t;

// Power settings (bits [1:0])
#define BMX280_REG_CTRL_MEAS_PWR_BIT_POS 0  // Ensure only writing 2 bit data
typedef enum {
    BMX280_PWR_MODE_SLP  = 0,
    BMX280_PWR_MODE_FRC  = 1,
    BMX280_PWR_MODE_NORM = 3
} bmx280_mode_t;

// Sets the rate, filter and interface options of the device. Writes to the “config”
// register in normal mode may be ignored. In sleep mode writes are not ignored.
// Bits [7:5] set the inactive duration / tstandby in normal mode.
// Bits [4:2] control the time constant of the IIR filter
// Bit 0 enables 3-wire SPI interface when set to '1'
#define BMX280_REG_CONFIG     0xF5 // R/W 

// Tstdby settings
#define BMX280_REG_CONFIG_TSTDBY_BIT_POS 5
typedef enum {
    BMX280_TSTDBY_0_5MS   = 0b000,
    BMX280_TSTDBY_62_5MS  = 0b001,
    BMX280_TSTDBY_125MS   = 0b010,
    BMX280_TSTDBY_250MS   = 0b011,
    BMX280_TSTDBY_500MS   = 0b100,
    BMX280_TSTDBY_1000MS  = 0b101,
    BMP280_TSTDBY_2000MS  = 0b110,
    BMP280_TSTDBY_4000MS  = 0b111,
    BME280_TSTDBY_10MS    = 0b110,
    BME280_TSTDBY_20MS    = 0b111
} bmx280_tsdby_t;

// Filter settings
#define BMX280_REG_CONFIG_FILT_BIT_POS 2
typedef enum {
    BMX280_FILT_OFF      = 0,
    BMX280_FILT_COEFF_2  = 1,
    BMX280_FILT_COEFF_4  = 2,
    BMX280_FILT_COEFF_8  = 3,
    BMX280_FILT_COEFF_16 = 4
} bmx280_filter_t;

// Contains the raw pressure measurement output data up[19:0]
#define BMX280_REG_PRESS_MSB  0xF7 // Read only, bits [19:12] of pressure data
#define BMX280_REG_PRESS_LSB  0xF8 // Read only, bits [11:4] of pressure data
#define BMX280_REG_PRESS_XLSB 0xF9 // Read only, bits [3:0] of pressure data exist in bits [7:4] of this register
#define BMX280_PRESS_RAW_LEN 3

// Contains the raw temperature measurement output data ut[19:0]
#define BMX280_REG_TEMP_MSB   0xFA // Read only, bits [19:12] of temperature data
#define BMX280_REG_TEMP_LSB   0xFB // Read only, bits [11:4] of temperature data
#define BMX280_REG_TEMP_XLSB  0xFC // Read only, bits [3:0] of temperature data exist in bits [7:4] of this register
#define BMX280_TEMP_RAW_LEN  3

// Contains the raw humidity measurement output data uh[15:0]
#define BME280_REG_HUM_MSB 0xFD // Read only, bits [15:8] of humidity data
#define BME280_REG_HUM_LSB 0xFE // Read only, bits [7:0] of humidity data
#define BME280_HUM_RAW_LEN   2

/*
Temperature measurement can be enabled or skipped. Skipping the measurement could be useful to
measure pressure extremely rapidly. When enabled, several oversampling options exist. Each
oversampling step reduces noise and increases the output resolution by one bit, which is stored in the
XLSB data register 0xFC. Enabling/disabling the temperature measurement and oversampling setting
are selected through the osrs_t[2:0] bits in control register 0xF4.
*/


// The trimming parameters are programmed into the devices’ non-volatile memory (NVM) during
// production and cannot be altered by the customer. Each compensation word is a 16-bit signed or
// unsigned integer value stored in two’s complement. As the memory is organized into 8-bit words, two
// words must always be combined in order to represent the compensation word. The 8-bit registers are
// named calib00…calib25 and are stored at memory addresses 0x88…0xA1. The corresponding
// compensation words are named dig_T# for temperature compensation related values and dig_P# for
// pressure compensation related values. The mapping is shown in Table 17.
#define BMX280_REG_CALIB00 0x88
#define BMX280_REG_CALIB01 0x89 
#define BMX280_REG_CALIB02 0x8A
#define BMX280_REG_CALIB03 0x8B
#define BMX280_REG_CALIB04 0x8C
#define BMX280_REG_CALIB05 0x8D
#define BMX280_REG_CALIB06 0x8E
#define BMX280_REG_CALIB07 0x8F
#define BMX280_REG_CALIB08 0x90
#define BMX280_REG_CALIB09 0x91
#define BMX280_REG_CALIB10 0x92
#define BMX280_REG_CALIB11 0x93
#define BMX280_REG_CALIB12 0x94
#define BMX280_REG_CALIB13 0x95
#define BMX280_REG_CALIB14 0x96
#define BMX280_REG_CALIB15 0x97
#define BMX280_REG_CALIB16 0x98
#define BMX280_REG_CALIB17 0x99
#define BMX280_REG_CALIB18 0x9A
#define BMX280_REG_CALIB19 0x9B
#define BMX280_REG_CALIB20 0x9C
#define BMX280_REG_CALIB21 0x9D
#define BMX280_REG_CALIB22 0x9E
#define BMX280_REG_CALIB23 0x9F
#define BMX280_REG_CALIB24 0xA0 // Unused / reserved in BMX280
#define BMX280_REG_CALIB25 0xA1 // Unused / reserved in BMP280 only
#define BMX280_REG_CALIB26 0xE1
#define BMX280_REG_CALIB27 0xE2
#define BMX280_REG_CALIB28 0xE3
#define BMX280_REG_CALIB29 0xE4
#define BMX280_REG_CALIB30 0xE5
#define BMX280_REG_CALIB31 0xE6
#define BMX280_REG_CALIB32 0xE7
#define BMX280_REG_CALIB33 0xE8
#define BMX280_REG_CALIB34 0xE9
#define BMX280_REG_CALIB35 0xEA
#define BMX280_REG_CALIB36 0xEB
#define BMX280_REG_CALIB37 0xEC
#define BMX280_REG_CALIB38 0xED
#define BMX280_REG_CALIB39 0xEE
#define BMX280_REG_CALIB40 0xEF
#define BMX280_REG_CALIB41 0xF0

// Compensation parameter storage (temperature), 16 bit words stored in adjacent 8 bit registers, LSB at lower addr
#define BMX280_REG_DIG_T1 BMX280_REG_CALIB00 // Unsigned short
#define BMX280_REG_DIG_T2 BMX280_REG_CALIB02 // Signed short
#define BMX280_REG_DIG_T3 BMX280_REG_CALIB04 // Signed short

// Compensation parameter storage (pressure), 16 bit words stored in adjacent 8 bit registers, LSB at lower addr
#define BMX280_REG_DIG_P1 BMX280_REG_CALIB06 // Unsigned short
#define BMX280_REG_DIG_P2 BMX280_REG_CALIB08 // Signed short
#define BMX280_REG_DIG_P3 BMX280_REG_CALIB10 // Signed short
#define BMX280_REG_DIG_P4 BMX280_REG_CALIB12 // Signed short
#define BMX280_REG_DIG_P5 BMX280_REG_CALIB14 // Signed short
#define BMX280_REG_DIG_P6 BMX280_REG_CALIB16 // Signed short
#define BMX280_REG_DIG_P7 BMX280_REG_CALIB18 // Signed short
#define BMX280_REG_DIG_P8 BMX280_REG_CALIB20 // Signed short
#define BMX280_REG_DIG_P9 BMX280_REG_CALIB22 // Signed short

// Compensation parameter storage (humidity), 16 bit words stored in adjacent 8 bit registers, LSB at lower addr
#define BME280_REG_DIG_H1 BMX280_REG_CALIB25 // Unsigned char
#define BME280_REG_DIG_H2 BMX280_REG_CALIB26 // Signed short
#define BME280_REG_DIG_H3 BMX280_REG_CALIB28 // Unsigned char
#define BME280_REG_DIG_H4 BMX280_REG_CALIB29 // Signed short (11 bits, use only bits [3:0] from 0xE5)
#define BME280_REG_DIG_H5 BMX280_REG_CALIB30 // Signed short (11 bits, use only bits [7:4] from 0xE5)
#define BME280_REG_DIG_H6 BMX280_REG_CALIB32 // Signed char

// Pack the MSB, LSB, and XLSB register values into a 32 bit signed int
#define BMX280_PACK_DATA_20BIT(MSB, LSB, XLSB) (int32_t)((uint32_t)MSB << 12 | (uint32_t)LSB << 4 | ((uint32_t)XLSB >> 4))
#define BME280_PACK_DATA_16BIT(MSB, LSB)       (int32_t)((uint32_t)MSB << 8 | (uint32_t)LSB << 0)
#define PASCAL_2_ATM(pascal) (double)(pascal * 9.86923 * 1/1000000) // Use to convert pressure values from BMP280 to units of ATM
#define C_2_F(c) (double)(c * 9/5 + 32)

/**\name API warning code */
// tHESE ARE FROM BOSCH
#define BME280_W_SELF_TEST_FAIL INT8_C(2)
#define BME280_CRC_DATA_ADDR	UINT8_C(0xE8)
#define BME280_CRC_DATA_LEN	    UINT8_C(1)
#define BME280_CRC_CALIB1_ADDR	UINT8_C(0x88)
#define BME280_CRC_CALIB1_LEN	UINT8_C(26)
#define BME280_CRC_CALIB2_ADDR	UINT8_C(0xE1)
#define BME280_CRC_CALIB2_LEN	UINT8_C(7)

typedef enum {
    BMP280 = 0x58, // 0x58 in device ID register @ addr 0xD0
    BME280 = 0x60, // 0x60 in device ID register @ addr 0xD0
    UNKNOWN        // Device ID register has not yet been read
} bmx280_dev_t;

typedef struct {
    bmx280_mode_t mode;
    bmx280_tsdby_t tsdby;
    bmx280_filter_t filt;
    bmx280_osrs_t osrs_temp; 
    bmx280_osrs_t osrs_press;
    bmx280_osrs_t osrs_hum;
    bmx280_dev_t dev;
} bmx280_config_t;

// See section 10 in BME280 datasheet rev1.23 for implementation details
// If necessary, adapt the measurement plausibility limits per your environment
// Keep in mind also the recommended anneal/rest period following reflow solder attach
// See section 7.9 "Reconditioning Procedure"
typedef enum {
    BMX280_OK                      = 0,
    BMX280_COMM_ERR_OR_WRONG_DEV   = 10,
    BMX280_TRIM_DATA_OOB           = 20,
    BMX280_TEMP_BW_OR_MEMS_DEFECT  = 30,
    BMX280_PRESS_BW_OR_MEMS_DEFECT = 31,
    BMX280_HUM_BW_OR_MEMS_DEFECT   = 32, // Added on my own volition
    BMX280_IMPLAUSIBLE_TEMP        = 40, // Default limits are 0-40C
    BMX280_IMPLAUSIBLE_PRESS       = 41, // Default limits are 900-1100hPa
    BMX280_IMPLAUSIBLE_HUM         = 42  // Default limits are 20-80%rH
} bmx280_self_test_result_t;

// Compensation functions provided by Bosch in BMX280 datasheet
BMX280_S32_t bmx280_compensate_T_int32(BMX280_S32_t adc_T);
BMX280_U32_t bmx280_compensate_P_int64(BMX280_S32_t adc_P);
BMX280_U32_t bme280_compensate_H_int32(BMX280_S32_t adc_H); // Only relevant for BME280 (BMP280 doesnt have humidity option)

nrf_err_t bmx280_init(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, bool rst);
nrf_err_t bmx280_sw_reset(const struct i2c_dt_spec * i2c);
nrf_err_t bmx280_check_status(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, bmx280_status_t * status);
nrf_err_t bmx280_set_mode(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg);
nrf_err_t bmx280_self_test(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, bmx280_self_test_result_t * result);
/*make these static?*/
// Chip wants to be burst read for all 3 measurement peripherals when being used... 
// If we want to be able to config for normal mode or frc mode and forget, should have one 
// single public call in the API which accomplishes all three, pending the actual dev config.
nrf_err_t bmx280_read_temp(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * temp);
nrf_err_t bmx280_read_press(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, uint32_t * press);
nrf_err_t bme280_read_hum(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, uint32_t * hum);
nrf_err_t bmx280_read_measurements(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * temp, uint32_t * press, uint32_t * hum);

/*!
 * @brief This API reads the stored CRC and then compare with calculated CRC
 *
 * @param[in] dev : Structure instance of bme280_dev.
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
//int8_t bme280_crc_selftest(const struct bme280_dev *dev);
bmx280_self_test_result_t bme280_crc_selftest(const struct i2c_dt_spec * i2c);

#endif // BMX280_H