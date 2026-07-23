/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Same as l4_e1 but using logger module instead
// The timestamping feature (LOG_BACKEND_FORMAT_TIMESTAMP) and the coloring of error and warning logs feature (LOG_BACKEND_SHOW_COLOR) are enabled
// by default. The logger gets the timestamp by internally calling the kernel function k_cycle_get_32().
// This routine returns the current time since bootup (uptime), as measured by the system’s hardware clock. 
// You could change this to return an actual time and date if an external Real-time clock is present on the system.

// Ensure to enable the logger module. This is done by adding the configuration line to the application configuration file prj.conf.

//#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>  // Replaces printk.h include

/*
Oddly, the following error occurs on bootup
[00:00:00.267,333] <inf> udc_nrf: Preinit
*** Booting nRF Connect SDK v3.3.0-ba167d9f3db4 ***
*** Using Zephyr OS v4.3.99-fd9204a02d52 ***
[00:00:00.267,608] <inf> udc_nrf: Initialized
[00:00:00.271,881] <inf> udc_nrf: SUSPEND state detected
[00:00:00.443,756] <inf> udc_nrf: Reset
[00:00:00.443,817] <inf> udc_nrf: RESUMING from suspend
[00:00:00.493,377] <inf> udc_nrf: Reset
[00:00:00.577,423] <err> udc: Failed to allocate net_buf 4095, ep 0x80
[00:00:00.619,049] <err> udc: Failed to allocate net_buf 4095, ep 0x80

The initial information logs are shown on my setup and not the nrf exercise since I use the USB CDC ACM serial emulator to view the logs. 

The error is harmless (logging continues working just fine)
https://github.com/zephyrproject-rtos/zephyr/issues/85108
https://devzone.nordicsemi.com/f/nordic-q-a/126783/udc-and-usbd_ch9-error-appears-with-hid-keyboard-example

Could not get rid of it even with unplugging all USB devices and directly plugging into the PC

Trying on Rpi, the issue is not reproduced.
So voila, its a Windows thing. This is an artifact of differences in how the host OS enumerates USB devices
Not going to look into it further, as it is not a showstopper. The logging works just fine, and the error is harmless.
*/

// ========================================================================================================================================

// When the logger module is enabled by setting the global configuration symbol CONFIG_LOG=y, 
// the following logger configurations are enabled by default:

/*
LOG_MODE_DEFERRED	Deferred mode is used by default. Log messages are buffered and processed later. This mode has the least impact on the application. Time-consuming processing is deferred to the known context.
LOG_PROCESS_THREAD 	A thread is created by the logger subsystem (deferred mode). This thread is woken up periodically (LOG_PROCESS_THREAD_SLEEP_MS) or whenever the number of buffered messages exceeds the threshold (LOG_PROCESS_TRIGGER_THR).
LOG_BACKEND_UART	Send logs to the UART console.
LOG_BACKEND_SHOW_COLOR	Prints errors in red and warnings in yellow. Not all terminal emulators support this feature.
LOG_BACKEND_FORMAT_TIMESTAMP	Timestamp is formatted to hh:mm:ss.ms,us.
LOG_MODE_OVERFLOW	If there is no space to log a new message, the oldest one is dropped.

Also,
CONFIG_LOG_MODE_MINIMAL enables a minimal logging implementation.
This is very useful when you have limited memory, and are not in need of the deferred feature of the logger. 
It has very little memory footprint overhead on top of the printk() implementation for standard logging macros.
By default, the logging module is significantly heavier than printk(), but it also provides much more functionality.
*/

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   10

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define SW0_NODE DT_ALIAS(sw0)

#define MAX_NUMBER_FACT 10

volatile bool button_flag = false;

LOG_MODULE_REGISTER(l4_e2, LOG_LEVEL_DBG);

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

void pin_isr(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins) {
	gpio_pin_toggle_dt(&led);
	button_flag = true;

	/*Important note!
	Code in ISR runs at a high priority, therefore, it should be written with timing in mind.
	Too lengthy or too complex tasks should not be performed by an ISR, they should be deferred
	to a thread.
	*/
}

long int factorial(int n) {
	if (n == 0 || n == 1) {
		return 1;
	} else {
		return n * factorial(n - 1);
	}
}	

int main(void)
{
	int ret;
	static struct gpio_callback pin_cb_data;

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	if (!gpio_is_ready_dt(&sw)) {
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&sw, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	gpio_init_callback(&pin_cb_data, pin_isr, BIT(sw.pin));
	
	gpio_add_callback(sw.port, &pin_cb_data);

	// In order to use the logger, you must register with it first. This is done by using the macro LOG_MODULE_REGISTER()  which takes two parameters:
    // A mandatory module name. The module name will be included in every log entry, which helps to distinguish messages generated by different modules and/or filtering. The module name must NOT be passed as a string. In other words, don’t use quotation marks “”.  
    // An optional maximal log level for the module. The maximal log level will determine which messages will be sent to the console. For instance, if you set the log level of module X to  LOG_LEVEL_DBG, it means all messages generated (Debug, Info, Warning, and Error) will be send to the console. On the other hand, if you set the maximal log level of module Y to LOG_LEVEL_WRN, it means only messages with a severity level of warning and error will be sent to the console. If the minimum log level is not provided, then default global log level (CONFIG_LOG_DEFAULT_LEVEL) is used in the file. The default global log level is set to LOG_LEVEL_INF.

	k_msleep(2000); // wait for 2 seconds to allow serial emulator to connect

	printk("nRF Connect SDK Fundamentals - Lesson 4 - Exercise 2\n");

	int exercise_num = 2;
	uint8_t data[] = {0x00, 0x01, 0x02, 0x03,
					0x04, 0x05, 0x06, 0x07,
					'H', 'e', 'l', 'l','o'};
	// Printf-like messages
	LOG_INF("nRF Connect SDK Fundamentals");
	LOG_INF("Exercise %d",exercise_num);    
	LOG_DBG("A log message in debug level");
	LOG_WRN("A log message in warning level!");
	LOG_ERR("A log message in Error level!");
	// Hexdump some data
	LOG_HEXDUMP_INF(data, sizeof(data),"Sample Data!"); 

	while (1) {

		if (button_flag) {
			button_flag = false; // Reset button flag
			LOG_INF("Button pressed!\n");
			for (int i = 1; i <= MAX_NUMBER_FACT; i++) {
				//factorial = factorial * i;
				LOG_INF("%d! = %ld\n", i, factorial(i));
			}
			LOG_INF("_______________________________________________________\n");
		}

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
