#include "zephyr/kernel.h"
#include <stddef.h>

extern const unsigned char tic_demo_data[];
extern unsigned int tic_demo_data_len;
extern struct ring_buf uart_rx_ringbuf;
extern struct k_sem rx_data_sem;

void tic_demo_thread(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(tic_demo_thread_id, 1024, tic_demo_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, 0);

#define DATARATE 100
#define SLEEP_MS 100

void tic_demo_thread(void *p1, void *p2, void *p3)
{
	uint32_t cursor = 0;

	for (;;) {
		uint32_t to_send = (DATARATE * SLEEP_MS) / 1000;

		size_t remaining = tic_demo_data_len - cursor;
		if (remaining >= to_send) {
			ring_buf_put(&uart_rx_ringbuf, &tic_demo_data[cursor], to_send);
			cursor += to_send;
		} else {
			ring_buf_put(&uart_rx_ringbuf, &tic_demo_data[cursor], remaining);
			to_send -= remaining;
			ring_buf_put(&uart_rx_ringbuf, &tic_demo_data[0], to_send);
			cursor = to_send;
		}

        k_sem_give(&rx_data_sem);

		k_sleep(K_MSEC(SLEEP_MS));
	}
}
