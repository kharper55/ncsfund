#include <zephyr/kernel.h>     // for zephyr kernel
#include <zephyr/sys/printk.h> // kernel print function

// NOTE
// prj.conf is left empty because the console drivers needed by printk() are enabled by default by the board configuration file. This is covered in-depth in Lesson 4.

// NOTE
// Default speed is set in devicetree file to 115200 baud

// To use the nrfConnect SDK built in terminal, go to connected devices section in extension panel, click the device drop down, and connect to VCOM with the plug icon. Select 115200 8n1 rtscts off

int main(void)
{
 while (1) {
 printk("Hello World!\n");
 k_msleep(1000);
 }
}