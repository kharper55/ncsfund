#include "bmx280.h"

uint16_t dig_T1;
int16_t dig_T2;
int16_t dig_T3;

uint16_t dig_P1;
int16_t dig_P2;
int16_t dig_P3;
int16_t dig_P4;
int16_t dig_P5;
int16_t dig_P6;
int16_t dig_P7;
int16_t dig_P8;
int16_t dig_P9;

uint8_t dig_H1;
int16_t dig_H2;
uint8_t dig_H3;
int16_t dig_H4;
int16_t dig_H5;
int8_t dig_H6;

BMX280_S32_t t_fine;

/*
In normal mode, the timing of measurements is not necessarily synchronized to the readout by the
user. This means that new measurement results may become available while the user is reading the
results from the previous measurement. In this case, shadowing is performed in order to guarantee
data consistency. Shadowing will only work if all data registers are read in a single burst read.
Therefore, the user must use burst reads if he does not synchronize data readout with the
measurement cycle. Using several independent read commands may result in inconsistent data.
If a new measurement is finished and the data registers are still being read, the new measurement
results are transferred into shadow data registers. The content of shadow registers is transferred into
data registers as soon as the user ends the burst read, even if not all data registers were read.

Hence, all reads should be burst reads
*/

// Bosch recommended configuration for weather monitoring application
bmx280_config_t bmx280_weather_mon_cfg = { // 0.16uA current consumption
    BMX280_PWR_MODE_FRC,
    BMX280_TSTDBY_0_5MS, // Value inconsequential in forced operating mode
    BMX280_FILT_OFF,
    BMX280_OVERSAMP_X1,  // Temp
    BMX280_OVERSAMP_X1,  // Press
    BMX280_OVERSAMP_X1,  // Hum
    UNKNOWN              // Should be set by user at run-time
};

// Bosch recommended configuration for humidity sensing application
bmx280_config_t bmx280_hum_sensing_cfg = { // 2.9uA current consumption
    BMX280_PWR_MODE_FRC,
    BMX280_TSTDBY_0_5MS, // Value inconsequential in forced operating mode
    BMX280_FILT_OFF,
    BMX280_OVERSAMP_X1,  // Temp
    BMX280_SKIP_MEAS,    // Press
    BMX280_OVERSAMP_X1,  // Hum
    UNKNOWN
};

// Bosch recommended configuration for indoor navigation application
bmx280_config_t bmx280_indoor_nav_cfg = { // 633uA current consumption
    BMX280_PWR_MODE_NORM,
    BMX280_TSTDBY_0_5MS,
    BMX280_FILT_COEFF_16,
    BMX280_OVERSAMP_X2,  // Temp
    BMX280_OVERSAMP_X16, // Press
    BMX280_OVERSAMP_X1,  // Hum
    UNKNOWN
};

// Bosch recommended configuration for gaming application
bmx280_config_t bmx280_gaming_cfg = { // 581uA current consumption
    BMX280_PWR_MODE_NORM,
    BMX280_TSTDBY_0_5MS,
    BMX280_FILT_COEFF_16,
    BMX280_OVERSAMP_X1,  // Temp
    BMX280_OVERSAMP_X4,  // Press
    BMX280_SKIP_MEAS,    // Hum
    UNKNOWN
};

LOG_MODULE_REGISTER(bmx280, LOG_LEVEL_DBG);

