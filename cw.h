/*
 * ex: set syntax=c tabstop=8 noexpandtab shiftwidth=8:
 *
 * cw-kbd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as  published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * cw-kbd is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with cw-kbd.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright © 2009-2010, Vernon Mauery (N7OH)
*/

#ifndef __CW_H__
#define __CW_H__

#include <stdint.h>
#include <stdbool.h>
#include "util.h"

#define DIT_PORT_LETTER F
#define DIT_BIT_NUMBER 5

#define DIT_PORT _PORT(DIT_PORT_LETTER)
#define DIT_DDR  _DDR(DIT_PORT_LETTER)
#define DIT_BIT  _BV(DIT_BIT_NUMBER)

#define DAH_PORT_LETTER F
#define DAH_BIT_NUMBER 6

#define DAH_PORT _PORT(DAH_PORT_LETTER)
#define DAH_DDR  _DDR(DAH_PORT_LETTER)
#define DAH_BIT  _BV(DAH_BIT_NUMBER)

#define CW_PORT_LETTER F
#define CW_BIT_NUMBER 7

#define CW_PORT _PORT(CW_PORT_LETTER)
#define CW_DDR  _DDR(CW_PORT_LETTER)
#define CW_BIT  _BV(CW_BIT_NUMBER)

#define BEEPER_PORT_LETTER B
#define BEEPER_BIT_NUMBER 4

#define BEEPER_PORT _PORT(BEEPER_PORT_LETTER)
#define BEEPER_DDR  _DDR(BEEPER_PORT_LETTER)
#define BEEPER_BIT  _BV(BEEPER_BIT_NUMBER)

#define CW_ENABLE_BEEPER 0x01
#define CW_ENABLE_KEYER  0x02
#define CW_ENABLE_DIDAH  0x04
#define CW_ENABLE_ALL    (CW_ENABLE_BEEPER|CW_ENABLE_KEYER|CW_ENABLE_DIDAH)

typedef void(*cw_dq_cb_t)(uint8_t);

typedef enum {
	keying_mode_unset = 0,
	keying_mode_straight,
	keying_mode_ultimatic,
	keying_mode_dumb = 0x04,
	keying_mode_bug = 0x05,
	keying_mode_paddle = 0x06,
	keying_mode_iambic = 0x10,
	keying_mode_iambic_a = 0x11,
	keying_mode_iambic_b = 0x12,
} __attribute__((packed)) keying_mode_t;

typedef enum {
	DIT,
	DAH,
	SPACE,
} __attribute__((packed)) didah_queue_t;

void cw_char(char c);
void cw_string(const char* str);
void cw_set_speed(uint8_t wpm);
void cw_set_left_key(didah_queue_t didah);
didah_queue_t cw_get_left_key(void);
void cw_init(uint8_t wpm, cw_dq_cb_t cb);
void cw_set_frequency(uint16_t hz);
void cw_set_keying_mode(keying_mode_t mode);
void cw_set_dq_callback(cw_dq_cb_t cb);
void cw_enable_outputs(uint8_t enable_what);
void cw_disable_outputs(uint8_t enable_what);
void cw_clear_queues(void);
void cw_set_beeper(bool beep);

#endif
