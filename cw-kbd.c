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

#endif /* DEBUG */

/* Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/* Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
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

/* The plan here is to tag the key presses with timestamps so we
 * can collate the timeline of events into dits, dahs and spaces
 * which can then be collated into characters and prosigns
 *
 * ideally, each event would look something like this:

enum key_event_type {
	KEY_EVENT_LEFT,
	KEY_EVENT_RIGHT,
	KEY_EVENT_LETTER_SPACE,
	KEY_EVENT_WORD_SPACE,
} __attribute__((packed));

struct key_event {
	uint16_t start;
	uint16_t length;
	enum key_event_type type;
};

 * This would allow us to look at them by starting time and determine
 * any overlaps by merely looking at the next event in the queue.
 * Since there are only two event types and only one instance of each
 * class of event can be active at any given time, we only have to
 * look at two events to determine the current state.  A queue length
 * of 16 events is more than enough to get us through single letters
 * and more.  This will cost 80 bytes.
 *
 * As the events get enqueued
 */

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

#define wdt_interrupt(value)   \
__asm__ __volatile__ (  \
	"in __tmp_reg__,__SREG__" "\n\t"   \
	"cli" "\n\t"   \
	"wdr" "\n\t"   \
	"sts %0,%1" "\n\t"  \
	"out __SREG__,__tmp_reg__" "\n\t"   \
	"sts %0,%2" "\n\t" \
	: /* no outputs */  \
	: "M" (_SFR_MEM_ADDR(_WD_CONTROL_REG)), \
	"r" (_BV(_WD_CHANGE_BIT) | _BV(WDIE)), \
	"r" ((uint8_t) ((value & 0x08 ? _WD_PS3_MASK : 0x00) | \
		_BV(WDIE) | (value & 0x07)) ) \
	: "r0"  \
)

