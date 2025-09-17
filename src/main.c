/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ble.h"
#include "tic.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define UART_DEVICE_NODE DT_NODELABEL(uart1)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define RX_CHUNK_LEN	64
#define RX_RINGBUF_SIZE 1024

K_SEM_DEFINE(rx_enable_sem, 1, 1);
K_SEM_DEFINE(rx_data_sem, 0, 1);
uint8_t async_rx_buffer[2u][RX_CHUNK_LEN];
volatile uint8_t async_rx_buffer_idx = 0;
RING_BUF_DECLARE(uart_rx_ringbuf, RX_RINGBUF_SIZE);

#if !defined(CONFIG_TIC_DEMO_MODE)
static void
uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int rc;
	uint32_t written;

	switch (evt->type) {
	case UART_RX_BUF_REQUEST:
		/* Return the next buffer index */
		rc = uart_rx_buf_rsp(
			dev, async_rx_buffer[async_rx_buffer_idx], sizeof(async_rx_buffer[0]));
		__ASSERT_NO_MSG(rc == 0);
		async_rx_buffer_idx = async_rx_buffer_idx ? 0 : 1;
		break;
	case UART_RX_BUF_RELEASED:
		break;
	case UART_RX_DISABLED:
		LOG_WRN("RX disabled");
		async_rx_buffer_idx = 0;
		k_sem_give(&rx_enable_sem);
		break;
	case UART_RX_RDY:
		written = ring_buf_put(
			&uart_rx_ringbuf, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
		if (written != evt->data.rx.len) {
			LOG_WRN("Received bytes dropped from ring buf");
		}
		/* Notify upper layer that new data has arrived */
		k_sem_give(&rx_data_sem);
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
#endif

struct tic_infos {
	tic_mode_t mode;
	union {
		tic_hist_t hist;
	};
	struct {
		uint64_t rx_bytes;
		uint32_t rx_datasets;
		uint32_t checksum_err;
	} stats;
};

static struct tic_infos tic_infos = {
	.mode = TIC_MODE_HISTORIQUE,
};

void tc_cb(tic_status_t err, const char *label, const char *data, void *user_data)
{
	if (err == TIC_STATUS_ERR_CHECKSUM) {
		tic_infos.stats.checksum_err++;
	} else if (err == TIC_STATUS_EOF) {
		LOG_INF("TIC PAPP: %u W IINST: %u A (IMAX %u A ISOUSC %u) BASE %u Wh - ADCO "
				"%.12s (rx B: %llu N dt: %u C err: %u)",
				tic_infos.hist.papp,
				tic_infos.hist.iinst,
				tic_infos.hist.imax,
				tic_infos.hist.isousc,
				tic_infos.hist.base,
				tic_infos.hist.adco,
				tic_infos.stats.rx_bytes,
				tic_infos.stats.rx_datasets,
				tic_infos.stats.checksum_err);
	} else if (err == TIC_STATUS_DATASET) {
		LOG_DBG("TIC %s:%s [%d]", label, data, err);
		tic_infos.stats.rx_datasets++;

		int ret = tic_hist_parse_data(&tic_infos.hist, label, data);
		if (ret == 0) {
			/* If value changed, update advertising data */
			ble_update_adv(&tic_infos.hist);
		} else {
			LOG_WRN("Failed to parse TIC data %s:%s (%d)", label, data, ret);
		}
	}
}

int main(void)
{
	int ret;
	static uint8_t buf[128];
	LOG_INF("Linky TIC reader started");

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not found!");
		return 0;
	}

	struct tic tic;
	ret = tic_init(&tic, tc_cb, (void *)&tic_infos);
	if (ret != 0) {
		LOG_ERR("Failed to initialize TIC: %d", ret);
		return 0;
	}

#if !defined(CONFIG_TIC_DEMO_MODE)
	/* Register the async interrupt handler */
	ret = uart_callback_set(uart_dev, uart_callback, (void *)uart_dev);
	if (ret != 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return 0;
	}
#endif /* !CONFIG_TIC_DEMO_MODE */

#if defined(CONFIG_BT)
	ret = ble_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize BLE: %d", ret);
		return 0;
	}
#endif /* CONFIG_BT */

	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(
			K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &rx_data_sem),
		K_POLL_EVENT_INITIALIZER(
			K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &rx_enable_sem),
	};

	for (;;) {
		events[0].state = K_POLL_STATE_NOT_READY;
		events[1].state = K_POLL_STATE_NOT_READY;

		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (ret == 0) {
			if (events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
				ret = k_sem_take(&rx_data_sem, K_NO_WAIT);
				__ASSERT(ret == 0, "Failed to take rx_data_sem");
				uint32_t read;
				while ((read = ring_buf_get(&uart_rx_ringbuf, buf, sizeof(buf))) > 0) {
					tic_infos.stats.rx_bytes += read;
					for (uint32_t i = 0; i < read; i++) {
						tic_data_in(&tic, buf[i]);
					}
				}
			}

			if (events[1].state == K_POLL_STATE_SEM_AVAILABLE) {
				ret = k_sem_take(&rx_enable_sem, K_NO_WAIT);
				__ASSERT(ret == 0, "Failed to take rx_enable_sem");
				LOG_INF("Enabling RX");
				ring_buf_reset(&uart_rx_ringbuf);
				k_sem_reset(&rx_data_sem);
				ret = uart_rx_enable(uart_dev,
									 async_rx_buffer[async_rx_buffer_idx],
									 RX_CHUNK_LEN,
									 SYS_FOREVER_US);
				if (ret != 0) {
					LOG_ERR("Failed to enable RX: %d", ret);
				}
			}
		}
	}

	return 0;
}