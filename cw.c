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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "cw-kbd.h"
#include "settings.h"
#include "util.h"
#include "cw.h"
#include "timer.h"
#include "tick.h"
#include "ringbuffer.h"


void didah_enqueue(didah_queue_t what);
uint8_t didah_dequeue(didah_queue_t *didah);
void didah_decode(didah_queue_t next);

static cw_dq_cb_t cw_dq_cb;
static uint8_t dit_len;
static keying_mode_t keying_mode = keying_mode_ultimatic;

DECLARE_RINGBUFFER(cw_q, 128);
static void cw_nq(uint8_t c) {
	ringbuffer_push(&cw_q, c);
}

static inline uint8_t cw_dq(void) {
	return ringbuffer_pop(&cw_q);
}


/* encoding of the morse code is as follows:
 * zeros at high order bits until start bit (one)
 * following the start bit, a zero is a dit and
 * a one is a dah.  For example, 0x14 expanded
 * into binary is 0b00010100, which is .-.. or L.
 *                  ^  ^^ 
 *   leading zeros -|  ||- start of dits/dahs
 *                     |- start bit (one)
 *
 * Any uncoded ascii chars were left as zero.
 */

/* we may have to take some special care with prosigns since
 * they really are just letters crammed together.  When reading
 * in the paddles, we have to make sure that there are no spaces
 * between the prosign letters or they will come out as distinct
 * letters.
 */
static const prog_uint8_t cw2ascii[] = {
	/* 00000000 */  ' ',
	/* 00000001 */  0,
	/* 00000010 */  'e',
	/* 00000011 */  't',
	/* 00000100 */  'i',
	/* 00000101 */  'a',
	/* 00000110 */  'n',
	/* 00000111 */  'm',
	/* 00001000 */  's',
	/* 00001001 */  'u',
	/* 00001010 */  'r',
	/* 00001011 */  'w',
	/* 00001100 */  'd',
	/* 00001101 */  'k',
	/* 00001110 */  'g',
	/* 00001111 */  'o',
	/* 00010000 */  'h',
	/* 00010001 */  'v',
	/* 00010010 */  'f',
	/* 00010011 */  0,
	/* 00010100 */  'l',
	/* 00010101 */  0,
	/* 00010110 */  'p',
	/* 00010111 */  'j',
	/* 00011000 */  'b',
	/* 00011001 */  'x',
	/* 00011010 */  'c',
	/* 00011011 */  'y',
	/* 00011100 */  'z',
	/* 00011101 */  'q',
	/* 00011110 */  0,
	/* 00011111 */  0,
	/* 00100000 */  '5',
	/* 00100001 */  '4',
	/* 00100010 */  0,
	/* 00100011 */  '3',
	/* 00100100 */  0,
	/* 00100101 */  0,
	/* 00100110 */  0,
	/* 00100111 */  '2',
	/* 00101000 */  0,
	/* 00101001 */  0,
	/* 00101010 */  '+',
	/* 00101011 */  0,
	/* 00101100 */  0,
	/* 00101101 */  0,
	/* 00101110 */  0,
	/* 00101111 */  '1',
	/* 00110000 */  '6',
	/* 00110001 */  '=',
	/* 00110010 */  '/',
	/* 00110011 */  0,
	/* 00110100 */  0,
	/* 00110101 */  0,
	/* 00110110 */  '(',
	/* 00110111 */  0,
	/* 00111000 */  '7',
	/* 00111001 */  0,
	/* 00111010 */  0,
	/* 00111011 */  0,
	/* 00111100 */  '8',
	/* 00111101 */  0,
	/* 00111110 */  '9',
	/* 00111111 */  '0',
	/* 01000000 */  0,
	/* 01000001 */  0,
	/* 01000010 */  0,
	/* 01000011 */  0,
	/* 01000100 */  0,
	/* 01000101 */  0,
	/* 01000110 */  0,
	/* 01000111 */  0,
	/* 01001000 */  0,
	/* 01001001 */  0,
	/* 01001010 */  0,
	/* 01001011 */  0,
	/* 01001100 */  '?',
	/* 01001101 */  '_',
	/* 01001110 */  0,
	/* 01001111 */  0,
	/* 01010000 */  0,
	/* 01010001 */  0,
	/* 01010010 */  '"',
	/* 01010011 */  0,
	/* 01010100 */  0,
	/* 01010101 */  '.',
	/* 01010110 */  0,
	/* 01010111 */  0,
	/* 01011000 */  0,
	/* 01011001 */  0,
	/* 01011010 */  0,
	/* 01011011 */  0,
	/* 01011100 */  0,
	/* 01011101 */  0,
	/* 01011110 */  '\'',
	/* 01011111 */  0,
	/* 01100000 */  0,
	/* 01100001 */  0,
	/* 01100010 */  0,
	/* 01100011 */  0,
	/* 01100100 */  0,
	/* 01100101 */  0,
	/* 01100110 */  0,
	/* 01100111 */  0,
	/* 01101000 */  0,
	/* 01101001 */  0,
	/* 01101010 */  ';',
	/* 01101011 */  '!',
	/* 01101100 */  0,
	/* 01101101 */  ')',
	/* 01101110 */  0,
	/* 01101111 */  0,
	/* 01110000 */  0,
	/* 01110001 */  0,
	/* 01110010 */  0,
	/* 01110011 */  ',',
	/* 01110100 */  0,
	/* 01110101 */  0,
	/* 01110110 */  0,
	/* 01110111 */  0,
	/* 01111000 */  ':',
	/* 01111001 */  0,
	/* 01111010 */  0,
	/* 01111011 */  0,
	/* 01111100 */  0,
	/* 01111101 */  0,
	/* 01111110 */  0,
	/* 01111111 */  0,
};