/*!
 * @brief This API calculates the CRC
 *
 * @param[in] mem_values : reg_data parameter to calculate CRC
 * @param[in] mem_length : Parameter to calculate CRC
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static uint8_t crc_calculate(uint8_t *mem_values, uint8_t mem_length);

// Compensation functions provided by Bosch in BMX280 datasheet

// ===========================================================================================================================================
// Returns temperature in DegC as signed 32 bit integer, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value. Must call bmp280_compensate_T_int32() before pressure/humidity compensation since 
// these depend on t_fine, which is updated via bmp280_compensate_T_int32()
BMX280_S32_t bmx280_compensate_T_int32(BMX280_S32_t adc_T) {
    BMX280_S32_t var1, var2, T;
    var1 = ((((adc_T>>3) - ((BMX280_S32_t)dig_T1<<1))) * ((BMX280_S32_t)dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((BMX280_S32_t)dig_T1)) * ((adc_T>>4) - ((BMX280_S32_t)dig_T1)))>> 12) * ((BMX280_S32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

// ===========================================================================================================================================
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BMX280_U32_t bmx280_compensate_P_int64(BMX280_S32_t adc_P) {
    BMX280_S64_t var1, var2, p;
    var1 = ((BMX280_S64_t)t_fine) - 128000;
    var2 = var1 * var1 * (BMX280_S64_t)dig_P6;
    var2 = var2 + ((var1*(BMX280_S64_t)dig_P5)<<17);
    var2 = var2 + (((BMX280_S64_t)dig_P4)<<35);
    var1 = ((var1 * var1 * (BMX280_S64_t)dig_P3)>>8) + ((var1 * (BMX280_S64_t)dig_P2)<<12);
    var1 = (((((BMX280_S64_t)1)<<47)+var1))*((BMX280_S64_t)dig_P1)>>33;
    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576-adc_P;
    p = (((p<<31)-var2)*3125)/var1;
    var1 = (((BMX280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
    var2 = (((BMX280_S64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BMX280_S64_t)dig_P7)<<4);
    return (BMX280_U32_t)p;
}

// ===========================================================================================================================================
// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH
BMX280_U32_t bme280_compensate_H_int32(BMX280_S32_t adc_H) {
    BMX280_S32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((BMX280_S32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((BMX280_S32_t)dig_H4) << 20) - (((BMX280_S32_t)dig_H5) *
    v_x1_u32r)) + ((BMX280_S32_t)16384)) >> 15) * (((((((v_x1_u32r *
    ((BMX280_S32_t)dig_H6)) >> 10) * (((v_x1_u32r * ((BMX280_S32_t)dig_H3)) >> 11) +
    ((BMX280_S32_t)32768))) >> 10) + ((BMX280_S32_t)2097152)) * ((BMX280_S32_t)dig_H2) +
    8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
    ((BMX280_S32_t)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (BMX280_U32_t)(v_x1_u32r>>12);
}

// ===========================================================================================================================================
nrf_err_t bmx280_sw_reset(const struct i2c_dt_spec * i2c) {

    uint8_t txdata = BMX280_REG_RESET_VALUE;

    nrf_err_t err = i2c_reg_write(i2c, BMX280_REG_RESET, &txdata, 1); // Force software reset by writing the reset word to device
    
    return err;
}

// ===========================================================================================================================================
nrf_err_t bmx280_init(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, bool rst) { // need to update this with settings for run-time config...

    bmx280_mode_t mode = cfg->mode;
    bmx280_tsdby_t tsdby = cfg->tsdby;
    bmx280_filter_t filt = cfg->filt;
    bmx280_osrs_t osrs_temp = cfg->osrs_temp;
    bmx280_osrs_t osrs_press = cfg->osrs_press;
    bmx280_osrs_t osrs_hum = cfg->osrs_hum;

    uint8_t rxData[2];

    uint8_t txdata = (mode << BMX280_REG_CTRL_MEAS_PWR_BIT_POS) & 0xFF; // Set power mode to normal
    txdata |= (osrs_temp << BMX280_REG_CTRL_MEAS_T_BIT_POS); // Set oversampling value for temperature to x1 (enable its measurement)
    txdata |= (osrs_press << BMX280_REG_CTRL_MEAS_P_BIT_POS); // Set oversampling value for pressure to x1 (enable its measurement)
    
    uint8_t txdata2 = osrs_hum << BME280_REG_CTRL_MEAS_H_BIT_POS; // Set oversampling value for humidity to x1 (enable its measurement)

    uint8_t device_id = 0x00;
    uint8_t * device_id_str;

    uint8_t config_reg_data = (tsdby << BMX280_REG_CONFIG_TSTDBY_BIT_POS) | (filt << BMX280_REG_CONFIG_FILT_BIT_POS); // In sleep mode, writes to config register are ignored.

    nrf_err_t err = ENONE;

    //sleep_ms(6000);

    LOG_INF("Reading BMX280 ID Register @ Address 0xD0...");

    err = i2c_reg_read(i2c, BMX280_REG_ID, &device_id, 1); 
    if (err != ENONE) return err;

    device_id_str = (device_id == BMP280_REG_ID_VAL ? "BMP280" : (device_id == BME280_REG_ID_VAL ? "BME280" : "UNKNOWN"));
    
    LOG_INF("Device ID is 0x%X. Device is %s.", device_id, device_id_str);
    if (device_id != BMP280_REG_ID_VAL && device_id != BME280_REG_ID_VAL) {
        err = -EGENERIC;
        return err;
    }
    else cfg->dev = device_id;

    if (rst) {
        err = bmx280_sw_reset(i2c);
        if (err != ENONE) return err;
        k_msleep(10);
    }

    LOG_INF("Fetching compensation parameters...");

    // Temp compensation values

    // Look into making these burst reads...

    err = i2c_reg_read(i2c, BMX280_REG_DIG_T1, rxData, 2); 
    if (err != ENONE) return err;
    dig_T1 = (uint16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_T1: %u", dig_T1);
    
    err = i2c_reg_read(i2c, BMX280_REG_DIG_T2, rxData, 2);
    if (err != ENONE) return err;
    dig_T2 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_T2: %d", dig_T2);

    err = i2c_reg_read(i2c, BMX280_REG_DIG_T3, rxData, 2); 
    if (err != ENONE) return err;
    dig_T3 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_T3: %d", dig_T3);

    // Pressure compensation values

    err = i2c_reg_read(i2c, BMX280_REG_DIG_P1, rxData, 2); 
    if (err != ENONE) return err;
    dig_P1 = (uint16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P1: %u", dig_P1);
    
    err = i2c_reg_read(i2c, BMX280_REG_DIG_P2, rxData, 2); 
    if (err != ENONE) return err;
    dig_P2 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P2: %d", dig_P2);

    err = i2c_reg_read(i2c, BMX280_REG_DIG_P3, rxData, 2); 
    if (err != ENONE) return err;
    dig_P3 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P3: %d", dig_P3);

    err = i2c_reg_read(i2c, BMX280_REG_DIG_P4, rxData, 2); 
    if (err != ENONE) return err;
    dig_P4 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P4: %d", dig_P4);
    
    err = i2c_reg_read(i2c, BMX280_REG_DIG_P5, rxData, 2); 
    if (err != ENONE) return err;
    dig_P5 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P5: %d", dig_P5);

    err = i2c_reg_read(i2c, BMX280_REG_DIG_P6, rxData, 2); 
    if (err != ENONE) return err;
    dig_P6 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P6: %d", dig_P6);

    err = i2c_reg_read(i2c, BMX280_REG_DIG_P7, rxData, 2); 
    if (err != ENONE) return err;
    dig_P7 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P7: %d", dig_P7);
    
    err = i2c_reg_read(i2c, BMX280_REG_DIG_P8, rxData, 2); 
    if (err != ENONE) return err;
    dig_P8 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P8: %d", dig_P8);

    err = i2c_reg_read(i2c, BMX280_REG_DIG_P9, rxData, 2); 
    if (err != ENONE) return err;
    dig_P9 = (int16_t)(rxData[0] | (rxData[1] << 8));
    LOG_INF("dig_P9: %d", dig_P9);

    // Humidity compensation values

    if (device_id == BME280_REG_ID_VAL) { // Only BME280 has the humidity peripheral

        err = i2c_reg_read(i2c, BME280_REG_DIG_H1, rxData, 1); 
        if (err != ENONE) return err;
        dig_H1 = (uint8_t)rxData[0];
        LOG_INF("dig_H1: %d", dig_H1);
        
        err = i2c_reg_read(i2c, BME280_REG_DIG_H2, rxData, 2); 
        if (err != ENONE) return err;
        dig_H2 = (int16_t)(rxData[0] | (rxData[1] << 8));
        LOG_INF("dig_H2: %d", dig_H2);

        err = i2c_reg_read(i2c, BME280_REG_DIG_H3, rxData, 2); 
        if (err != ENONE) return err;
        dig_H3 = (uint8_t)rxData[0];
        LOG_INF("dig_H3: %d", dig_H3);

        err = i2c_reg_read(i2c, BME280_REG_DIG_H4, rxData, 2); 
        if (err != ENONE) return err;
        dig_H4 = (int16_t)((rxData[0] << 4) | (rxData[1] & 0x0F)); // Should double check this
        LOG_INF("dig_H4: %d", dig_H4);
        
        err = i2c_reg_read(i2c, BME280_REG_DIG_H5, rxData, 2); 
        if (err != ENONE) return err;
        dig_H5 = (int16_t)((rxData[0] >> 4) | (rxData[1] << 4)); // Should double check this
        LOG_INF("dig_H5: %d", dig_H5);

        err = i2c_reg_read(i2c, BME280_REG_DIG_H6, rxData, 1); 
        if (err != ENONE) return err;
        dig_H6 = (int8_t)rxData[0];
        LOG_INF("dig_H6: %d", dig_H6);
    }

    LOG_INF("Configuring %s for normal mode operation with no oversampling...", device_id_str);

    // Note: The “ctrl_meas” register sets the pressure and temperature data acquisition options of the device. The
    // register needs to be written after changing “ctrl_hum” for the changes to become effective.

    // But keep in mind that the “ctrl_hum” register sets the humidity data acquisition options of the device. Changes to this
    // register only become effective after a write operation to “ctrl_meas”.

    err = i2c_reg_write(i2c, BME280_REG_CTRL_HUM, &txdata2, 1); // Enable humidity by setting oversampling to x1
    if (err != ENONE) return err;

    err = i2c_reg_write(i2c, BMX280_REG_CTRL_MEAS, &txdata, 1); // Take out of sleep and enable temp / pressure
    if (err != ENONE) return err;
    
    LOG_INF("Done configuring %s.", device_id_str);

    return err;

}

// ===========================================================================================================================================
nrf_err_t bmx280_check_status(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, bmx280_status_t * status) {
    nrf_err_t err = ENONE;
    uint8_t buff;

    err = i2c_reg_read(i2c, BMX280_REG_STATUS, &buff, 1); // Read back status register
    if (err != ENONE) return err;
    *status = (((buff >> BMX280_REG_STATUS_MEAS_BIT_POS) & 0x01) | (((buff >> BMX280_REG_STATUS_IMG_BIT_POS) & 0x01) << 1));
    
    return err;
}

// ===========================================================================================================================================
nrf_err_t bmx280_set_mode(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg) {

    nrf_err_t err = ENONE;
    bmx280_mode_t mode = cfg->mode;
    uint8_t data;

    if (!(mode == BMX280_PWR_MODE_SLP || mode == BMX280_PWR_MODE_FRC || mode == BMX280_PWR_MODE_NORM)) return -EGENERIC;

    // If cfg-> mode == BMX280_PWR_MODE_FRC, must set mode, then wait tmeas before reading is possible
    // In forced mode, the device returns to sleep mode after measurement

    err = i2c_reg_read(i2c, BMX280_REG_CTRL_MEAS, &data, 1);  // Read ctrl_meas register
    if (err != ENONE) return err;

    data &= ~(0x03 << BMX280_REG_CTRL_MEAS_PWR_BIT_POS);       // Preserve all bits except for the power mode bits
    data |= (mode & 0x03) << BMX280_REG_CTRL_MEAS_PWR_BIT_POS; // Write to power mode bits

    err = i2c_reg_write(i2c, BMX280_REG_CTRL_MEAS, &data, 1); // Write back to ctrl_meas register
    if (err != ENONE) return err;

    return err;
}

/* NEED TO LOOK INTO THE FOLLOWING...
- Whats the deal with BME280 vs BMP280 data formatting... confused about XLSB usage on BME280...? Its different than from BMP280
- Need to think about how to handle humidity / pressure measurement without tfine? If temperature is disabled, can we grab these values?...
Not important for the application but matters for the driver... Should probably not go this route, requires a lot of application knowledge to
make such a call... with slow changing temperature, its prob fine...
- Add Doxygen style comments
- consider making the raw functions return only: PICO_ERROR_NONE instead of byte counts
- play with various filter and tsdby settings, etc.. investigate best low power settings
*/

