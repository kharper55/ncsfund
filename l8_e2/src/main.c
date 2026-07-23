#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "ncs_version.h"
#if NCS_VERSION_NUMBER < 0x20600
#include <zephyr/random/rand32.h>
#else 
#include <zephyr/random/random.h>
#endif
#include <string.h>

#define THREAD0_STACKSIZE 1024
#define THREAD1_STACKSIZE 1024

#define THREAD0_PRIORITY        4 
#define THREAD1_PRIORITY        4

#define COMBINED_TOTAL   40
int32_t increment_count = 0; 
int32_t decrement_count = COMBINED_TOTAL; 

K_MUTEX_DEFINE(test_mutex);

void shared_code_section(void) {
        uint8_t race_condition = 0;
        int32_t increment_count_copy = 0;
        int32_t decrement_count_copy = 0;

        /* Critical Section */

        k_mutex_lock(&test_mutex, K_FOREVER);

        increment_count += 1;
        increment_count = increment_count % COMBINED_TOTAL; 

        decrement_count -= 1;
        if (decrement_count == 0) {
                decrement_count = COMBINED_TOTAL;
        }

        /*if (increment_count + decrement_count != COMBINED_TOTAL) {
                race_condition = 1;
        }*/

        // Checking the value still needs to be protected by the mutex, as the value of the counters can be changed by another task otherwise.
        if (increment_count + decrement_count != COMBINED_TOTAL) {
                race_condition = 1;
                increment_count_copy = increment_count;
                decrement_count_copy = decrement_count;
        }

        k_mutex_unlock(&test_mutex);

        if( race_condition ){
                printk("Race condition happend!\n");
                printk("Increment_count (%d) + Decrement_count (%d) = %d \n", increment_count_copy,
                decrement_count_copy, (increment_count_copy + decrement_count_copy));
                k_msleep(400 + sys_rand32_get() % 10);
        }
        
        /*if( race_condition ){
                printk("Race condition happend!\n");
                printk("Increment_count (%d) + Decrement_count (%d) = %d \n", increment_count, decrement_count, (increment_count + decrement_count));
                k_msleep(400 + sys_rand32_get() % 10);
        }*/
}

void thread0(void)
{
	printk("Thread 0 started\n");
	while (1) {
		shared_code_section(); // When only one thread accesses this function, without a mutex, no race condition occurs. When both 
	}
}

void thread1(void)
{
	printk("Thread 1 started\n");
	while (1) {
		shared_code_section(); 
	}
}

// Define and initialize threads
K_THREAD_DEFINE(thread0_id, THREAD0_STACKSIZE, thread0, NULL, NULL, NULL, THREAD0_PRIORITY, 0, 5000);
K_THREAD_DEFINE(thread1_id, THREAD1_STACKSIZE, thread1, NULL, NULL, NULL, THREAD1_PRIORITY, 0, 5000);