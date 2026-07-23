#include "app_i2c.h"

/**
 * Initialize an I2C peripheral for use as a master/controller.
 * Note that the specified SDA and SCL pins must support the selected I2C
 * peripheral instance and no error checking is performed to ensure this.
 * @param[in]  i2c        I2C peripheral instance.
 * @param[in]  scl        Pin number for SCL pin.
 * @param[in]  sda        Pin number for SDA pin.
 * @param[in]  fclk       Bit rate for I2C transmissions. 
 * @param[in]  puen       Enable internal pullups.
 * 
 * @return None
 */   

/* void app_i2c_init(i2c_dt_spec * i2c, const uint8_t scl, const uint8_t sda, const uint32_t fclk, const bool puen) {

    int err = PICO_ERROR_NONE;

    i2c_init(i2c, fclk); // 400kHz

    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);

    if (puen) {
        gpio_pull_up(sda);
        gpio_pull_up(scl); 
    }
}*/

/**
 * Read a number of registers from an I2C slave. 
 * @param[in]  i2c        I2C peripheral instance.
 * @param[in]  reg_addr   Register address to begin reading from.
 * @param[out] buff       Destination buffer for received data. Must be at least as large as nbytes.
 * @param[in]  nbytes     Number of bytes to read. Use a number > 1 for burst reads.
 * 
 * @return ENONE on success, -EIO on failure.
 */  
nrf_err_t i2c_reg_read(const struct i2c_dt_spec * i2c, const uint8_t reg_addr, uint8_t * buff, const size_t nbytes) {

    int ret = ENONE;

    if (buff == NULL || nbytes == 0) return -EGENERIC;

    //int ret = i2c_write_blocking(i2c, slave_addr, &reg_addr, 1, true); // this is the handle for rpi pico sdk
    //if (ret != 1) return PICO_ERROR_GENERIC;

    //ret = i2c_read_blocking(i2c, slave_addr, buff, nbytes, false); // this is the handle for rpi pico sdk

    switch(nbytes) {
        // 0 case is handled by the check at the top of the function
        case 1:
            // Returns a negative value on error (-EIO General input / output error), returns 0 on success
            ret = i2c_reg_read_byte_dt(i2c, reg_addr, buff);
            break;
        default:
            ret = i2c_burst_read_dt(i2c, reg_addr, buff, nbytes);
            break;
    }

    //ret = i2c_burst_read_dt(i2c, reg_addr, buff, nbytes);
    //if (ret != (int)nbytes) return PICO_ERROR_GENERIC;
    
    return ret;
}

/**
 * Write to a number of registers on an I2C slave.
 * @param[in]  i2c        I2C peripheral instance.
 * @param[in]  reg_addr   Register address to begin writing to.
 * @param[in]  data       Data to transmit. Must be at least as large as nbytes.
 * @param[in]  nbytes     Number of bytes to write. Use a number > 1 for burst writes.
 * 
 * @return ENONE on success, -EIO on failure.
 */  
nrf_err_t i2c_reg_write(const struct i2c_dt_spec * i2c, const uint8_t reg_addr, const uint8_t * data, const size_t nbytes) {

    // Does it actually make sense to pass nbytes here? When would we write multiple bytes?
    
    if (nbytes > I2C_BUFF_SIZE_MAX || data == NULL || nbytes == 0) return -EGENERIC;

    uint8_t src[nbytes + 1]; // VLA used to avoid a fixed-size temporary buffer

    src[0] = reg_addr;
    memcpy(&src[1], data, nbytes);

    //int ret = i2c_write_blocking(i2c, slave_addr, src, nbytes + 1, false);
    //if (ret != (int)(nbytes + 1)) return PICO_ERROR_GENERIC;

    // Returns a negative value on error (-EIO General input / output error), returns 0 on success
    int ret = i2c_write_dt(i2c, src, nbytes + 1);
    
    return ret;
}