// ===========================================================================================================================================
static nrf_err_t bmx280_read_temp_raw(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * temp_raw) {

    nrf_err_t err = ENONE;
    uint8_t buff[BMX280_TEMP_RAW_LEN];

    if (cfg->mode == BMX280_PWR_MODE_FRC) err = bmx280_set_mode(i2c, cfg);
    if (err != ENONE) return err;

    if (temp_raw == NULL) return -EGENERIC;

    //printf("Reading temperature...\n\n");
    err = i2c_reg_read(i2c, BMX280_REG_TEMP_MSB, buff, BMX280_TEMP_RAW_LEN); // Read back temperature measurement
    if (err != ENONE) return err;
    *temp_raw = BMX280_PACK_DATA_20BIT(buff[0], buff[1], buff[2]);
    
    return err;
}

// ===========================================================================================================================================
nrf_err_t bmx280_read_temp(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * temp) {

    nrf_err_t err = ENONE;
    int32_t temp_raw;
    
    err = bmx280_read_temp_raw(i2c, cfg, &temp_raw);
    if (err == ENONE) *temp = bmx280_compensate_T_int32(temp_raw);

    return err;
}

// ===========================================================================================================================================
static nrf_err_t bmx280_read_press_raw(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * press_raw) {

    nrf_err_t err = ENONE;
    uint8_t buff[BMX280_PRESS_RAW_LEN];

    if (cfg->mode == BMX280_PWR_MODE_FRC) err = bmx280_set_mode(i2c, cfg);
    if (err != ENONE) return err;

    if (press_raw == NULL) return -EGENERIC;

    //printf("Reading pressure...\n\n");
    err = i2c_reg_read(i2c, BMX280_REG_PRESS_MSB, buff, BMX280_PRESS_RAW_LEN); // Read back pressure measurement, let the BMP280 auto increment registers
    if (err != ENONE) return err;
    *press_raw = BMX280_PACK_DATA_20BIT(buff[0], buff[1], buff[2]);

    return err;
}