ISR(WDT_vect) {
	wdt_disable();
	power_down();
	wdt_interrupt(WDTO_8S);
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

#ifdef INJECT_STR
static void inject_string(void) {
	cw_string(" ");
}
#endif /* INJECT_STR */

static void toggle_port(void) {
	PORTD ^= _BV(PD6);
}

enum command_mode_state {
	command_idle, /* waiting for command */
	command_input, /* waiting for command parameters */
	command_output, /* sending command reply */
} __attribute__((packed));

#if DEBUG
prog_char command_state_0[] PROGMEM = "command_idle";
prog_char command_state_1[] PROGMEM = "command_input";
prog_char command_state_2[] PROGMEM = "command_output";
prog_char *command_mode_state_s[] PROGMEM = {
	command_state_0,
	command_state_1,
	command_state_2,
};
#endif /* DEBUG */

void command_mode_cb(uint8_t v) {
	static enum command_mode_state cm_state;
	static uint8_t cmd_bytes;
	static uint8_t msg[65];
	static uint8_t idx;
	static uint8_t mid;
	static uint8_t command;
	debug("command_mode(%S) <- %d\r\n",
		&command_mode_state_s[cm_state], v);
	if (v == 0) {
		cm_state = command_idle;
		cmd_bytes = idx = command = 0;
		memset(msg, 0, sizeof(msg));
		return;
	}
	hid_nq(v);
	switch (cm_state) {
	case command_idle:
		command = v;
		switch (v) {
		case 'c': /* callsign */
			/* up to 16 bytes */
			cmd_bytes = 16;
			cm_state = command_input;
			debug("set callsign\r\n");
			break;
		case 'd': /* dit paddle */
			/* read in 1 byte for paddle
			 * e or t
			 * (touch once with dit paddle) */
			cmd_bytes = 1;
			cm_state = command_input;
			debug("set dit\r\n");
			break;
		case 'e':
			set_command_mode(false);
			settings_get_callsign(msg);
			msg[16] = 0;
			cw_string((char*)msg);
			break;
		case 'k': /* keyer mode */
			/* one byte for keyer mode
			 * iambic (A), iambic (B), (C)ootie or bug,
			 * (S)traight, (U)ltimatic */
			cmd_bytes = 1;
			cm_state = command_input;
			debug("set keyer\r\n");
			break;
		case 'm': /* message mode */
			/* one byte for mid 0-9 */
			cmd_bytes = 1;
			cm_state = command_input;
			debug("set message mode\r\n");
			break;
		case 's': /* speed setting */
			/* read 2 bytes for wpm */
			cmd_bytes = 2;
			cm_state = command_input;
			debug("set speed\r\n");
			break;
		case 't': /* tone setting */
			/* read in 3 bytes for tone */
			cmd_bytes = 3;
			cm_state = command_input;
			debug("set tone\r\n");
			break;
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
		if (cm_state == command_input) {
			memset(msg, 0, sizeof(msg));
			idx = 0;
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
			/* save message */
			cm_state = command_idle;
			switch (command) {
			case 'c': /* callsign */
				settings_set_callsign(msg);
				break;
			case 'd': /* left paddle */
				if (v == 't') {
					didah_queue_t other =
						(cw_get_left_key() == DIT ? DAH : DIT);
					cw_set_left_key(other);
				}
				break;
			case 'k': /* keyer mode */
			{
				keying_mode_t m;
				switch (msg[0]) {
					case 'a': m = keying_mode_iambic_a; break;
					case 'b': m = keying_mode_iambic_b; break;
					case 'c': m = keying_mode_bug; break;
					case 's': m = keying_mode_straight; break;
					case 'u': m = keying_mode_ultimatic; break;
					default: m = settings_get_keying_mode(); break;
				}
				cw_set_keying_mode(m);
			}
			case 'm': /* message mode */
				idx = 0;
				mid = msg[0] - '0';
				if (mid > 9) {
					cm_state = command_input;
					cmd_bytes = 1;
					break;
				}
				memset(msg, 0, sizeof(msg));
				cmd_bytes = 64;
				cm_state = command_input;
				command = 'x';
				break;
			case 's': /* speed setting */
				cw_set_speed(atoi((char*)msg));
				break;
			case 't': /* tone setting */
				cw_set_frequency(atoi((char*)msg));
				break;
			case 'x':
				/* save the message */
				settings_set_memory(mid, msg);
				/* play back message */
				cmd_bytes = strlen((char*)msg) + 3;
				cm_state = command_output;
				cw_char('0'+mid);
				cw_char(':');
				cw_char(' ');
				cw_string((char*)msg);
				break;
			}
			if (cm_state == command_idle)
				set_command_mode(false);
		} else {
			msg[idx++] = v;
		}
		break;
	case command_output:
		if (--cmd_bytes == 0) {
			cm_state = command_idle;
		}
		break;
	}
}

void int6_enable(void) {
	/* disable debounce timer and enable interrupt */
	ms_tick_register(NULL, TICK_DEBOUNCE_INT6, 0);
	EICRB = (EICRB & ~_BV(ISC60)) | _BV(ISC61);
	EIFR = _BV(INTF6);
	EIMSK |= _BV(INT6);
}

bool command_mode = false;
void set_command_mode(bool mode) {
	command_mode = mode;
	cw_clear_queues();
	if (command_mode) {
		ms_tick_register(toggle_port, TICK_TOGGLE_PORT, 250);
		command_mode_cb(0);
		cw_set_dq_callback(command_mode_cb);
	} else {
		ms_tick_register(toggle_port, TICK_TOGGLE_PORT, 1000);
		cw_set_dq_callback(hid_nq);
	}
}

/* command mode button */
ISR(INT6_vect) {
	/* debounce by disabling further interrupts for a short
	 * period of time (timer) and then re-enabling them */
	EIMSK &= ~_BV(INT6);

	ms_tick_register(int6_enable, TICK_DEBOUNCE_INT6, 250);
	set_command_mode(!command_mode);
	debug("command_mode = %d\r\n", command_mode);
}

void sw_init(void) {
	settings_init();
	ms_tick_init();
	ms_tick_register(usb_work, TICK_USB_WORK, 1);
#ifdef INJECT_STR
	ms_tick_register(inject_string, TICK_INJECT_STR, 60000);
#endif /* INJECT_STR */
	ms_tick_register(toggle_port, TICK_TOGGLE_PORT, 1000);
}

/* Configures the board hardware and chip peripherals for the demo's functionality. */
void hw_init(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	sei();

	/* Hardware Initialization */
	USB_Init();
#ifdef DEBUG
	while (!debug_write) {
		usb_work();
	}
#endif /* DEBUG */

	ms_tick_start();
	cw_init(settings_get_wpm(), (cw_dq_cb_t)&hid_nq);
	cw_set_frequency(settings_get_frequency());

	/* initialize the command mode button */
	int6_enable();
}

int main(void)
{
	sw_init();
	hw_init();

	cw_string("    hi.");
	for (;;)
	{
		idle();
	}
}

