#include "zephyr/logging/log_core.h"
#include "zephyr/sys/printk.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <tic.h>

LOG_MODULE_REGISTER(tic, LOG_LEVEL_WRN);

#define TIC_LF	0x0A
#define TIC_CR	0x0D
#define TIC_SP	0x20
#define TIC_HT	0x09
#define TIC_ETX 0x03
#define TIC_STX 0x02

int tic_init(tic_t *tic, tic_callback_t callback)
{
	if (!tic || !callback) {
		return -1;
	}

	tic->state	  = TIC_WF;
	tic->callback = callback;
	tic->w_cursor = 0;

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

int tic_data_in(tic_t *tic, unsigned char *buf, size_t len)
{
	while (len--) {
		unsigned char chr = *buf++ & 0x7F; // Ensure 7-bit data

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
				tic->callback(TIC_STATUS_EOF, NULL, NULL);
				tic->state = TIC_WF;
			} else {
				// unexpected character, stay in WAIT_DATASET
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
				LOG_WRN("Label size overflow %u > %u", tic->w_cursor, TIC_LABEL_MAX_LEN);
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
				tic->callback(TIC_STATUS_ERR_CHECKSUM, NULL, NULL);
				tic_reset(tic);
				break;
			}
			tic->state = TIC_EOD;
			break;
		case TIC_EOD:
			if (chr == TIC_CR) {
				tic->callback(TIC_STATUS_DATASET, tic->label, tic->data);
				tic->state = TIC_WD;
			} else {
				LOG_WRN("Unexpected EOF: %x", chr);
				tic_reset(tic);
			}
			break;
		default:
			break;
		}
	}

	return 0;
}