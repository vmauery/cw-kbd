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

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "util.h"

#include "settings.h"
#include "ringbuffer.h"
#include "cw-kbd.h"
#include "cw.h"
#include "timer.h"
#include "tick.h"

static void hid_nq(uint8_t c);
void set_command_mode(bool mode);
static void exit_command_mode(void);
void soft_reset(void);

uint8_t hid_in_report_buffer[sizeof(USB_KeyboardReport_Data_t)];

static USB_ClassInfo_HID_Device_t kbd_iface = {
	.Config = {
		.InterfaceNumber              = 0,
		.ReportINEndpointNumber       = KEYBOARD_EPNUM,
		.ReportINEndpointSize         = KEYBOARD_EPSIZE,
		.ReportINEndpointDoubleBank   = false,
		.PrevReportINBuffer           = hid_in_report_buffer,
		.PrevReportINBufferSize       = sizeof(hid_in_report_buffer),
	},
};

#ifdef DEBUG
static USB_ClassInfo_CDC_Device_t serial_iface = {
	.Config = {
		.ControlInterfaceNumber         = 1,
		.DataINEndpointNumber           = CDC_TX_EPNUM,
		.DataINEndpointSize             = CDC_TXRX_EPSIZE,
		.DataINEndpointDoubleBank       = false,
		.DataOUTEndpointNumber          = CDC_RX_EPNUM,
		.DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
		.DataOUTEndpointDoubleBank      = false,
		.NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
		.NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
		.NotificationEndpointDoubleBank = false,
	},
};

void debug_write_bytes(const char *msg) {
	if (debug_write) {
		CDC_Device_SendString(&serial_iface, msg, strlen(msg));
		CDC_Device_Flush(&serial_iface);
	}
}
void debug_write_byte(const char c) {
	if (debug_write) {
		CDC_Device_SendByte(&serial_iface, c);
		CDC_Device_Flush(&serial_iface);
	}
}

uint8_t debug_write = 0;

void _debug(PGM_P fmt, ...) {
	if (debug_write) {
		char _dbg_msg[100];
		uint8_t l;
		utoa(millis, _dbg_msg, 10);
		l = strlen(_dbg_msg);
		_dbg_msg[l++] = ':';
		_dbg_msg[l++] = ' ';
		_dbg_msg[l] = 0;
		va_list ap;
		va_start(ap, fmt);
		my_vsnprintf(_dbg_msg+l, sizeof(_dbg_msg)-l, fmt, ap);
		va_end(ap);
		debug_write_bytes(_dbg_msg);
	}
}

void _ulog(PGM_P fmt, ...) {
	char _dbg_msg[100];
	va_list ap;
	va_start(ap, fmt);
	my_vsnprintf(_dbg_msg, sizeof(_dbg_msg), fmt, ap);
	va_end(ap);
	usart_print(_dbg_msg);
}

#endif /* DEBUG */

static void usb_work(void);
static uint16_t enable_usb;
/* Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	ms_tick_register(usb_work, TICK_USB_WORK, 15);
	enable_usb = 10000;
	usb_work();
	ulog("USB ON\r");
}

/* Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	ulog("USB OFF\r");
	ms_tick_unregister(TICK_USB_WORK);
}

/* Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
#ifdef DEBUG
	if (!(CDC_Device_ConfigureEndpoints(&serial_iface)))
		;
#endif /* DEBUG */

	if (!(HID_Device_ConfigureEndpoints(&kbd_iface)))
		;
}

#ifdef DEBUG
/* this fires to let us know when the comm port opened or closed */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const dev)
{
	uint8_t line_state = dev->State.ControlLineStates.HostToDevice;
	if (line_state == 0) {
		debug_write = 0;
	} else {
		debug_write = 1;
	}
}
#endif /* DEBUG */

void EVENT_USB_Device_UnhandledControlRequest(void)
{
#ifdef DEBUG
	CDC_Device_ProcessControlRequest(&serial_iface);
#endif /* DEBUG */
	HID_Device_ProcessControlRequest(&kbd_iface);
}

