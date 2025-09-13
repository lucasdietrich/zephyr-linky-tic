/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tic.h"
#include "zephyr/device.h"
#include "zephyr/sys/printk.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

#define CAPTURE_FLOW 0

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

#define UART_DEVICE_NODE DT_NODELABEL(uart1)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static uint64_t rx_bytes = 0;
static uint64_t rx_data = 0u;
static uint64_t checksum_err = 0;

void tc_cb(tic_status_t err, const char *label, const char *data)
{
	if (err == TIC_STATUS_ERR_CHECKSUM) {
		checksum_err++;
	} else if (err == TIC_STATUS_EOF) {
		printk("TIC bytes: %llu, datasets: %llu, checksum errors: %llu\n", rx_bytes, rx_data, checksum_err);
	} else if (err == TIC_STATUS_DATASET) {
		printk("TIC %s:%s [%d]\n", label, data, err);
		rx_data++;
	}
}

int main(void)
{
	int ret;
	printk("Linky TIC reader started\n");

	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!\n");
		return 0;
	}

	struct tic tic;
	ret = tic_init(&tic, tc_cb);
	if (ret != 0) {
		printk("Failed to initialize TIC: %d\n", ret);
		return 0;
	}
	
	for (;;) {
		char s;
		ret = uart_poll_in(uart_dev, &s);
		if (ret == 0) {
			rx_bytes++;
			tic_data_in(&tic, &s, 1);
		} else if (ret == -1) {
			// No data
			k_msleep(1);
		} else {
			LOG_ERR("Error reading from UART: %d\n", ret);
		}
	}

	return 0;
}