// ===========================================================================================================================================
nrf_err_t bmx280_read_press(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, uint32_t * press) {

    nrf_err_t err = ENONE;
    int32_t press_raw;

    if (cfg->mode == BMX280_PWR_MODE_FRC) err = bmx280_set_mode(i2c, cfg); 
    if (err != ENONE) return err;
    
    err = bmx280_read_press_raw(i2c, cfg, &press_raw);
    if (err != ENONE) return err;
    *press = bmx280_compensate_P_int64(press_raw);

    return err;
}

// ===========================================================================================================================================
static nrf_err_t bme280_read_hum_raw(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * hum_raw) {

    nrf_err_t err = ENONE;
    uint8_t buff[BME280_HUM_RAW_LEN];

    if (cfg->mode == BMX280_PWR_MODE_FRC) err = bmx280_set_mode(i2c, cfg); 
    if (err != ENONE) return err;

    if (hum_raw == NULL) return -EGENERIC;

    //printf("Reading humidity...\n\n");
    err = i2c_reg_read(i2c, BME280_REG_HUM_MSB, buff, BME280_HUM_RAW_LEN); // Read back humidity measurement
    if (err != ENONE) return err;
    *hum_raw = BME280_PACK_DATA_16BIT(buff[0], buff[1]);

    return err;
}