void EVENT_USB_Device_StartOfFrame(void) {
}

static const prog_uint8_t ascii2hid[] = {
	/* 00 */  0,     /*  */
	/* 01 */  0,     /*  */
	/* 02 */  0,     /*  */
	/* 03 */  0,     /*  */
	/* 04 */  0,     /*  */
	/* 05 */  0,     /*  */
	/* 06 */  0,     /*  */
	/* 07 */  0,     /*  */
	/* 08 */  0x2a,  /* Backspace */
	/* 09 */  0,     /*  */
	/* 0a */  0x28,  /*  */
	/* 0b */  0,     /*  */
	/* 0c */  0,     /*  */
	/* 0d */  0,     /*  */
	/* 0e */  0,     /*  */
	/* 0f */  0,     /*  */
	/* 10 */  0,     /*  */
	/* 11 */  0,     /*  */
	/* 12 */  0,     /*  */
	/* 13 */  0,     /*  */
	/* 14 */  0,     /*  */
	/* 15 */  0,     /*  */
	/* 16 */  0,     /*  */
	/* 17 */  0,     /*  */
	/* 18 */  0,     /*  */
	/* 19 */  0,     /*  */
	/* 1a */  0,     /*  */
	/* 1b */  0,     /*  */
	/* 1c */  0,     /*  */
	/* 1d */  0,     /*  */
	/* 1e */  0,     /*  */
	/* 1f */  0,     /*  */
	/* 20 */  0x2c,  /* ' ' */
	/* 21 */  0x1e | 0x80,  /* '!' */
	/* 22 */  0x34 | 0x80,  /* '"' */
	/* 23 */  0,     /* '#' */
	/* 24 */  0x21 | 0x80,  /* '$' */
	/* 25 */  0,     /* '%' */
	/* 26 */  0,     /* '&' */
	/* 27 */  0,     /* '´' */
	/* 28 */  0x26 | 0x80,  /* '(' */
	/* 29 */  0x27 | 0x80,  /* ')' */
	/* 2a */  0,     /* '*' */
	/* 2b */  0x2e | 0x80,  /* '+' */
	/* 2c */  0x36,  /* ',' */
	/* 2d */  0x2d,  /* '-' */
	/* 2e */  0x37,  /* '.' */
	/* 2f */  0x38,  /* '/' */
	/* 30 */  0x27,  /* '0' */
	/* 31 */  0x1e,  /* '1' */
	/* 32 */  0x1f,  /* '2' */
	/* 33 */  0x20,  /* '3' */
	/* 34 */  0x21,  /* '4' */
	/* 35 */  0x22,  /* '5' */
	/* 36 */  0x23,  /* '6' */
	/* 37 */  0x24,  /* '7' */
	/* 38 */  0x25,  /* '8' */
	/* 39 */  0x26,  /* '9' */
	/* 3a */  0x13 | 0x80,  /* ':' */
	/* 3b */  0x13,  /* ';' */
	/* 3c */  0,     /* '<' */
	/* 3d */  0x2e,  /* '=' */
	/* 3e */  0,     /* '>' */
	/* 3f */  0x38 | 0x80,  /* '?' */
	/* 40 */  0,     /* '@' */
	/* 41 */  0x04 | 0x80,  /* 'A' */
	/* 42 */  0x05 | 0x80,  /* 'B' */
	/* 43 */  0x06 | 0x80,  /* 'C' */
	/* 44 */  0x0a | 0x80,  /* 'D' */
	/* 45 */  0x0e | 0x80,  /* 'E' */
	/* 46 */  0x08 | 0x80,  /* 'F' */
	/* 47 */  0x17 | 0x80,  /* 'G' */
	/* 48 */  0x0b | 0x80,  /* 'H' */
	/* 49 */  0x0f | 0x80,  /* 'I' */
	/* 4a */  0x1c | 0x80,  /* 'J' */
	/* 4b */  0x11 | 0x80,  /* 'K' */
	/* 4c */  0x18 | 0x80,  /* 'L' */
	/* 4d */  0x10 | 0x80,  /* 'M' */
	/* 4e */  0x0d | 0x80,  /* 'N' */
	/* 4f */  0x33 | 0x80,  /* 'O' */
	/* 50 */  0x15 | 0x80,  /* 'P' */
	/* 51 */  0x14 | 0x80,  /* 'Q' */
	/* 52 */  0x16 | 0x80,  /* 'R' */
	/* 53 */  0x07 | 0x80,  /* 'S' */
	/* 54 */  0x09 | 0x80,  /* 'T' */
	/* 55 */  0x0c | 0x80,  /* 'U' */
	/* 56 */  0x19 | 0x80,  /* 'V' */
	/* 57 */  0x1a | 0x80,  /* 'W' */
	/* 58 */  0x1b | 0x80,  /* 'X' */
	/* 59 */  0x12 | 0x80,  /* 'Y' */
	/* 5a */  0x1d | 0x80,  /* 'Z' */
	/* 5b */  0,     /* '[' */
	/* 5c */  0,     /* '\' */
	/* 5d */  0,     /* ']' */
	/* 5e */  0,     /* '^' */
	/* 5f */  0x2d | 0x80,  /* '_' */
	/* 60 */  0x34,  /* '`' */
	/* 61 */  0x04,  /* 'a' */
	/* 62 */  0x05,  /* 'b' */
	/* 63 */  0x06,  /* 'c' */
	/* 64 */  0x0a,  /* 'd' */
	/* 65 */  0x0e,  /* 'e' */
	/* 66 */  0x08,  /* 'f' */
	/* 67 */  0x17,  /* 'g' */
	/* 68 */  0x0b,  /* 'h' */
	/* 69 */  0x0f,  /* 'i' */
	/* 6a */  0x1c,  /* 'j' */
	/* 6b */  0x11,  /* 'k' */
	/* 6c */  0x18,  /* 'l' */
	/* 6d */  0x10,  /* 'm' */
	/* 6e */  0x0d,  /* 'n' */
	/* 6f */  0x33,  /* 'o' */
	/* 70 */  0x15,  /* 'p' */
	/* 71 */  0x14,  /* 'q' */
	/* 72 */  0x16,  /* 'r' */
	/* 73 */  0x07,  /* 's' */
	/* 74 */  0x09,  /* 't' */
	/* 75 */  0x0c,  /* 'u' */
	/* 76 */  0x19,  /* 'v' */
	/* 77 */  0x1a,  /* 'w' */
	/* 78 */  0x1b,  /* 'x' */
	/* 79 */  0x12,  /* 'y' */
	/* 7a */  0x1d,  /* 'z' */
	/* 7b */  0,     /* '{' */
	/* 7c */  0,     /* '|' */
	/* 7d */  0,     /* '}' */
	/* 7e */  0,     /* '~' */
	/* 7f */  0,     /* DEL */
};

