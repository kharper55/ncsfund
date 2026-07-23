#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "ncs_version.h"
#if NCS_VERSION_NUMBER < 0x20600
#include <zephyr/random/rand32.h>
#else 
#include <zephyr/random/random.h>
#endif
#include <string.h>

#define STACKSIZE 1024
#define PRODUCER_PRIORITY        5 
#define CONSUMER_PRIORITY        5

volatile uint32_t available_instance_count = 10; // not important to know what the resource is in this exercise (it can be anything from printers, fax, peripherals, clock), just assume that we have 10 instances of that resource

K_SEM_DEFINE(instance_monitor_sem, 10, 10); // Add a semaphore by first defining the semaphore using K_SEM_DEFINE()

void release_access(void) {
        available_instance_count++;
        printk("Resource given and available_instance_count = %d\n", available_instance_count);
}

void get_access(void) {
        available_instance_count--;
        printk("Resource taken and available_instance_count = %d\n",  available_instance_count);
}

void producer(void) {
        printk("Producer thread started\n");
	while (1) {
		//release_access();
                k_sem_give(&instance_monitor_sem);
	        printk("Resource given and available_instance_count = %d\n", k_sem_count_get(&instance_monitor_sem));
		k_msleep(sys_rand32_get() % 10); // sleeps for a random (0-9ms) amount of time
	}
}

void consumer(void) {
        printk("Consumer thread started\n");
	while (1) {
		//get_access();
                k_sem_take(&instance_monitor_sem, K_FOREVER); // Get semaphore when getting access to the resource
                printk("Resource taken and available_instance_count = %d\n", k_sem_count_get(&instance_monitor_sem));
		k_msleep(sys_rand32_get() % 10);
	}
}

K_THREAD_DEFINE(producer_id, STACKSIZE, producer, NULL, NULL, NULL, PRODUCER_PRIORITY, 0, 0);
K_THREAD_DEFINE(consumer_id, STACKSIZE, consumer, NULL, NULL, NULL, CONSUMER_PRIORITY, 0, 0);