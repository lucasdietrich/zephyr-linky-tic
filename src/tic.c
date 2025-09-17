/*
 * Copyright (c) 2025 Lucas Dietrich <ld.adecy@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <sys/errno.h>
#include <tic.h>

LOG_MODULE_REGISTER(tic, LOG_LEVEL_WRN);

#define TIC_LF	0x0A
#define TIC_CR	0x0D
#define TIC_SP	0x20
#define TIC_HT	0x09
#define TIC_ETX 0x03
#define TIC_STX 0x02

int tic_init(tic_t *tic, tic_callback_t callback, void *user_data)
{
	if (!tic || !callback) {
		return -1;
	}

	tic->state	   = TIC_WF;
	tic->callback  = callback;
	tic->user_data = user_data;
	tic->w_cursor  = 0;

	return 0;
}

static void tic_reset(tic_t *tic)
{
	tic->state = TIC_WF;
}

bool is_printable(char c)
{
	return (c >= 0x20 && c <= 0x7E);
}

int tic_data_in(tic_t *tic, unsigned char chr)
{
	chr &= 0x7F; // Ensure 7-bit data

	switch (tic->state) {
	case TIC_WF:
		if (chr == TIC_STX) {
			tic->state = TIC_WD;
		}
		break;
	case TIC_WD:
		if (chr == TIC_LF) {
			tic->state		   = TIC_LABEL;
			tic->w_cursor	   = 0;
			tic->calc_checksum = 0;
		} else if (chr == TIC_ETX) {
			tic->callback(TIC_STATUS_EOF, NULL, NULL, tic->user_data);
			tic->state = TIC_WF;
		} else {
			LOG_WRN("Unexpected char while waiting for LF: %x", chr);
			tic_reset(tic);
		}
		break;
	case TIC_LABEL:
		if (chr == TIC_SP) {
			tic->state				  = TIC_DATA;
			tic->label[tic->w_cursor] = '\0';
			tic->w_cursor			  = 0;
			tic->calc_checksum += chr;
		} else if (tic->w_cursor < TIC_LABEL_MAX_LEN) {
			tic->label[tic->w_cursor++] = (char)chr;
			tic->calc_checksum += chr;
		} else {
			// TODO issue callback error
			LOG_WRN("Label size overflow %u > %u, new char: %c (%x)",
					tic->w_cursor + 1u,
					TIC_LABEL_MAX_LEN,
					is_printable(chr) ? (char)chr : '.',
					chr);
			LOG_HEXDUMP_WRN(tic->label, tic->w_cursor, "Label so far:");
			tic_reset(tic);
		}
		break;
	case TIC_DATA:
		if (chr == TIC_SP) {
			tic->state				 = TIC_CHECKSUM;
			tic->data[tic->w_cursor] = '\0';
		} else if (tic->w_cursor < TIC_DATA_MAX_LEN) {
			tic->data[tic->w_cursor++] = (char)chr;
			tic->calc_checksum += chr;
		} else {
			// TODO issue callback error
			LOG_WRN("Data size overflow %u > %u", tic->w_cursor, TIC_DATA_MAX_LEN);
			tic_reset(tic);
		}
		break;
	case TIC_CHECKSUM:
		tic->calc_checksum = (tic->calc_checksum & 0x3F) + 0x20;
		if (tic->calc_checksum != chr) {
			LOG_WRN("Checksum error for %s: calc %x != recv %x",
					tic->label,
					tic->calc_checksum,
					chr);
			// TODO issue callback error
			tic->callback(TIC_STATUS_ERR_CHECKSUM, NULL, NULL, tic->user_data);
			tic_reset(tic);
			break;
		}
		tic->state = TIC_EOD;
		break;
	case TIC_EOD:
		if (chr == TIC_CR) {
			tic->callback(TIC_STATUS_DATASET, tic->label, tic->data, tic->user_data);
			tic->state = TIC_WD;
		} else {
			// TODO issue callback error
			LOG_WRN("Unexpected EOF: %x", chr);
			tic_reset(tic);
		}
		break;
	default:
		break;
	}

	return 0;
}

int tic_hist_parse_data(tic_hist_t *hist, const char *label, const char *data)
{
	if (!hist || !label || !data) {
		return -EINVAL;
	}

	if (strncmp(label, TIC_LABEL_ADCO, TIC_LABEL_SIZE(TIC_LABEL_ADCO)) == 0) {
		memcpy(hist->adco, data, TIC_ADCO_DATA_LEN);
	} else if (strncmp(label, TIC_LABEL_ISOUSC, TIC_LABEL_SIZE(TIC_LABEL_ISOUSC)) == 0) {
		hist->isousc = (uint16_t)strtoul(data, NULL, 10);
	} else if (strncmp(label, TIC_LABEL_BASE, TIC_LABEL_SIZE(TIC_LABEL_BASE)) == 0) {
		hist->base = (uint32_t)strtoul(data, NULL, 10);
	} else if (strncmp(label, TIC_LABEL_PTEC, TIC_LABEL_SIZE(TIC_LABEL_PTEC)) == 0) {
		hist->ptec = (uint16_t)strtoul(data, NULL, 10);
	} else if (strncmp(label, TIC_LABEL_IINST, TIC_LABEL_SIZE(TIC_LABEL_IINST)) == 0) {
		hist->iinst = (uint16_t)strtoul(data, NULL, 10);
	} else if (strncmp(label, TIC_LABEL_IMAX, TIC_LABEL_SIZE(TIC_LABEL_IMAX)) == 0) {
		hist->imax = (uint16_t)strtoul(data, NULL, 10);
	} else if (strncmp(label, TIC_LABEL_PAPP, TIC_LABEL_SIZE(TIC_LABEL_PAPP)) == 0) {
		hist->papp = (uint16_t)strtoul(data, NULL, 10);
	} else if (strncmp(label, TIC_LABEL_OPTARIF, TIC_LABEL_SIZE(TIC_LABEL_OPTARIF)) ==
			   0) {
		if (strcmp(data, TIC_HIST_DATA_OPTARIF_EXPECTED) != 0) {
			return -ENOTSUP; // Unsupported OPTARIF value
		}
	} else if (strncmp(label, TIC_LABEL_HHPHC, TIC_LABEL_SIZE(TIC_LABEL_HHPHC)) == 0) {
		if (strcmp(data, TIC_HIST_DATA_HHPHC_EXPECTED) != 0) {
			return -ENOTSUP; // Unsupported HHPHC value
		}
	} else if (strncmp(label, TIC_LABEL_MOTDETAT, TIC_LABEL_SIZE(TIC_LABEL_MOTDETAT)) ==
			   0) {
		if (strcmp(data, TIC_HIST_DATA_MOTDETAT_EXPECTED) != 0) {
			return -ENOTSUP; // Unsupported MOTDETAT value
		}
	} else {
		return -ENOTSUP; // Unknown label
	}

	return 0;
}