DECLARE_RINGBUFFER(hid_q, 8);
static void hid_nq(uint8_t c) {
	debug("hid_nq(%d)\r\n", c);
	ringbuffer_push(&hid_q, c);
}

static inline uint8_t hid_dq(void) {
	return ringbuffer_pop(&hid_q);
}

static inline uint8_t hid_peek(void) {
	return ringbuffer_peek(&hid_q);
}


/* This is called periodically to let us report keys if we have any.
 * We only report one key at a time because the report buffer is a
 * list of keys currently pressed, not keys to add to the queue.  So
 * if we want to add a whole bunch of key presses, we add them one
 * at a time in subsequent HID reports.
 */
bool CALLBACK_HID_Device_CreateHIDReport(
	USB_ClassInfo_HID_Device_t* const iface,
	uint8_t* const report_id,
	const uint8_t report_type,
	void* report_data,
	uint16_t* report_size)
{
	static uint8_t last_key = 0;
	USB_KeyboardReport_Data_t* report = (USB_KeyboardReport_Data_t*)report_data;
	uint8_t key_count = 0;
	uint8_t c;
	
	/* report->Modifier = HID_KEYBOARD_MODIFER_LEFTSHIFT; */
	
	c = hid_peek();
	if (c && (last_key != c)) {
		uint8_t v;
		last_key = c = hid_dq();
		v = pgm_read_byte(&ascii2hid[c]);
		debug("hid_dq() => %#x -> %#x\r\n", c, v);
		if (v & 0x80) {
			report->Modifier = HID_KEYBOARD_MODIFER_LEFTSHIFT;
			v &= 0x7f;
		} else
			report->Modifier = 0;
		report->KeyCode[key_count++] = v;
	} else
		last_key = 0;

	*report_size = sizeof(USB_KeyboardReport_Data_t);
	return key_count != 0;
}

