/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "zephyr/device.h"
#include "zephyr/sys/printk.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

#define UART_DEVICE_NODE DT_NODELABEL(uart1)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

int main(void)
{
	int ret;
	printk("Hello World\n");

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!\n");
		return 0;
	}

	uint32_t w = 0;
	static unsigned char buf[256];
	
	for (;;) {
		bool flush = false;

		char *s = &buf[w];
		ret = uart_poll_in(uart_dev, s);
		if (ret == 0) {
			
			(*s) &= 0x7F; // 7 bits only
			w++;
			if (w >= sizeof(buf)) {
				flush = true;
			}
		} else if (ret == -1) {
			// No data
			k_msleep(100);
		} else {
			printk("Error reading from UART: %d\n", ret);
			flush = true;
		}

		if (flush && w > 0) {
			LOG_HEXDUMP_INF(buf, w, "Received data");
			w = 0;
		}
	}

	return 0;
}