// ===========================================================================================================================================
nrf_err_t bme280_read_hum(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, uint32_t * hum) {

    nrf_err_t err = ENONE;
    int32_t hum_raw;

    if (cfg->mode == BMX280_PWR_MODE_FRC) err = bmx280_set_mode(i2c, cfg);
    if (err != ENONE) return err;

    err = bme280_read_hum_raw(i2c, cfg, &hum_raw);
    if (err != ENONE) return err;
    *hum = bme280_compensate_H_int32(hum_raw);

    return err;
}

// ===========================================================================================================================================
nrf_err_t bmx280_self_test(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, bmx280_self_test_result_t * result) {

    nrf_err_t err = ENONE;
    uint8_t device_id;
    //bmx280_config_t cfg = {BMX280_PWR_MODE_NORM, BMX280_TSTDBY_0_5MS, BMX280_FILT_OFF, BMX280_OVERSAMP_X1, BMX280_OVERSAMP_X1, BMX280_OVERSAMP_X1};
    int32_t temp_raw;
    int32_t press_raw;
    int32_t hum_raw;
    int32_t temp;
    uint32_t press;
    uint32_t hum;
    *result = BMX280_COMM_ERR_OR_WRONG_DEV;

    err = bmx280_sw_reset(i2c); // Optional if device in known POR state
    k_msleep(5);

    // This function attempts to read the Chip ID. If it is correct, a functioning communication is assumed.
    // Note that the write function functionality is not explicitly tested.
    if (err == ENONE) err = i2c_reg_read(i2c, BMX280_REG_ID, &device_id, 1); 
    if (err == ENONE && (device_id == BMP280_REG_ID_VAL || device_id == BME280_REG_ID_VAL)) *result = BMX280_OK;
    else return err;

    // Trimming data out of bound check (implementation taken from Bosch Sensortec as they do not disclose CRC)
    *result = bme280_crc_selftest(i2c);
    if (*result != BMX280_OK) return -EGENERIC;

    err = bmx280_init(i2c, cfg, false);
    if (err != ENONE) return err;

    uint8_t status;

    do {
        i2c_reg_read(i2c, BMX280_REG_STATUS, &status, 1);
    } while (status & 0x09);   // measuring or NVM copy active

    // Temperature bond wire or MEMS defect check
    // A pressure and temperature measurement is performed and uncompensated pressure and
    // temperature values are read out. If the measurement results are clipped to the respective minimum or
    // maximum ADC values, this is usually caused by defective bond wires. However, a defective sensing
    // element could also cause this test to fail.
    // Please note that some combinations of bond wire or sensing element defects do not result in clipping
    // of the measurement value and will therefore not be detected with this test. These cases can be
    // detected by the plausibility test instead.
    err = bmx280_read_temp_raw(i2c, cfg, &temp_raw);
    if (err == ENONE) if (temp_raw == 0 || temp_raw == 0xFFFFF) *result = BMX280_TEMP_BW_OR_MEMS_DEFECT;

    // Pressure bond wire or MEMS defect check
    err = bmx280_read_press_raw(i2c, cfg, &press_raw);
    if (err == ENONE) if (press_raw == 0 || press_raw == 0xFFFFF) *result = BMX280_PRESS_BW_OR_MEMS_DEFECT;

    // Pressure bond wire or MEMS defect check (Not actually listed in datasheet, but assuming its separately bonded out)
    err = bme280_read_hum_raw(i2c, cfg, &hum_raw);
    if (err == ENONE) if (hum_raw == 0 || hum_raw == 0xFFFF) *result = BMX280_HUM_BW_OR_MEMS_DEFECT;

    // Implausible temperature check
    // The pressure and temperature values read out previously are compensated using the read out
    // compensation parameters. The compensated temperature and pressure is compared against
    // plausibility limits set in bme280_selftest.h, which must be set to match the customer production
    // environment. Please use the the plausibility limits as described in chapter 3.
    err = bmx280_read_temp(i2c, cfg, &temp);
    if (err == ENONE) if (temp/100.0 <= 0 || temp/100.0 >= 40) *result = BMX280_IMPLAUSIBLE_TEMP;

    // Implausible pressure check
    err = bmx280_read_press(i2c, cfg, &press);
    if (err == ENONE) if (press/25600.0 <= 900 || press/25600.0 >= 1100) *result = BMX280_IMPLAUSIBLE_PRESS;

    // Implausible humidity check
    if (device_id == BME280_REG_ID_VAL) {
        err = bme280_read_hum(i2c, cfg, &hum);
        if (err == ENONE) if (hum/1024.0 <= 20 || hum/1024.0 >= 80) *result = BMX280_IMPLAUSIBLE_HUM;
    }
    
    err = bmx280_sw_reset(i2c); 

    return err;
}