void CALLBACK_HID_Device_ProcessHIDReport(
			USB_ClassInfo_HID_Device_t* const iface,
			const uint8_t report_id,
			const uint8_t report_type,
			const void* report_data,
			const uint16_t report_size)
{
}

void idle(void) {
	uint8_t imode;
	set_sleep_mode(0);
	imode = rcli();
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
	SREG = imode;
}

void power_down(void) {
	uint8_t imode;
	set_sleep_mode(2);
	imode = rcli();
	sleep_enable();
	sei();
	sleep_cpu();
	sleep_disable();
	SREG = imode;
}

static void usb_work(void) {
	HID_Device_MillisecondElapsed(&kbd_iface);
	HID_Device_USBTask(&kbd_iface);
#ifdef DEBUG
	/* Must throw away unused bytes from the host, or
	 * it will lock up while waiting for the device */
	CDC_Device_ReceiveByte(&serial_iface);
	CDC_Device_USBTask(&serial_iface);
#endif /* DEBUG */

	USB_USBTask();
}

#ifdef DEBUG
static void console_control(uint8_t b) {
	switch (b) {
	case 's': settings_dump(); break;
	default: break;
	}
}
#endif /* DEBUG */

static struct {
	uint8_t next;
	uint8_t freq;
} repeat_q[MEMORY_COUNT];
static uint8_t this_minute;
static uint8_t string_inject_count;

static void inject_string(void);

static void set_memory_repeat(uint8_t mid, uint8_t freq) {
	settings_set_memory_repeat(mid, freq);
	repeat_q[mid].freq = freq;
	repeat_q[mid].next = this_minute + freq;
	if (freq) {
		if (!string_inject_count)
			ms_tick_register(inject_string, TICK_INJECT_STR, 60000);
		string_inject_count++;
	} else {
		string_inject_count--;
		if (!string_inject_count)
			ms_tick_unregister(TICK_INJECT_STR);
	}
}

static void inject_string_init(void) {
	uint8_t i, f;
	for (i=0; i<MEMORY_COUNT; i++) {
		f = settings_get_memory_repeat(i);
		set_memory_repeat(i, f);
	}
}

static void inject_string(void) {
	uint8_t i;
	uint8_t msg[65];

	this_minute++;
	for (i=0; i<MEMORY_COUNT; i++) {
		if (repeat_q[i].freq && repeat_q[i].next == this_minute) {
			repeat_q[i].next += repeat_q[i].freq;
			settings_get_memory(i, msg);
			cw_string((char*)msg);
		}
	}
}

static void clear_led(void) {
	PORTD &= ~_BV(PD6);
}

static void set_led(void) {
	PORTD |= _BV(PD6);
}

static void toggle_led(void) {
	PORTD ^= _BV(PD6);
}

enum command_mode_state {
	command_idle, /* waiting for command */
	command_input, /* waiting for command parameters */
	command_output, /* sending command reply */
	command_next_action, /* see what to do next */
	command_done, /* go back to regular mode */
} __attribute__((packed));

