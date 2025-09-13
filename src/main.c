/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tic.h"
#include "zephyr/device.h"
#include "zephyr/sys/printk.h"
#include "zephyr/sys/time_units.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define UART_DEVICE_NODE DT_NODELABEL(uart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define RX_CHUNK_LEN 64
K_SEM_DEFINE(rx_enable, 1, 1);
uint8_t async_rx_buffer[2u][RX_CHUNK_LEN];
volatile uint8_t async_rx_buffer_idx = 0;

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int rc;

	switch (evt->type) {
	case UART_RX_BUF_REQUEST:
		/* Return the next buffer index */
		LOG_DBG("Providing buffer index %d", async_rx_buffer_idx);
		rc = uart_rx_buf_rsp(dev, async_rx_buffer[async_rx_buffer_idx],
				     sizeof(async_rx_buffer[0]));
		__ASSERT_NO_MSG(rc == 0);
		async_rx_buffer_idx = async_rx_buffer_idx ? 0 : 1;
		break;
	case UART_RX_BUF_RELEASED:
		break;
	case UART_RX_DISABLED:
		LOG_WRN("RX disabled");
		async_rx_buffer_idx = 0;
		k_sem_give(&rx_enable);
		break;
	case UART_RX_RDY:
		LOG_HEXDUMP_INF(evt->data.rx.buf + evt->data.rx.offset,
				evt->data.rx.len, "RX_RDY");
		break;
	case UART_RX_STOPPED:
		LOG_WRN("RX stopped, reason %d", evt->data.rx_stop.reason);
		break;
	case UART_TX_DONE:
	case UART_TX_ABORTED:
	default:
		LOG_WRN("Unhandled event %d", evt->type);
		break;
	}
}

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
	LOG_INF("Linky TIC reader started\n");

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not found!\n");
		return 0;
	}

	struct tic tic;
	ret = tic_init(&tic, tc_cb);
	if (ret != 0) {
		LOG_ERR("Failed to initialize TIC: %d\n", ret);
		return 0;
	}

	/* Register the async interrupt handler */
	ret = uart_callback_set(uart_dev, uart_callback, (void *)uart_dev);
	if (ret != 0) {
		LOG_ERR("Failed to set UART callback: %d\n", ret);
		return 0;
	}

	for (;;) {
		if (k_sem_take(&rx_enable, K_NO_WAIT) == 0) {
			LOG_INF("Enabling RX\n");
			ret = uart_rx_enable(uart_dev,
						   async_rx_buffer[async_rx_buffer_idx],
						   RX_CHUNK_LEN,
						   SYS_FOREVER_US);
			if (ret != 0) {
				LOG_ERR("Failed to enable RX: %d\n", ret);
			}
		}

		k_sleep(K_SECONDS(1));
		// char s;
		// ret = uart_poll_in(uart_dev, &s);
		// if (ret == 0) {
		// 	rx_bytes++;
		// 	tic_data_in(&tic, &s, 1);
		// } else if (ret == -1) {
		// 	// No data
		// 	k_msleep(1);
		// } else {
		// 	LOG_ERR("Error reading from UART: %d\n", ret);
		// }
	}

	return 0;
}