nrf_err_t bmx280_read_measurements(const struct i2c_dt_spec * i2c, bmx280_config_t * cfg, int32_t * temp, uint32_t * press, uint32_t * hum) {

    // to do another night... figure out how to shorten reads if device doesnt need to use all 3 sensors
    //If a measurement channel is disabled, the corresponding output pointer is ignored and left unmodified.
    // Not sure i want to allow measuring press / hum with temp disabled, but currently this aenables that....

    nrf_err_t err = ENONE;
    bmx280_mode_t mode = cfg->mode;
    bmx280_dev_t dev = cfg->dev;
    //bmx280_status_t status = BMX280_MEASURING;
    uint8_t raw[8]; 
    int32_t temp_raw;
    int32_t press_raw;
    int32_t hum_raw;

    float t_meas = 1.25f;
    if (cfg->osrs_temp)
        t_meas += 2.3f * (1 << (cfg->osrs_temp - 1));

    if (cfg->osrs_press)
        t_meas += 2.3f * (1 << (cfg->osrs_press - 1)) + 0.575f;

    if (cfg->osrs_hum)
        t_meas += 2.3f * (1 << (cfg->osrs_hum - 1)) + 0.575f;

    int t_meas_ms = (int)ceilf(t_meas); // Really would rather avoid any floating point math or ceilf if possible

    if (cfg == NULL) return -EGENERIC;
    
    //int timeout_incr_us = 10;
    //int timeout_dur_us = t_meas_ms * 1000;
    //int timeout_incr = timeout_dur_us / timeout_incr_us;

    if (temp == NULL || press == NULL || (hum == NULL && dev == BME280)) return -EGENERIC;

    if (mode == BMX280_PWR_MODE_FRC) { 
        err = bmx280_set_mode(i2c, cfg);
        if (err != ENONE) return err;

        /*while(status != BMX280_WAITING && timeout_incr < (timeout_dur_us / timeout_incr_us) && err == ENONE) {
            err = bmx280_check_status(cfg, &status);
            timeout_incr++;
            sleep_us(timeout_incr_us);
        }
        if (err != ENONE) return err;
        if (timeout_incr >= (timeout_dur_us / timeout_incr_us)) return -EGENERIC;*/
        
        //sleep_ms(t_meas_ms); // Not sure what I was doing here, maybe I just wasnt ready to implement this...
    }

    err = i2c_reg_read(i2c, BMX280_REG_PRESS_MSB, raw, 8);
    if (err != ENONE) return err;

    if (cfg->osrs_temp > 0) {
        temp_raw = BMX280_PACK_DATA_20BIT(raw[3], raw[4], raw[5]);
        *temp = bmx280_compensate_T_int32(temp_raw); // updates t_fine, used by press / hum compensation routines
    }

    if (cfg->osrs_press > 0) {
        press_raw = BMX280_PACK_DATA_20BIT(raw[0], raw[1], raw[2]);
        *press = bmx280_compensate_P_int64(press_raw);
    }

    if (dev == BME280 && cfg->osrs_hum > 0) {
        hum_raw = BME280_PACK_DATA_16BIT(raw[6], raw[7]);
        *hum = bme280_compensate_H_int32(hum_raw);
    }

    return err;
}