#ifdef DEBUG
prog_char command_state_0[] PROGMEM = "command_idle";
prog_char command_state_1[] PROGMEM = "command_input";
prog_char command_state_2[] PROGMEM = "command_output";
prog_char command_state_3[] PROGMEM = "command_next_action";
prog_char command_state_4[] PROGMEM = "command_done";
prog_char *command_mode_state_s[] PROGMEM = {
	command_state_0,
	command_state_1,
	command_state_2,
	command_state_3,
	command_state_4,
};
#endif /* DEBUG */

/*

Things we do in command mode:
	1) prompt (one char)
	2) read a string (one or more bytes)
	3) play a message (with or without sending it)

	dit paddle: (command mode, key d)
		prompt ':d'
		read $dit
		play == L or R
	playback: (command mode, e or 0-9)
		send string
	keyer mode: (command mode, key k)
		prompt ':k'
		read $mode
		play '== $mode'
	speed: (command mode, key s)
		prompt ':s'
		read $speed
		play '== $speed'
	tone: (command mode, key t)
		prompt ':t'
		read $tone
		play '== $tone'
	message: (command mode, key m)
		prompt ':m'
		read $mid
		prompt ':$mid'
		read $msg
		play '== $msg'
	preset: (command mode, key p)
		promp ':p'
		read $preset
		play == $preset

*/
void command_mode_cb(uint8_t v) {
	static enum command_mode_state cm_state;
	static uint8_t cmd_bytes;
	static uint8_t msg[65];
	static uint8_t idx;
	static uint8_t mid;
	static uint8_t next_action;
	static uint8_t command;

	debug("**** command_mode(%S) <- %d\r\n",
		&command_mode_state_s[cm_state], v);
	debug("\tcmd_bytes=%u, mid=%u, cmd=%u, na=%u, idx=%u\r\n", cmd_bytes, mid, command, next_action, idx);
	debug("msg = [%s]\r\n", msg);
	if (v == 0) {
		cm_state = command_idle;
		cmd_bytes = idx = command = next_action = 0;
		memset(msg, 0, sizeof(msg));
		return;
	}
	if (v == ' ' && cm_state != command_output)
		return;
	if (v > 2)
		hid_nq(v);
	ms_tick_register(exit_command_mode, TICK_FAUX_WDT, 30000);
	switch (cm_state) {
	case command_idle:
		command = v;
		next_action = 0;
		switch (v) {
		case 'd': /* dit paddle */
		case 'k': /* keyer mode */
		case 'm': /* message mode */
		case 'p': /* load preset */
		case 'r': /* repeat mode */
		case 's': /* speed setting */
		case 't': /* tone setting */
			cm_state = command_output;
			cmd_bytes = 2;
			cw_char(':');
			cw_char(v);
			break;
		case 'a': /* autospace */
			cw_set_word_space(!settings_get_autospace(), true);
			cw_set_word_space(false, false);
			cm_state = command_output;
			cmd_bytes = 1;
			cw_char('+');
			break;
		case 'o': /* jump to reset */
			soft_reset();
			break;
		case 'q': /* quiet mode */
			cw_set_beeper(!settings_get_beeper());
			cm_state = command_output;
			cmd_bytes = 2;
			cw_char('=');
			cw_char('=');
			break;
		case 'e':
			v = '0';
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			debug("play memory %d\r\n", v-'0');
			set_command_mode(false);
			settings_get_memory(v-'0', msg);
			msg[64] = 0;
			cw_string((char*)msg);
			break;
		default:
			debug("unknown command: %v\r\n", v);
			cmd_bytes = 1;
			cm_state = command_output;
			cw_char('?');
			break;
		}
		break;
	case command_input:
		/* '+' === /AR or EOM */
		if (v == '\b') {
			if (idx > 0) {
				msg[--idx] = 0;
				cmd_bytes++;
			}
			break;
		}
		if (v == '_')
			v = ' ';
		if (--cmd_bytes == 0 || v == '+') {
			if (v == '+')
				msg[idx] = 0;
			else {
				msg[idx++] = v;
				msg[idx] = 0;
			}
			debug("command_input: msg = [%s]\r\n", msg);
			cm_state = command_next_action;
			command_mode_cb(1);
		} else {
			msg[idx++] = v;
		}
		break;
	case command_output:
		if (--cmd_bytes == 0) {
			debug("* * * output done, switching to command_next_action\r\n");
			cm_state = command_next_action;
			command_mode_cb(1);
		}
		break;
	case command_next_action:
		/* commonly, next_action is this:
		 * 0: just sent prompt and recieved next input
		 *    so we should parse the input and respond
		 * for message, it is:
		 * 0: reply with :$mid prompt
		 * 1: save message
		 */
		cm_state = command_output;
		switch (command) {
		case 'a':
			cm_state = command_done;
			break;
		case 'd':
			if (next_action == 0) {
				next_action = 1;
				cmd_bytes = 1;
				cm_state = command_input;
			} else if (next_action == 1) {
				didah_queue_t other = cw_get_left_key();
				switch(msg[0]) {
				case 'm':
				case 'o':
				case 't':
				case '0':
					other = (other == DIT ? DAH : DIT);
					cw_set_left_key(other);
				case 'e':
				case 'i':
				case 'h':
				case 's':
				case '5':
					next_action = 2;
					msg[0] = '='; msg[1] = '='; msg[2] = ' ';
					msg[3] = (other == DIT ? 'L' : 'R');
					msg[4] = 0;
					cmd_bytes = 4;
					cw_string((char*)msg);
					break;
				default:
					next_action = 0;
					cw_char('!');
					cw_char(':');
					cw_char('d');
					cmd_bytes = 3;
					break;
				}
			} else {
				cm_state = command_done;
			}
			break;
		case 'k':
			if (next_action == 0) {
				next_action = 1;
				cmd_bytes = 1;
				cm_state = command_input;
			} else if (next_action == 1) {
				keying_mode_t m = keying_mode_unset;
				switch (msg[0]) {
					case 'a': m = keying_mode_iambic_a; break;
					case 'b': m = keying_mode_iambic_b; break;
					case 'c': m = keying_mode_bug; break;
					case 'p': m = keying_mode_paddle; break;
					case 's': m = keying_mode_straight; break;
					case 'u': m = keying_mode_ultimatic; break;
				}
				if (m != keying_mode_unset) {
					next_action = 2;
					cw_set_keying_mode(m);
					msg[3] = msg[0];
					msg[0] = '='; msg[1] = '='; msg[2] = ' ';
					msg[4] = 0;
					cmd_bytes = 4;
					cw_string((char*)msg);
				} else {
					next_action = 0;
					cw_char('!');
					cw_char(':');
					cw_char('k');
					cmd_bytes = 3;
				}
			} else {
				cm_state = command_done;
			}
			break;
		case 'p':
			if (next_action == 0) {
				next_action = 1;
				cmd_bytes = 1;
				cm_state = command_input;
			} else if (next_action == 1) {
				idx = 0;
				mid = msg[0] - '0';
				if (mid == ('?' - '0')) {
					cmd_bytes = 1;
					next_action = 2;
					cw_char(settings_get_preset() + '0');
				} else if (mid > 9) {
					next_action = 0;
					cw_char('!');
					cw_char(':');
					cw_char(command);
				} else {
					restore_preset(mid);
					next_action = 2;
					msg[0] = '='; msg[1] = '=';
					msg[2] = ' '; msg[3] = command;
					msg[4] = mid + '0';
					msg[5] = 0;
					cmd_bytes = 5;
					cw_string((char*)msg);
				}
			} else {
				cm_state = command_done;
			}
			break;
		case 's':
			if (next_action == 0) {
				next_action = 1;
				cmd_bytes = 2;
				cm_state  = command_input;
			} else if (next_action == 1) {
				uint8_t speed = atoi((char*)msg);
				if (speed > 4 && speed < 100) {
					next_action = 2;
					cw_set_speed(speed);
					msg[3] = msg[0]; msg[4] = msg[1];
					msg[0] = '='; msg[1] = '='; msg[2] = ' ';
					msg[5] = 0;
					cmd_bytes = 5;
					cw_string((char*)msg);
				} else {
					next_action = 0;
					cw_char('!');
					cw_char(':');
					cw_char('s');
					cmd_bytes = 3;
				}
			} else {
				cm_state = command_done;
			}
			break;
		case 't':
			if (next_action == 0) {
				next_action = 1;
				cmd_bytes = 2;
				cm_state  = command_input;
			} else if (next_action == 1) {
				uint8_t tone = atoi((char*)msg);
				if (tone > 8) {
					next_action = 2;
					cw_set_frequency(tone);
					msg[3] = msg[0]; msg[4] = msg[1];
					msg[0] = '='; msg[1] = '='; msg[2] = ' ';
					msg[5] = 0;
					cmd_bytes = 5;
					cw_string((char*)msg);
				} else {
					next_action = 0;
					cw_char('!');
					cw_char(':');
					cw_char('t');
					cmd_bytes = 3;
				}
			} else {
				cm_state = command_done;
			}
			break;
		case 'q':
			cm_state = command_done;
			break;
		case 'm':
		case 'r':
			if (next_action == 0) {
				next_action = 1;
				cmd_bytes = 1;
				cm_state  = command_input;
			} else if (next_action == 1) {
				idx = 0;
				mid = msg[0] - '0';
				if (mid > 9) {
					cmd_bytes = 3;
					cw_char('!');
					cw_char(':');
					cw_char(command);
					next_action = 0;
				} else {
					cmd_bytes = 2;
					cw_char(':');
					cw_char('0' + mid);
					next_action = 2;
				}
				cm_state = command_output;
			} else if (next_action == 2) {
				if (command == 'r')
					cmd_bytes = 2;
				else
					cmd_bytes = 64;
				cm_state = command_input;
				next_action = 3;
			} else if (next_action == 3) {
				if (command == 'r') {
					uint8_t r = atoi((char*)msg);
					if ((msg[0] == '0' || r > 0) && r < 100) {
						next_action = 4;
						set_memory_repeat(mid, r);
						msg[5] = msg[0]; msg[6] = msg[1];
						msg[0] = '='; msg[1] = '='; msg[2] = ' ';
						msg[3] = 'r'; msg[4] = '0'+mid;
						msg[7] = 0;
						cmd_bytes = strlen((char*)msg);
						cw_string((char*)msg);
					} else {
						next_action = 2;
						cmd_bytes = 4;
						cw_char('!');
						cw_char(':');
						cw_char(command);
						cw_char('0'+mid);
					}
				} else {
					next_action = 4;
					settings_set_memory(mid, msg);
					/* play back message */
					cmd_bytes = strlen((char*)msg) + 3;
					cw_char('=');
					cw_char('=');
					cw_char(' ');
					cw_string((char*)msg);
				}
			} else {
				cm_state = command_done;
			}
			break;
		default:
			cm_state = command_idle;
			break;
		}
		if (cm_state == command_input) {
			idx = 0;
			memset(msg, 0, sizeof(msg));
		} else if (cm_state == command_done) {
			command_mode_cb(1);
		}
		break;
	case command_done:
		command_mode_cb(0);
		set_command_mode(false);
		break;
	}
}