struct prosign {
	uint16_t bits;
	char value[4];
};

static const PROGMEM struct prosign prosigns[] = {
	{ 0b00010101, { 'A', 'A', 0 } }, /* End of line */
	{ 0b00101000, { 'A', 'S', 0 } }, /* Stand by */
	{ 0b00101010, { 'A', 'R', 0 } }, /* End of message */
	{ 0b00110001, { 'B', 'T', 0 } }, /* break between address and text or text and signature */
	{ 0b00110010, { 'N', 'R', 0 } }, /* NR, number */
	{ 0b01000101, { 'S', 'K', 0 } }, /* out, clear, no more communications */
	{ 0b01011010, { 'A', 'C', 0 } }, /* used for @ symbol */
	{ 0b10000000, { '\b', 0 } }, /* this is the cheater version of error (backspace) */
	{ 0b10001001, { '$', 0 } },
	{ 0b100000000, { '\b', 0 } }, /* 8 dits == mistake */
	{ 0b1000111000, { 'S', 'O', 'S', 0 } }, /* technically SOS is a prosign */
};

static const prog_uint8_t cw[] = {
	/* 00 */  0,
	/* 01 */  0,
	/* 02 */  0,
	/* 03 */  0,
	/* 04 */  0,
	/* 05 */  0,
	/* 06 */  0,
	/* 07 */  0,
	/* 08 */  0,
	/* 09 */  0,
	/* 0a */  0,
	/* 0b */  0,
	/* 0c */  0,
	/* 0d */  0,
	/* 0e */  0,
	/* 0f */  0,
	/* 10 */  0,
	/* 11 */  0,
	/* 12 */  0,
	/* 13 */  0,
	/* 14 */  0,
	/* 15 */  0,
	/* 16 */  0,
	/* 17 */  0,
	/* 18 */  0,
	/* 19 */  0,
	/* 1a */  0,
	/* 1b */  0,
	/* 1c */  0,
	/* 1d */  0,
	/* 1e */  0,
	/* 1f */  0,
	/* 20 */  0x01,  /* ' ' */
	/* 21 */  0x6b,  /* '!' */
	/* 22 */  0x52,  /* '"' */
	/* 23 */  0,     /* '#' */
	/* 24 */  0x89,  /* '$' */
	/* 25 */  0,     /* '%' */
	/* 26 */  0,     /* '&' */
	/* 27 */  0,     /* '´' */
	/* 28 */  0x36,  /* '(' */
	/* 29 */  0x6d,  /* ')' */
	/* 2a */  0,     /* '*' */
	/* 2b */  0x2a,  /* '+' */
	/* 2c */  0x73,  /* ',' */
	/* 2d */  0x61,  /* '-' */
	/* 2e */  0x55,  /* '.' */
	/* 2f */  0x32,  /* '/' */
	/* 30 */  0x3f,  /* '0' */
	/* 31 */  0x2f,  /* '1' */
	/* 32 */  0x27,  /* '2' */
	/* 33 */  0x23,  /* '3' */
	/* 34 */  0x21,  /* '4' */
	/* 35 */  0x20,  /* '5' */
	/* 36 */  0x30,  /* '6' */
	/* 37 */  0x38,  /* '7' */
	/* 38 */  0x3c,  /* '8' */
	/* 39 */  0x3e,  /* '9' */
	/* 3a */  0x78,  /* ':' */
	/* 3b */  0x6a,  /* ';' */
	/* 3c */  0,     /* '<' */
	/* 3d */  0x31,  /* '=' */
	/* 3e */  0,     /* '>' */
	/* 3f */  0x4c,  /* '?' */
	/* 40 */  0,     /* '@' */
	/* 41 */  0x5,   /* 'A' */
	/* 42 */  0x18,  /* 'B' */
	/* 43 */  0x1a,  /* 'C' */
	/* 44 */  0xc,   /* 'D' */
	/* 45 */  0x2,   /* 'E' */
	/* 46 */  0x12,  /* 'F' */
	/* 47 */  0xe,   /* 'G' */
	/* 48 */  0x10,  /* 'H' */
	/* 49 */  0x4,   /* 'I' */
	/* 4a */  0x17,  /* 'J' */
	/* 4b */  0xd,   /* 'K' */
	/* 4c */  0x14,  /* 'L' */
	/* 4d */  0x7,   /* 'M' */
	/* 4e */  0x6,   /* 'N' */
	/* 4f */  0xf,   /* 'O' */
	/* 50 */  0x16,  /* 'P' */
	/* 51 */  0x1d,  /* 'Q' */
	/* 52 */  0xa,   /* 'R' */
	/* 53 */  0x8,   /* 'S' */
	/* 54 */  0x3,   /* 'T' */
	/* 55 */  0x9,   /* 'U' */
	/* 56 */  0x11,  /* 'V' */
	/* 57 */  0xb,   /* 'W' */
	/* 58 */  0x19,  /* 'X' */
	/* 59 */  0x1b,  /* 'Y' */
	/* 5a */  0x1c,  /* 'Z' */
	/* 5b */  0,     /* '[' */
	/* 5c */  0,     /* '\' */
	/* 5d */  0,     /* ']' */
	/* 5e */  0,     /* '^' */
	/* 5f */  0x4d,  /* '_' */
	/* 60 */  0x5e,  /* '`' */
	/* 61 */  0x5,   /* 'a' */
	/* 62 */  0x18,  /* 'b' */
	/* 63 */  0x1a,  /* 'c' */
	/* 64 */  0xc,   /* 'd' */
	/* 65 */  0x2,   /* 'e' */
	/* 66 */  0x12,  /* 'f' */
	/* 67 */  0xe,   /* 'g' */
	/* 68 */  0x10,  /* 'h' */
	/* 69 */  0x4,   /* 'i' */
	/* 6a */  0x17,  /* 'j' */
	/* 6b */  0xd,   /* 'k' */
	/* 6c */  0x14,  /* 'l' */
	/* 6d */  0x7,   /* 'm' */
	/* 6e */  0x6,   /* 'n' */
	/* 6f */  0xf,   /* 'o' */
	/* 70 */  0x16,  /* 'p' */
	/* 71 */  0x1d,  /* 'q' */
	/* 72 */  0xa,   /* 'r' */
	/* 73 */  0x8,   /* 's' */
	/* 74 */  0x3,   /* 't' */
	/* 75 */  0x9,   /* 'u' */
	/* 76 */  0x11,  /* 'v' */
	/* 77 */  0xb,   /* 'w' */
	/* 78 */  0x19,  /* 'x' */
	/* 79 */  0x1b,  /* 'y' */
	/* 7a */  0x1c,  /* 'z' */
	/* 7b */  0,     /* '{' */
	/* 7c */  0,     /* '|' */
	/* 7d */  0,     /* '}' */
	/* 7e */  0,     /* '~' */
	/* 7f */  0,     /* DEL */
};