/*!
 * @brief This API reads the stored CRC and then compare with calculated CRC. Property of Bosch.
 *
 * @param[in] dev : Structure instance of bme280_dev.
 *
 * @return Result of API execution status
 * @retval zero -> self test success / +ve value -> warning(self test fail)
 */
bmx280_self_test_result_t bme280_crc_selftest(const struct i2c_dt_spec * i2c) {

	bmx280_self_test_result_t rslt = BMX280_OK;
	uint8_t reg_addr;
	uint8_t reg_data[64];

	uint8_t stored_crc = 0;
	uint8_t calculated_crc = 0;

	nrf_err_t err = ENONE;

	// Read stored crc value from register
	reg_addr = BME280_CRC_DATA_ADDR;

    // Old func, keeping until I can test refactored version below
	//rslt = bme280_get_regs(reg_addr, reg_data, BME280_CRC_DATA_LEN, dev);
	/*err = i2c_reg_read(i2c, BMX280_SLAVE_ADDR, reg_addr, reg_data, BME280_CRC_DATA_LEN);
	if (err != ENONE) {rslt = BMX280_COMM_ERR_OR_WRONG_DEV;}
	if (rslt == BMX280_OK) {
		stored_crc = reg_data[0];
        printf("Stored CRC value: 0x%X\n\n", stored_crc);
		// Calculated CRC value with calibration register 
		reg_addr = BME280_CRC_CALIB1_ADDR;
		//rslt = bme280_get_regs(reg_addr, &reg_data[0], BME280_CRC_CALIB1_LEN, dev);
		err = i2c_reg_read(i2c, BMX280_SLAVE_ADDR, reg_addr, &reg_data[0], BME280_CRC_CALIB1_LEN);
		if (err != ENONE) rslt = BMX280_COMM_ERR_OR_WRONG_DEV;
		if (rslt == BMX280_OK) {
			reg_addr = BME280_CRC_CALIB2_ADDR;
			//rslt = bme280_get_regs(reg_addr, &reg_data[BME280_CRC_CALIB1_LEN], BME280_CRC_CALIB2_LEN, dev);
			err = i2c_reg_read(i2c, BMX280_SLAVE_ADDR, reg_addr, &reg_data[BME280_CRC_CALIB1_LEN], BME280_CRC_CALIB2_LEN);
			if (err != ENONE) rslt = BMX280_COMM_ERR_OR_WRONG_DEV;
			if (rslt == BMX280_OK) {
				calculated_crc = crc_calculate(reg_data, BME280_CRC_CALIB1_LEN + BME280_CRC_CALIB2_LEN);
				// Validate CRC 
				if (stored_crc == calculated_crc)
					rslt = BMX280_OK;
				else
					rslt = BMX280_TRIM_DATA_OOB;
			}
		}
	}*/

    err = i2c_reg_read(i2c, reg_addr, reg_data, BME280_CRC_DATA_LEN);
	if (err != ENONE) {
        rslt = BMX280_COMM_ERR_OR_WRONG_DEV;
        return rslt;
    }

    stored_crc = reg_data[0];
    //printf("Stored CRC value: 0x%X\n\n", stored_crc);
    // Calculated CRC value with calibration register 
    reg_addr = BME280_CRC_CALIB1_ADDR;
    //rslt = bme280_get_regs(reg_addr, &reg_data[0], BME280_CRC_CALIB1_LEN, dev);
    err = i2c_reg_read(i2c, reg_addr, &reg_data[0], BME280_CRC_CALIB1_LEN);
    if (err != ENONE) {
        rslt = BMX280_COMM_ERR_OR_WRONG_DEV;
        return rslt;
    }

    reg_addr = BME280_CRC_CALIB2_ADDR;
    //rslt = bme280_get_regs(reg_addr, &reg_data[BME280_CRC_CALIB1_LEN], BME280_CRC_CALIB2_LEN, dev);
    err = i2c_reg_read(i2c, reg_addr, &reg_data[BME280_CRC_CALIB1_LEN], BME280_CRC_CALIB2_LEN);
    if (err != ENONE) {
        rslt = BMX280_COMM_ERR_OR_WRONG_DEV;
        return rslt;
    }

    calculated_crc = crc_calculate(reg_data, BME280_CRC_CALIB1_LEN + BME280_CRC_CALIB2_LEN);
    // Validate CRC 
    if (stored_crc == calculated_crc) rslt = BMX280_OK;
    else rslt = BMX280_TRIM_DATA_OOB;

	return rslt;
}