void int6_enable(void) {
	/* disable debounce timer and enable interrupt */
	DDRE &= ~_BV(PE6);
	EICRB &= ~(_BV(ISC61) | _BV(ISC60));
	EIFR = _BV(INTF6);
	EIMSK |= _BV(INT6);
}

static void int6_disable(void) {
	EIMSK &= ~_BV(INT6);
}

bool command_mode = false;
void set_command_mode(bool mode) {
	command_mode = mode;
	if (command_mode) {
		cw_set_word_space(false, false);
		cw_disable_outputs(CW_ENABLE_KEYER|CW_ENABLE_DIDAH);
		ms_tick_register(toggle_led, TICK_TOGGLE_LED, 250);
		ms_tick_register(exit_command_mode, TICK_FAUX_WDT, 30000);
		command_mode_cb(0);
		cw_set_dq_callback(command_mode_cb);
	} else {
		cw_set_word_space(settings_get_autospace(), false);
		cw_enable_outputs(CW_ENABLE_KEYER|CW_ENABLE_DIDAH);
		ms_tick_unregister(TICK_TOGGLE_LED);
		ms_tick_unregister(TICK_FAUX_WDT);
		set_led();
		cw_set_dq_callback(hid_nq);
	}
	cw_clear_queues();
}

static void exit_command_mode(void) {
	set_command_mode(false);
	ms_tick_unregister(TICK_FAUX_WDT);
}