static void beeper_on(void);
static void beeper_off(void);

static void cw_led_on(void) {
	// turn on led
	CW_PORT |= CW_BIT;
	// turn on beeper
	beeper_on();
}

static void cw_led_off(void) {
	// turn off led
	CW_PORT &= ~CW_BIT;
	// turn off beeper
	beeper_off();
}

void cw_char(char c) {
	if (c == '\'') {
		c = '`';
	}
	cw_nq(c);
}

void cw_string(const char* str) {
	while (str && *str) {
		cw_char(*str);
		str++;
	}
}

enum cw_state {
		cws_hyper = 0,
		cws_idle,
		cws_send_bit,
		cws_dah,
		cws_dah2,
		cws_bit_sp,
		cws_char_sp,
		cws_word_sp,
		cws_word_sp2,
		cws_word_sp3,
} __attribute__((packed));

#if DEBUG
prog_char cw_state_0[] PROGMEM = "cws_hyper";
prog_char cw_state_1[] PROGMEM = "cws_idle";
prog_char cw_state_2[] PROGMEM = "cws_send_bit";
prog_char cw_state_3[] PROGMEM = "cws_dah";
prog_char cw_state_4[] PROGMEM = "cws_dah2";
prog_char cw_state_5[] PROGMEM = "cws_bit_sp";
prog_char cw_state_6[] PROGMEM = "cws_char_sp";
prog_char cw_state_7[] PROGMEM = "cws_word_sp";
prog_char cw_state_8[] PROGMEM = "cws_word_sp2";
prog_char cw_state_9[] PROGMEM = "cws_word_sp3";
prog_char *cw_state_s[] PROGMEM = {
	cw_state_0,
	cw_state_1,
	cw_state_2,
	cw_state_3,
	cw_state_4,
	cw_state_5,
	cw_state_6,
	cw_state_7,
	cw_state_8,
	cw_state_9,
};
#endif /* DEBUG */

