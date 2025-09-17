/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TIC_H
#define _TIC_H

#include <stddef.h>
#include <stdint.h>

#define TIC_LABEL_MAX_LEN 8u
#define TIC_DATA_MAX_LEN  12u

#define TIC_LABEL_ADCO	   "ADCO"
#define TIC_LABEL_ISOUSC   "ISOUSC"
#define TIC_LABEL_BASE	   "BASE"
#define TIC_LABEL_PTEC	   "PTEC"
#define TIC_LABEL_IINST	   "IINST"
#define TIC_LABEL_IMAX	   "IMAX"
#define TIC_LABEL_PAPP	   "PAPP"
#define TIC_LABEL_HHPHC	   "HHPHC"
#define TIC_LABEL_MOTDETAT "MOTDETAT"
#define TIC_LABEL_OPTARIF  "OPTARIF"
#define TIC_LABEL_HHPHC	   "HHPHC"
#define TIC_LABEL_MOTDETAT "MOTDETAT"

#define TIC_HIST_DATA_OPTARIF_EXPECTED	"BASE"
#define TIC_HIST_DATA_HHPHC_EXPECTED	"A"
#define TIC_HIST_DATA_MOTDETAT_EXPECTED "000000"

#define TIC_LABEL_SIZE(_label) (sizeof(_label) - 1u)

#define TIC_ADCO_DATA_LEN 12u

typedef enum tic_mode {
	TIC_MODE_HISTORIQUE = 0,
	TIC_MODE_STANDARD,
} tic_mode_t;

typedef enum tic_status {
	TIC_STATUS_DATASET = 0,
	TIC_STATUS_EOF,
	TIC_STATUS_ERR_CHECKSUM
} tic_status_t;

typedef void (*tic_callback_t)(tic_status_t err,
							   const char *label,
							   const char *data,
							   void *user_data);

typedef enum tic_state {
	TIC_WF,		  // Waiting for start of frame (STX)
	TIC_WD,		  // Waiting for start of dataset (LF)
	TIC_LABEL,	  // Parsing label
	TIC_DATA,	  // Parsing data
	TIC_CHECKSUM, // Parsing checksum
	TIC_EOD,	  // Waiting for end of dataset (CR)
} tic_state_t;

typedef struct tic {
	tic_callback_t callback;
	void *user_data;
	tic_state_t state;
	size_t w_cursor;
	unsigned char calc_checksum;
	char label[TIC_LABEL_MAX_LEN + 1];
	char data[TIC_DATA_MAX_LEN + 1];
} tic_t;

int tic_init(tic_t *tic, tic_callback_t callback, void *user_data);

int tic_data_in(tic_t *tic, unsigned char chr);

typedef struct tic_hist {
	char adco[TIC_ADCO_DATA_LEN]; // CONST
	uint16_t isousc;			  // CONST
	uint32_t base;				  // VARIABLE
	uint16_t ptec;				  // VARIABLE
	uint16_t iinst;				  // VARIABLE
	uint16_t imax;				  // CONST
	uint16_t papp;				  // VARIABLE
} tic_hist_t;

// 12 + 2 + 4 + 2 + 2 + 2 + 4

int tic_hist_parse_data(tic_hist_t *hist, const char *label, const char *data);

#endif /* _TIC_H */