void int6_debounce(void) {
	static uint16_t v;
	static uint16_t last_millis;
	uint8_t pressed;

	pressed = ((PINE & _BV(PE6)) ? 0 : 1);
	if (pressed) {
		v++;
		if (v == 10) {
			set_command_mode(!command_mode);
		} else if (v == 2000) {
			// set sane defaults without resetting mems
			set_command_mode(false);
			debug("choose sane defaults\n");
			settings_choose_sanity();
			restore_preset(settings_get_preset());
		} else if (v == 5000) {
			// reset eeprom
			set_command_mode(false);
			debug("reset eeprom!!\n");
			settings_default();
		}
	} else {
		v = 0;
		ms_tick_unregister(TICK_INT6_DEBOUNCE);
		int6_enable();
	}
	last_millis = millis;
}

/* command mode button */
ISR(INT6_vect) {
	int6_disable();
	debug("INT6\n");
	ms_tick_register(int6_debounce, TICK_INT6_DEBOUNCE, 1);
}

/* if you wire together PD4 and reset, you can do a HARD reset */
/* #define HARD_RESET */
__attribute__((naked)) void soft_reset(void) {
	uint8_t mcucr;
	USB_ShutDown();
	while (USB_IsInitialized);
	cli();
#ifdef HARD_RESET
	PORTD &= _BV(PD4);
	while(1);
#else /* !HARD_RESET */
	mcucr = MCUCR | _BV(IVCE) | _BV(IVSEL);
	MCUCR |= _BV(IVCE);
	MCUCR = mcucr;
	cw_fini();
	int6_disable();
	clear_led();
	ms_tick_stop();
	asm volatile ("jmp %0" : : "i"(BOOT_START_ADDR));
#endif /* HARD_RESET */
}