static void cw_out_advance_tick(void);
static void cw_out_normal_tick(void) {
	/* return to normal speed */
	ms_tick_register(cw_out_advance_tick, TICK_CW_ADVANCE, dit_len);
}

static void cw_out_hyper_tick(void) {
	/* return to normal speed */
	ms_tick_register(cw_out_advance_tick, TICK_CW_ADVANCE, 1);
}

static void cw_out_advance_tick(void) {
	// state machine
	static enum cw_state state = cws_idle;
	static uint8_t byte, bit, orig_byte;
	didah_queue_t didah;

#ifdef DEBUG
	static enum cw_state lstate = cws_send_bit;
	if (lstate != state)
		debug("cw_out: state %S\r\n", &cw_state_s[state]);
	lstate = state;
#endif /* DEBUG */

	switch (state) {
		case cws_hyper:
			cw_out_hyper_tick();
			state = cws_idle;
			break;
		case cws_idle:
			if (didah_dequeue(NULL)) {
				debug("have didahs to dequeue\r\n");
				cw_out_normal_tick();
				state = cws_send_bit;
				return cw_out_advance_tick();
			}
			if ((byte = cw_dq())) {
				debug("cw_dq byte %d\r\n", byte);
				if (byte > 127) break;
				orig_byte = byte;
				if (byte == 32) {
					didah_enqueue(SPACE);
					state = cws_word_sp;
					break;
				}
				byte = pgm_read_byte(&cw[byte]);
				if (byte == 0) break;
				bit = 0;
				while (byte && !(byte&0x80)) {
					byte <<= 1;
					bit++;
				}
				byte <<= 1;
				bit++;
				while (bit < 8) {
					didah_enqueue((byte & 0x80)?DAH:DIT);
					byte <<= 1;
					bit++;
				}
				didah_enqueue(SPACE);
				state = cws_send_bit;
				cw_out_normal_tick();
				return cw_out_advance_tick();
			}
			break;

		case cws_send_bit:
			if (didah_dequeue(&didah)) {
				switch (didah) {
				case DIT:
					state = cws_bit_sp;
					cw_led_on();
					break;

				case DAH:
					state = cws_dah;
					cw_led_on();
					break;

				case SPACE:
					state = cws_char_sp;
					break;
				}
			} else {
				state = cws_hyper;
				return cw_out_advance_tick();
			}
			break;

		case cws_dah: state = cws_dah2; break;
		case cws_dah2: state = cws_bit_sp; break;

		case cws_bit_sp:
			// turn off the bit
			cw_led_off();
			state = cws_send_bit;
			break;

		case cws_char_sp: state = cws_hyper; break;

		case cws_word_sp: state = cws_word_sp2; break;
		case cws_word_sp2: state = cws_word_sp3; break;
		case cws_word_sp3: state = cws_hyper; break;
	}
}

