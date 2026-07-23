/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>		 // For kernel functions, such as k_msleep()
#include <zephyr/drivers/gpio.h> // nRF GPIO device driver API
#include <zephyr/drivers/uart.h> // nRF UART device driver API

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   25

/* The devicetree node identifier for the "led0" alias. */
//#define LED0_NODE DT_ALIAS(led0) // Get the node identifier from the nodes alias
#define LED0_NODE DT_NODELABEL(led0) 

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = true;

	// This line verifies that the device is ready for use, i.e. in a
	// state so that it can be used with its standard API. See lesson 2 notes
	// The generic form of this is device_is_ready() 
	if (!gpio_is_ready_dt(&led)) { 
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		//ret = gpio_pin_toggle_dt(&led);
		ret = gpio_pin_set_dt(&led, led_state);
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
