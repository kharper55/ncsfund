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

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   10

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define SW0_NODE DT_ALIAS(sw0)

#define MAX_NUMBER_FACT 10

bool button_flag = false;

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec sw = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

void pin_isr(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins) {
	gpio_pin_toggle_dt(&led);
	button_flag = true;
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
	//long int factorial = 1;

	//bool led_state = true;
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

	k_msleep(3000); // wait for 3 seconds to allow serial emulator to connect

	printk("nRF Connect SDK Fundamentals - Lesson 4 - Exercise 1\n");

	while (1) {

		if (button_flag) {
			button_flag = false; // Reset button flag
			//factorial = 1;		 // Reset factorial
			printk("Button pressed!\n");
			for (int i = 1; i <= MAX_NUMBER_FACT; i++) {
				//factorial = factorial * i;
				printk("%d! = %ld\n", i, factorial(i));
			}
			printk("_______________________________________________________\n");
		}

		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