enum keying_state {
	keying_idle,
	keying_left_press,     /* enqueue left didahs per left didah cycle */
	keying_right_press,    /* enqueue right didahs per right didah cycle */
	keying_both_press,     /* enqueue didahs per last pressed didah cycle */
} __attribute__((packed));

enum keying_transition_events {
	keying_x_tick = 1,
	keying_x_left_key_press,
	keying_x_left_key_release,
	keying_x_right_key_press,
	keying_x_right_key_release,
} __attribute__((packed));

#if DEBUG
prog_char key_state_0[] PROGMEM = "keying_idle";
prog_char key_state_1[] PROGMEM = "keying_left_press";
prog_char key_state_2[] PROGMEM = "keying_right_press";
prog_char key_state_3[] PROGMEM = "keying_both_press";
prog_char *keying_state_s[] PROGMEM = {
	key_state_0,
	key_state_1,
	key_state_2,
	key_state_3,
};

prog_char key_evt_0[] PROGMEM = "keying_x_no_event";
prog_char key_evt_1[] PROGMEM = "keying_x_tick";
prog_char key_evt_2[] PROGMEM = "keying_x_left_key_press";
prog_char key_evt_3[] PROGMEM = "keying_x_left_key_release";
prog_char key_evt_4[] PROGMEM = "keying_x_right_key_press";
prog_char key_evt_5[] PROGMEM = "keying_x_right_key_release";
prog_char *keying_transition_events_s[] PROGMEM = {
	key_evt_0,
	key_evt_1,
	key_evt_2,
	key_evt_3,
	key_evt_4,
	key_evt_5,
};

#endif /* DEBUG */

#define DIDAH_Q_LEN 16
DECLARE_RINGBUFFER(cw_didah_q, DIDAH_Q_LEN);

void didah_enqueue(didah_queue_t what) {
	debug("enqueue %d\r\n", what);
	ringbuffer_push(&cw_didah_q, what);
}