void sw_init(void) {
	settings_init();
	ms_tick_init();
	inject_string_init();
}

/* Configures the board hardware and chip peripherals for the demo's functionality. */
void hw_init(void)
{
	uint8_t v;
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* disable jtag */
	v = MCUCR | _BV(JTD);
	MCUCR = v;
	MCUCR = v;

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	sei();

#ifdef HARD_RESET
	PORTD |= _BV(PD4);
	DDRD |= _BV(PD4);
#endif /* HARD_RESET */

	/* Hardware Initialization */
	USB_Init();
#if 0
	while (!debug_write) {
		usb_work();
	}
#endif

	ms_tick_start();
	cw_init(settings_get_wpm(), (cw_dq_cb_t)&hid_nq);

	/* enable blinky led */
	DDRD |= _BV(PD6);
	set_led();

	/* initialize the command mode button */
	int6_enable();
}

int main(void)
{
#ifdef DEBUG
#define ulog_limited(B) \
	if (sc++ == 0) \
		ulog_byte(B)
	uint8_t sc = 0;
	sei();
	usart_init(115200);
	usart_read_callback(console_control);
	ulog("hello world\r\n");
	cli();
#else
#define ulog_limited(B) while(0) {}
#endif /* DEBUG */
	sw_init();
	hw_init();

	cw_string("    hi.");
	for (;;)
	{
		if (enable_usb) {
			usb_work();
			_delay_ms(1);
			enable_usb--;
		}
		if (waiting_events) {
			idle();
			ulog_limited('.');
		} else {
			ulog_byte('@');
			power_down();
			ulog_byte('*');
		}
	}
}

