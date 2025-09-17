/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "tic.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/time_units.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(ble, LOG_LEVEL_DBG);

#define TIC_HIST_ADV_DATA_LEN 11u
#define TIC_HIST_SD_DATA_LEN  17u

static uint8_t energy_meter_ad_data[2u + TIC_HIST_ADV_DATA_LEN] = {0xff, 0xff};
static uint8_t energy_meter_sd_data[2u + TIC_HIST_SD_DATA_LEN]	= {0xff, 0xff};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(
		BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	BT_DATA(
		BT_DATA_MANUFACTURER_DATA, energy_meter_ad_data, sizeof(energy_meter_ad_data)),
};

const struct bt_data sd[] = {
	BT_DATA(
		BT_DATA_MANUFACTURER_DATA, energy_meter_sd_data, sizeof(energy_meter_sd_data)),
};

static int tic_hist_ser_ad(const tic_hist_t *hist, uint8_t *adbuf, size_t buf_len)
{
	if (!hist || !adbuf || buf_len < TIC_HIST_ADV_DATA_LEN) {
		return -EINVAL;
	}

	adbuf[0] = TIC_MODE_HISTORIQUE;
	sys_put_le32(hist->base, &adbuf[1]);
	sys_put_le16(hist->iinst, &adbuf[5]);
	sys_put_le16(hist->ptec, &adbuf[7]);
	sys_put_le16(hist->papp, &adbuf[9]);

	return 0;
}

static int tic_hist_ser_sd(const tic_hist_t *hist, uint8_t *sdbuf, size_t buf_len)
{
	if (!hist || !sdbuf || buf_len < TIC_HIST_SD_DATA_LEN) {
		return -EINVAL;
	}

	sdbuf[0] = TIC_MODE_HISTORIQUE;
	memcpy(&sdbuf[1], hist->adco, TIC_ADCO_DATA_LEN);
	sys_put_le16(hist->isousc, &sdbuf[13]);
	sys_put_le16(hist->imax, &sdbuf[15]);

	return 0;
}

#define ADV_PARAM                                                                        \
	BT_LE_ADV_PARAM(0, BT_GAP_ADV_FAST_INT_MIN_1, BT_GAP_ADV_FAST_INT_MAX_1, NULL)

int ble_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	struct bt_le_adv_param params = {0};

	params.id				  = BT_ID_DEFAULT;
	params.sid				  = 0;
	params.secondary_max_skip = 0;
	params.options			  = BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_NOTIFY_SCAN_REQ;
#if defined(CONFIG_TIC_ADV_FAST1)
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
#elif defined(CONFIG_TIC_ADV_FAST2)
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
#else
	params.interval_min = BT_GAP_ADV_SLOW_INT_MIN;
	params.interval_max = BT_GAP_ADV_SLOW_INT_MAX;
#endif
	params.peer = NULL;

	err = bt_le_adv_start(&params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return err;
	}

	return err;
}

int ble_update_adv(const tic_hist_t *hist)
{
	int err;

	err = tic_hist_ser_ad(hist, &energy_meter_ad_data[2u], TIC_HIST_ADV_DATA_LEN);
	if (err) {
		LOG_ERR("Failed to serialize TIC historique data for advertising (err %d)\n",
				err);
		return err;
	}

	err = tic_hist_ser_sd(hist, &energy_meter_sd_data[2u], TIC_HIST_SD_DATA_LEN);
	if (err) {
		LOG_ERR("Failed to serialize TIC historique data for scan response (err %d)\n",
				err);
		return err;
	}

	err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Failed to update advertising data (err %d)\n", err);
		return err;
	}

	return 0;
}