uint8_t didah_dequeue(didah_queue_t *didah) {
	uint8_t have_didahs;
	have_didahs = !ringbuffer_empty(&cw_didah_q);
	if (!didah) {
		return have_didahs;
	}
	if (have_didahs) {
		*didah = (didah_queue_t)ringbuffer_pop(&cw_didah_q);
		debug("dequeue %d\r\n", *didah);
		didah_decode(*didah);
		return 1;
	}
	return 0;
}

void didah_decode(didah_queue_t next) {
	static uint16_t bits = 0x01;
	uint8_t c;
	switch (next) {
	case DIT:
		bits <<= 1;
		debug("bits = %#x\r\n", bits);
		break;
	case DAH:
		bits <<= 1;
		bits |= 1;
		debug("bits = %#x\r\n", bits);
		break;
	case SPACE:
		debug("decoding %#x\r\n", bits);
		if (bits == 0x01) {
			if (cw_dq_cb)
				cw_dq_cb(' ');
			debug("decode: space\r\n");
		/* find bits in a table */
		} else if (bits < 127 && (c = pgm_read_byte(&cw2ascii[bits]))) {
			if (cw_dq_cb)
				cw_dq_cb(c);
			debug("decode: %#x -> %d\r\n", bits&0xff, c);
		} else {
			uint8_t i, j;
			debug("prosign: bits = %#b\r\n", bits);
			for (i=0; i<(sizeof(prosigns)/sizeof(struct prosign)); i++) {
				debug("prosign_lookup: %#b\r\n",
				      pgm_read_word(&prosigns[i].bits));
				if (bits == pgm_read_word(&prosigns[i].bits)) {
					j = 0;
					while ((c = pgm_read_byte(&prosigns[i].value[j]))) {
						if (cw_dq_cb) {
							if (j == 0 && c != '\b')
								cw_dq_cb('/');
							cw_dq_cb(c);
						}
						j++;
					}
					break;
				}
			}
		}
		bits = 0x01;
		break;
	}
}

#define initial_dit_len 96
uint16_t didah_len[2];

static didah_queue_t left_didah;

void cw_set_left_key(didah_queue_t didah) {
	left_didah = didah;
	settings_set_left_key(didah);
}
didah_queue_t cw_get_left_key(void) {
	return left_didah;
}

