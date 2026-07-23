**nRF Connect SDK Fundamentals Course w/ nicenano clone**

This is a collection of all lessons completed following the nRF Connect SDK Fundamentals Course hosted by Nordic Semiconductor. The complete course can be found here: https://academy.nordicsemi.com/courses/nrf-connect-sdk-fundamentals/

The intent was to gain familiarity with the nRF52840 SoC, learn about Zephyr RTOS, and reinstill RTOS fundamentals (previous experience with FreeRTOS using ESP-IDF). These learnings will support future custom development with nRF SoCs using the nRF Connect SDK and nRF52840 device in embedded battery-powered BLE devices. Using hardware which was not specifically intended for the tutorial further benefitted my understanding as I had to port/adapt all example code for my specific hardware. This forced me to dive deeper into devicetree structure/syntax and Kconfig. Next up is the intermediate course as well as BLE fundamentals, after which I plan to focus on software + hardware development in parallel.

The course was completed using ncs v3.3.0 using an nRF Pro Micro clone, such as the following: https://www.amazon.com/dp/B0CYLNZ6V4?ref=ppx_yo2ov_dt_b_fed_asin_title

More details about 3rd party nRF boards and clones can be found here:
https://www.nologo.tech/en/product/otherboard/NRF52840.html
https://github.com/joric/nrfmicro/wiki/Alternatives

Naturally, since the course is intended for use with the nRF52840 development kit by Nordic, some deviations to the example code were necessary.

Note that for all examples, sysbuild was not used as I ran into an issue with flashing the device when building with sysbuild enabled. I've yet to double back to rectify/understand the issue, but ultimately I am not worried about replacing the bootloader or building applications for multi-core SoCs so I opted not to use sysbuild. Also note that since the Pro Micro clone does not have an on-board USB-serial bridge, all console interaction (with exception noted below) employ the use of the USB-CDC backend, which is configured by default in the devicetree file for the board. This has implications for implementing the asynchronous UART example in lesson 5.

Notable deviations to the examples throughout the fundamentals course are noted below:
- Asynch UART (l5_e1): For this lesson, I used a Zephyr overlay and modified the Kconfig settings to force the use of the UART backend, routing all serial communication to two GPIO pins. Note that the USB-CDC-ACM implementation in ncs 3.3.0 is incompatible with the asynchronous UART drivers. A DSD-TECH SH-V09C5 was employed to act as a USB-serial converter and complete the loop.
- I2C (l6_e1): For this lesson, given that I was using the nRF Pro Micro (clone), I used an external BME280 sensor breakout which I happened to have on-hand. Since the 2nd exercise in this lesson did not provide much value in my opinion (it doesn't introduce any new concepts, but employs the use of an ambient light detection sensor via I2C) I opted to take the example a bit further on the 1st exercise. All I really did was port over a barebones driver for the BME280 which I wrote originally for the Raspbery Pi Pico device. This only involved modifying the handles and calls which use the I2C driver supplied by nRF. Further validations are necessary to test this driver code, but it will immediately support bring-up on at least one embedded project I am working on.
