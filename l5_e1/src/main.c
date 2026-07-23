/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
/* STEP 3 - Include the header file of the UART driver in main.c */
#include <zephyr/drivers/uart.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   10

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#define RECEIVE_BUFF_SIZE 10 // Define the size of the receive buffer.
static uint8_t rx_buf[RECEIVE_BUFF_SIZE] = {0}; // Define the receive buffer and initialize its members to zeros.

#define RECEIVE_TIMEOUT 100 // Define the receiving timeout period to be 100 microseconds. This means when a byte is received and there is inactivity (no new data) for 100 microseconds, a UART_RX_RDY event is generated. If you are going to use receive timeout in your application, pick a value that matches your application requirements.

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
//static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0)); // Some drivers in Zephyr have API-specific structures and calls that encapsulate all the information needed to control the device in one structure. The UART driver does _not_ have this, so we will use the macro call `DEVICE_DT_GET()`

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {

	// For this application, we are only interested in the UART_RX_RDY and the UART_RX_DISABLED events related to receiving data over UART

		case UART_RX_RDY:
		/*
		With if((evt->data.rx.len) == 1), we are assuming a terminal in char mode, which is the default terminal in nRF Connect for VS Code (nRF Terminal) and PuTTY. 
		These two terminal by default, does not require you to press enter to send the data, hence do not send extra bytes.
		On the other hand,  if you are using a serial terminal in “line mode”, you need to press enter to send the data, which typically sends two extra bytes “/r/n” 
		(Depending on the terminal configuration). Therefore you need to change the condition to match the length of the data you are sending.
		*/
			if ((evt->data.rx.len) == 1) {

				if (evt->data.rx.buf[evt->data.rx.offset] == '1') {
					gpio_pin_toggle_dt(&led);
				} 
				// These are used for dk's with multiple LEDs, thats not me
				/*else if (evt->data.rx.buf[evt->data.rx.offset] == '2') {
					gpio_pin_toggle_dt(&led1);
				} else if (evt->data.rx.buf[evt->data.rx.offset] == '3') {
					gpio_pin_toggle_dt(&led2);
				}*/
			}
			break;

		case UART_RX_DISABLED:
			uart_rx_enable(dev, rx_buf, sizeof rx_buf, RECEIVE_TIMEOUT);
			break;

		default:
			break;
	}
}

/*
https://forum.seeedstudio.com/t/async-uart-configuration-error/283194/5
&
https://devzone.nordicsemi.com/f/nordic-q-a/106129/cannot-get-async-uart0-to-work-with-usb-cdc

https://devzone.nordicsemi.com/f/nordic-q-a/110322/device-tree-proj-conf-and-strange-uart-behaviour

Note that the Async API is incompatible with the NRF52840 USB stack

Should try to reimplement using the interrupt-driven API instead of the async API. The interrupt-driven API is compatible with the USB stack, but it does not support timeouts. If you need to use timeouts, you will have to implement them yourself using a timer or a work queue.
Or, reassign UART0 to use physical tx/rx pins instead of the USB CDC interface. This will allow you to use the Async API without any issues.

Did the latter with USBA-UART FTDI FT232 USB-UART bridge and pins p0.08 and p0.06. Had to edit prj.conf to manipulate console/uart config 
and add an overlay to change uart0 pin routing and update the chosen node w/ regard to console selection (use uart0 node instead of USB CDC ACM node).
*/

static uint8_t tx_buf[] = {"nRF Connect SDK Fundamentals Course\r\nPress 1 on your keyboard to toggle the LED on the NRF52840 Pro Micro clone\r\n"};

int main(void)
{
	int ret;

	k_msleep(2000);

	// Check if UART device is ready
	if (!device_is_ready(uart)){
		printk("UART device not ready\r\n");
		return 1 ;
	}

	// Check if GPIO device is ready. This method checks the port of the GPIO device, which is the recommended way to check if a GPIO device is ready. The gpio_is_ready_dt() function checks if the GPIO device is ready based on the devicetree specification.
	if (!device_is_ready(led.port)){
		printk("GPIO device is not ready\r\n");
		return 1;
	}
	
	// Alternative way to check if GPIO device is ready using the gpio_is_ready_dt() function. This function checks if the GPIO device is ready based on the devicetree specification.
	// This is a convenience wrapper designed specifically for gpio_dt_spec.

	/* Internally it is basically:

	static inline bool gpio_is_ready_dt(const struct gpio_dt_spec *spec)
	{
     	return device_is_ready(spec->port);
	}
	So for this particular check, they are functionally identical. */

	//if (!gpio_is_ready_dt(&led)) {
	//	printk("GPIO device is not ready\r\n");
	//	return 0;
	//}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Configured GPIO\r\n");
		return 0;
	}

	// Register the UART callback function
	ret = uart_callback_set(uart, uart_cb, NULL);
	if (ret) {
		if (ret == -ENOTSUP) {
			printk("UART callback not supported\r\n");
		} else if (ret == -ENOSYS) {
			printk("UART callback not implemented\r\n");
		} else {
			printk("UART callback set failed, err = %d\r\n", ret);
		}
		return 1;
	}

	// Since we are not using flow control, pass SYS_FOREVER_US to disable transmission timeout. This is not related to receiving timeout.
	ret = uart_tx(uart, tx_buf, sizeof(tx_buf), SYS_FOREVER_US);
	if (ret) {
		printk("TX failed %d\n", ret);
		return 1;
	}

	// Start receiving by calling uart_rx_enable() and pass it the address of the receive buffer.
	ret = uart_rx_enable(uart ,rx_buf,sizeof rx_buf,RECEIVE_TIMEOUT);
	if (ret) {
		printk("RX failed %d\n", ret);
		return 1;
	}
	
	while (1) {	
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