/*!
 * @brief This API calculates the CRC. Property of Bosch.
 *
 * @param[in] mem_values : reg_data parameter to calculate CRC
 * @param[in] mem_length : Parameter to calculate CRC
 *
 * @return Result of API execution status
 * @retval zero -> Success / +ve value -> Warning / -ve value -> Error
 */
static uint8_t crc_calculate(uint8_t *mem_values, uint8_t mem_length)
{
	uint32_t crc_reg = 0xFF;
	uint8_t polynomial = 0x1D;
	uint8_t bitNo, index;
	uint8_t din = 0;

	for (index = 0; index < mem_length; index++) {
		for (bitNo = 0; bitNo < 8; bitNo++) {
			if (((crc_reg & 0x80) > 0) ^ ((mem_values[index] & 0x80) > 0))
				din = 1;
			else
				din = 0;

			/* Truncate 8th bit for crc_reg and mem_values */
			crc_reg = (uint32_t)((crc_reg & 0x7F) << 1);

			/* crc_calculate() modifies reg_data in-place */
			mem_values[index] = (uint8_t)((mem_values[index] & 0x7F) << 1);
			crc_reg = (uint32_t)(crc_reg ^ (polynomial * din));
		}
	}

	return (uint8_t)(crc_reg ^ 0xFF);
}