#define right_didah ((left_didah+1)&0x01)
#define other_didah(D) ((D+1)&0x01)
static void cw_in_advance_tick(enum keying_transition_events event) {
	static enum keying_state cstate = keying_idle;
	static int16_t keyed_ticks[3];
	static didah_queue_t last_keyed;
	static uint8_t enqueued_spaces = 0;
	enum keying_state nstate;
#ifdef DEBUG
	static enum keying_state lstate = keying_both_press;
	static uint8_t tick_events;
#endif

	uint8_t iv = rcli();
	nstate = cstate;

#ifdef DEBUG
	if (event != keying_x_tick || tick_events++ == 0) {
		if (tick_events != 1)
			debug("**** skipped %u tick events\r\n", tick_events-1);
		debug("cw_in: state %S (%S)\r\n", &keying_state_s[cstate], &keying_transition_events_s[event]);
		if (event != keying_x_tick)
			tick_events = 0;
	}
	lstate = cstate;
#endif /* DEBUG */

	/* figure out what happened to get us here */
	switch (cstate) {
	case keying_idle:
		/* if we get a (left|right) keypress, enqueue and go to keying_\1_press */
		switch (event) {
		case keying_x_tick:
			if (enqueued_spaces < 1) {
				keyed_ticks[SPACE]++;
				if (keyed_ticks[SPACE] > didah_len[DIT]) {
					didah_enqueue(SPACE);
					enqueued_spaces++;
					keyed_ticks[SPACE] = 0;
				}
			}
			break;
		case keying_x_left_key_press:
			nstate = keying_left_press;
			keyed_ticks[left_didah] = didah_len[left_didah];
			break;
		case keying_x_left_key_release:
			debug("BAD!!! keying_x_left_key_release in idle\r\n");
			break;
		case keying_x_right_key_press:
			nstate = keying_right_press;
			keyed_ticks[right_didah] = didah_len[right_didah];
			break;
		case keying_x_right_key_release:
			debug("BAD!!! keying_x_right_key_release in idle\r\n");
			break;
		}
		break;
	case keying_left_press:
		/* if we have a right key press go to keying_both_press */
		/* if we have a key release, go back to idle */
		/* if we have a tick, stay and enqueue left didahs */
		switch (event) {
		case keying_x_tick:
			if (keyed_ticks[left_didah]++ == didah_len[left_didah]) {
				keyed_ticks[left_didah] = 0;
				didah_enqueue(left_didah);
				last_keyed = SPACE;
			}
			break;
		case keying_x_left_key_press:
			debug("BAD!!! keying_x_left_key_release in keying_left_press\r\n");
			break;
		case keying_x_left_key_release:
			nstate = keying_idle;
			enqueued_spaces = 0;
			keyed_ticks[SPACE] = 0;
			if (keying_mode == keying_mode_iambic_b && last_keyed != SPACE) {
				didah_enqueue(last_keyed);
				last_keyed = SPACE;
			}
			break;
		case keying_x_right_key_press:
			keyed_ticks[right_didah] = didah_len[right_didah];
			nstate = keying_both_press;
			last_keyed = right_didah;
			break;
		case keying_x_right_key_release:
			debug("BAD!!! keying_x_right_key_release in keying_left_press\r\n");
			break;
		}
		break;
	case keying_right_press:
		/* if we have a left key press, go to keying_both_press */
		/* if we have a key release, go back to idle */
		/* if we have a tick, stay and enqueue right didahs */
		switch (event) {
		case keying_x_tick:
			if (keyed_ticks[right_didah]++ == didah_len[right_didah]) {
				keyed_ticks[right_didah] = 0;
				didah_enqueue(right_didah);
				last_keyed = SPACE;
			}
			break;
		case keying_x_left_key_press:
			keyed_ticks[left_didah] = didah_len[left_didah];
			nstate = keying_both_press;
			last_keyed = left_didah;
			break;
		case keying_x_left_key_release:
			debug("BAD!!! keying_x_left_key_release in keying_right_press\r\n");
			break;
		case keying_x_right_key_press:
			debug("BAD!!! keying_x_right_key_release in keying_right_press\r\n");
			break;
		case keying_x_right_key_release:
			nstate = keying_idle;
			enqueued_spaces = 0;
			keyed_ticks[SPACE] = 0;
			if (keying_mode == keying_mode_iambic_b && last_keyed != SPACE) {
				didah_enqueue(last_keyed);
				last_keyed = SPACE;
			}
			break;
		}
		break;
	case keying_both_press:
		/* if we have N ticks, move on to enqueue and continue */
		/* if we have a key release, go back to keying_*_press */
		switch (event) {
		case keying_x_tick:
			if (keyed_ticks[last_keyed]++ == didah_len[last_keyed]) {
				didah_enqueue(last_keyed);
				if (keying_mode | keying_mode_iambic) {
					last_keyed = other_didah(last_keyed);
				}
				keyed_ticks[last_keyed] = 0;
			}
			break;
		case keying_x_left_key_press:
			debug("BAD!!! keying_x_left_key_press in keying_both_ready\r\n");
			break;
		case keying_x_left_key_release:
			keyed_ticks[right_didah] = didah_len[right_didah] - didah_len[DIT];
			nstate = keying_right_press;
			break;
		case keying_x_right_key_press:
			debug("BAD!!! keying_x_right_key_press in keying_both_ready\r\n");
			break;
		case keying_x_right_key_release:
			keyed_ticks[left_didah] = didah_len[left_didah] - didah_len[DIT];
			nstate = keying_left_press;
			break;
		}
		break;
	}
	cstate = nstate;
	sreg(iv);
}

