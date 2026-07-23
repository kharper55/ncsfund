#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACKSIZE 1024 

#define THREAD0_PRIORITY 2 
#define THREAD1_PRIORITY 3
#define WORKQ_PRIORITY   4 // The workqueue thread should have the lowest priority since we want this thread to execute offloaded (non-urgent) work. Remember that high priority translates to lower numerical value.

#define WORQ_THREAD_STACK_SIZE 512

// Define stack area used by workqueue thread
static K_THREAD_STACK_DEFINE(my_stack_area, WORQ_THREAD_STACK_SIZE);

// Define queue structure
static struct k_work_q offload_work_q = {0};

static inline void emulate_work() {
        // This function should take about ~24 ms to finish on a nRF54L15 SoC running at 128 MHz. On a 64 MHz nRF52840 SoC it should take double that time.
	for(volatile int count_out = 0; count_out < 300000; count_out ++);
}

struct work_info {
    struct k_work work;
    char name[25];
} my_work;

void offload_function(struct k_work *work_term) {
	emulate_work();
}

void thread0(void) { // thread entry function for thread 0

        uint64_t time_stamp;
        int64_t delta_time;

        k_work_queue_start(&offload_work_q, my_stack_area,
                   K_THREAD_STACK_SIZEOF(my_stack_area), WORKQ_PRIORITY,
                   NULL); // start the workqueue

        strcpy(my_work.name, "Thread0 emulate_work()");
        k_work_init(&my_work.work, offload_function); // initialize the work item to connect the work item to its handler 

        while(1) {
                
                time_stamp = k_uptime_get();
                //emulate_work();
                k_work_submit_to_queue(&offload_work_q, &my_work.work); // submit a work item to the workqueue
                delta_time = k_uptime_delta(&time_stamp);

                // now offloading the processing of emulate_work() into the lower priority worker thread which means that it should 
                // process less in this high priority context before it goes to sleep (for 20 ms). 
                // This, in turn, should translate to more processing time for thread1 (by fewer interruptions from thread0).
                printk("thread0 yielding this round in %lld ms\n", delta_time);

                // To give lower priority threads a chance to run, the current thread needs to be put to “Non-runnable”. This can be done using k_sleep().
                k_msleep(20); // Sleep the thread
        }
}

void thread1(void) { // thread entry function for thread 1
        uint64_t time_stamp;
        int64_t delta_time;

        while(1) {
                
                time_stamp = k_uptime_get();
                emulate_work();
                delta_time = k_uptime_delta(&time_stamp);

                printk("thread1 yielding this round in %lld ms\n", delta_time);
                k_msleep(20);
        }
}

K_THREAD_DEFINE(thread0_id, STACKSIZE, thread0, NULL, NULL, NULL, THREAD0_PRIORITY, 0, 0);
K_THREAD_DEFINE(thread1_id, STACKSIZE, thread1, NULL, NULL, NULL, THREAD1_PRIORITY, 0, 0); // delay of 0 means thread will be created and put in the ready state right away