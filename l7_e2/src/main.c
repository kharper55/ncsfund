#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACKSIZE 1024  // Even though the threads are simple in this exercise, we are setting a stack size of 1024. Stack sizes should always be a power of two (512, 1024, 2048, etc.).
#define THREAD0_PRIORITY 6
#define THREAD1_PRIORITY 7

// without yielding in thread0, thread1 will be starved
// without yielding in thread1, thread0 will print once, then thread0 will be starved since thread1 never yields

void thread0(void) { // thread entry function for thread 0
        while(1) {
                printk("Hello, I am thread0\n");

                // Causes the current thread to give away execution (yield) to another thread of the same or higher priority. The thread will be in the “Runnable” state, just pushed to the end of the list of “Runnable” threads. 
                // To give lower priority threads a chance to run, the current thread needs to be put to “Non-runnable”. This can be done using k_sleep().
                //k_yield();
                //k_msleep(5);
                k_busy_wait(1000000); // This wont place thread into runnable state, i.e. it wont let the other thread run without timeslicing enabled
        }
}

void thread1(void) { // thread entry function for thread 1
        while(1) {
                printk("Hello, I am thread1\n");
                //k_yield();
                //k_msleep(5);
                k_busy_wait(1000000);
        }
}

// Since the threads have no dependency on each other and neither yield nor sleep, they will always be in the “Runnable” state competing for the CPU resource.

K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL, THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL, THREAD1_PRIORITY, 0, 0); // delay of 0 means thread will be created and put in the ready state right away