static clock_select16_t beeper_clock;
static void beeper_on(void) {
	timer3_set_scale(beeper_clock);
}
static void beeper_off(void) {
	timer3_set_scale(t16_stopped);
	BEEPER_PORT &= ~BEEPER_BIT;
}

void cw_set_frequency(uint16_t hz) {
	uint16_t top;
	uint32_t tmp;

	debug("cw_set_frequency(%u)\r\n", hz);
	if (hz < 80 || hz > 8000) {
		debug("hz out of range\r\n");
		hz = 220;
	}

	settings_set_frequency(hz);
	tmp = F_CPU / hz / 2 - 1;

	if (tmp <= 0xffff) {
		top = (uint16_t)tmp;
		beeper_clock = t16_no_prescaling;
	} else {
		top = F_CPU / hz / 2 / 64 - 1;
		beeper_clock = t16_divide_by_64;
	}
	// we turn the beeper on independently with beeper_on
	timer3_set_top(top);
}

void cw_set_speed(uint8_t wpm) {
	debug("cw_set_speed(%u)\r\n", wpm);
	if (wpm < 3 || wpm > 99) {
		debug("wpm out of range\r\n");
		wpm = 13;
	}
	settings_set_wpm(wpm);
	dit_len = 1200 / wpm;
	didah_len[0] = 2*(uint16_t)dit_len;
	didah_len[1] = 4*(uint16_t)dit_len;
	cw_out_normal_tick();
}

void cw_set_keying_mode(keying_mode_t mode) {
	keying_mode = mode;
	settings_set_keying_mode(mode);
}

void toggle_bit(void) {
	BEEPER_PORT ^= BEEPER_BIT;
}

void cw_tick(void) {
	cw_in_advance_tick(keying_x_tick);
}

void cw_set_dq_callback(cw_dq_cb_t cb) {
	cw_dq_cb = cb;
}

void cw_enable_outputs(bool enable) {
	if (enable) {
		DIT_DDR |= DIT_BIT;
		DAH_DDR |= DAH_BIT;
		CW_DDR |= CW_BIT;
	} else {
		DIT_DDR &= ~DIT_BIT;
		DAH_DDR &= ~DAH_BIT;
		CW_DDR &= ~CW_BIT;
	}
}

ISR(INT0_vect) {
	enum keying_transition_events event;
	event = (PIND & _BV(PD0)) ?
	        keying_x_left_key_release : keying_x_left_key_press;
	debug("key_left_%s (%d)\r\n", ((event==keying_x_left_key_press)?"press":"release"), PIND);
	cw_in_advance_tick(event);
}

ISR(INT1_vect) {
	enum keying_transition_events event;
	event = (PIND & _BV(PD1)) ?
	        keying_x_right_key_release : keying_x_right_key_press;
	debug("key_right_%s (%d)\r\n", ((event==keying_x_right_key_press)?"press":"release"), PIND);
	cw_in_advance_tick(event);
}

/* this sets up timer1 for asynchronous CW output */
void cw_init(uint8_t wpm, cw_dq_cb_t cb) {
	left_didah = settings_get_left_key();
	cw_set_dq_callback(cb);
	ms_tick_register(cw_tick, TICK_CW_PARSE, 1);
	cw_set_speed(wpm);
	timer3_set_compare_a_callback(&toggle_bit);
	timer3_init(t16_stopped, T16_CTC_OCRNA, T16_COMPA);

	/* setup the output pins/ports */
	cw_enable_outputs(true);
	BEEPER_DDR |= BEEPER_BIT;

	_delay_ms(100);
	/* capture both edge events on INT0 and INT1 */
	DDRD = DDRD & ~(_BV(DDD0) | _BV(DDD1));
	EICRA = (EICRA & ~(_BV(ISC11) | _BV(ISC01))) | (_BV(ISC10) | _BV(ISC00));
	EIFR = _BV(INTF1) | _BV(INTF0);
	EIMSK |= (_BV(INT1) | _BV(INT0));
}

