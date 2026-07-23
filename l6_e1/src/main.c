#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h> // <-- Required for log_process()
#include "app_i2c.h"
#include "bmx280.h"

#define SLEEP_TIME_MS 100

// Note:
// Pinout of pro micro is defined here: C:\ncs\v3.3.0\zephyr\boards\others\promicro_nrf52840\promicro_nrf52840-pinctrl.dtsi
// I2C0 SDA is pin 1.00, SCL is pin 0.11
// Wire a BME280 sensor to the I2C0 pins for this example

// Didn't have the dev board they wanted for either example in this section, so I went a bit overkill and just ported my RPI BMX280 API
// For use with Zephyr... Just had to modify all the API functions to work with the Zephyr implementation of i2c driver

#define I2C0_NODE DT_NODELABEL(bme280) // This is why I can pass dev_i2c to all BMX280 API functions and not have to worry about the slave address
static const struct i2c_dt_spec dev_i2c = I2C_DT_SPEC_GET(I2C0_NODE);

LOG_MODULE_REGISTER(l6_e1, LOG_LEVEL_DBG); // max log level of LOG_LEVEL_DBG makes all log levels available, including LOG_INF, LOG_ERR, LOG_WRN, LOG_DBG

/*
Module registration can be skipped in two cases:

- The module consists of more than one file, and another file invokes this macro. (LOG_MODULE_DECLARE() should be used instead in all of the 
module's other files.) 
- Instance logging is used and there is no need to create module entry. In that case LOG_LEVEL_SET() should be used to set log level used within 
the file.
*/

int main(void) {

        int32_t temp;
        uint32_t press;
        uint32_t hum;
        bmx280_osrs_t temp_osrs = BMX280_OVERSAMP_X1;
        bmx280_osrs_t press_osrs = BMX280_SKIP_MEAS;
        bmx280_osrs_t hum_osrs = press_osrs;

        const extern bmx280_config_t bmx280_indoor_nav_cfg;
        const extern bmx280_config_t bmx280_weather_mon_cfg;        

        //bmx280_config_t myCfg = {BMX280_PWR_MODE_NORM, BMX280_TSTDBY_0_5MS, BMX280_FILT_OFF, temp_osrs, press_osrs, hum_osrs};
        //bmx280_config_t myCfg = bmx280_indoor_nav_cfg; // Works fine
        bmx280_config_t myCfg = bmx280_weather_mon_cfg;

        k_msleep(2000); // Give time to reconnect to USB VCOM after reset

        LOG_INF("NRF52840 setup complete\n");

        /*
        Interesting behavior observed
        With full LOG module, lots of messages sent quickly are thrown out (i.e. for the BME280 startup sequence, reporting out all the trim reg values)
        When this occurs, a LOG ERR is printed and reports that XX messages were thrown out (where XX is some number)
        This only occurs for the rapid fire logs right at startup, the 100ms separated logs in the main loop all report fine...

        With minimal LOG module (CONFIG_LOG_MODE_MINIMAL=y in prj.conf), the LOGS all show up during startup sequence
        This may be because log minimal adjusts the rate limit? Although the KCONFIG GUI doesnt show any difference...

        ^^ Actually, its because LOG_MINIMAL implementation does not defer the console prints, which is not what I want
        Instead I could allow new logs to overwrite old ones in the ring buffer or expand the memory allocation for the ring buffer
        Still not perfect, not really sure what would fix it, but not an issue for now.
        */

        bmx280_self_test_result_t result = BMX280_OK;
        bmx280_self_test(&dev_i2c, &myCfg, &result);
        LOG_INF("Err %d: BMX280 self test %s.", result, result == BMX280_OK ? "PASSED" : "FAILED");
        //if (*result == BMX280_COMM_ERR_OR_WRONG_DEV) printf("Cannot access register 0x%X for CRC check.\n\n", BME280_CRC_DATA_ADDR);
        //else printf("\n");

        k_msleep(2);

        bmx280_init(&dev_i2c, &myCfg, false);

        k_msleep(100);

        while(1) {

                bmx280_read_measurements(&dev_i2c, &myCfg, &temp, &press, &hum);

                // Note: looks like log automatically applies a newline
                // Not sure how to disable this beyond using LOG_RAW() or LOG_PRINTK() or setting kconfig option for minimal implementation (CONFIG_LOG_MODE_MINIMAL=y)... not my fav but also was just curious
                LOG_INF("Pressure (Pa): %.2f Temperature (°F): %.2f Humidity (%%): %.2f", press/256.0, C_2_F(temp/100.0), hum/1024.0);
                k_msleep(SLEEP_TIME_MS);
        }

        return 0;
}
