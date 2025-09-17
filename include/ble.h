/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BLE_H
#define _BLE_H

#include "tic.h"

#include <zephyr/kernel.h>

int ble_init(void);

int ble_update_adv(const tic_hist_t *hist);

#endif /* _BLE_H */