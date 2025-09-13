/*
 * Copyright (c) 2024 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TIC_H
#define _TIC_H

#define TIC_LABEL_MAX_LEN 8u
#define TIC_DATA_MAX_LEN 12u

#include <stddef.h>

typedef enum tic_status {
    TIC_STATUS_DATASET = 0,
    TIC_STATUS_EOF,
    TIC_STATUS_ERR_CHECKSUM
    
} tic_status_t;

typedef void (*tic_callback_t)(tic_status_t err, const char *label, const char *data);

typedef enum tic_state {
    TIC_WF, // Waiting for start of frame (STX)
    TIC_WD, // Waiting for start of dataset (LF)
    TIC_LABEL, // Parsing label
    TIC_DATA, // Parsing data
    TIC_CHECKSUM, // Parsing checksum
    TIC_EOD, // Waiting for end of dataset (CR)
} tic_state_t;

typedef struct tic {
    tic_callback_t callback;
    tic_state_t state;
    size_t w_cursor;
    unsigned char calc_checksum;
    char label[TIC_LABEL_MAX_LEN + 1];
    char data[TIC_DATA_MAX_LEN + 1];
} tic_t;

int tic_init(tic_t *tic, tic_callback_t callback);

int tic_data_in(tic_t *tic, unsigned char *buf, size_t len);

#endif /* _TIC_H */