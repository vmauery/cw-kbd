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

//#include <avr/eeprom.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "util.h"

#include "cw-kbd.h"
#include "cw.h"
#include "timer.h"
#include "tick.h"

static uint8_t wpm = 20;

static uint8_t hid_nq(char c);

enum paddle_mode_t {
	paddle_mode_bug = 0,
	paddle_mode_straight_key,
	paddle_mode_ultimatic,
	paddle_mode_iambic_a,
	paddle_mode_iambic_b,
	paddle_mode_count,
};
/*
EEMEM struct prefs_ {
	uint8_t wpm;
	enum paddle_mode_t mode;
	uint8_t autospace;
} prefs = {
	13,
	paddle_mode_bug,
	0,
};
*/

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
	/* 21 */  0x1e,  /* '!' */
	/* 22 */  0x34,  /* '"' */
	/* 23 */  0,     /* '#' */
	/* 24 */  0x21,  /* '$' */
	/* 25 */  0,     /* '%' */
	/* 26 */  0,     /* '&' */
	/* 27 */  0,     /* '´' */
	/* 28 */  0x26,  /* '(' */
	/* 29 */  0x27,  /* ')' */
	/* 2a */  0,     /* '*' */
	/* 2b */  0x2e,  /* '+' */
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
	/* 3a */  0x13,  /* ':' */
	/* 3b */  0x13,  /* ';' */
	/* 3c */  0,     /* '<' */
	/* 3d */  0x2e,  /* '=' */
	/* 3e */  0,     /* '>' */
	/* 3f */  0x38,  /* '?' */
	/* 40 */  0,     /* '@' */
	/* 41 */  0x04,  /* 'A' */
	/* 42 */  0x05,  /* 'B' */
	/* 43 */  0x06,  /* 'C' */
	/* 44 */  0x0a,  /* 'D' */
	/* 45 */  0x0e,  /* 'E' */
	/* 46 */  0x08,  /* 'F' */
	/* 47 */  0x17,  /* 'G' */
	/* 48 */  0x0b,  /* 'H' */
	/* 49 */  0x0f,  /* 'I' */
	/* 4a */  0x1c,  /* 'J' */
	/* 4b */  0x11,  /* 'K' */
	/* 4c */  0x18,  /* 'L' */
	/* 4d */  0x10,  /* 'M' */
	/* 4e */  0x0d,  /* 'N' */
	/* 4f */  0x33,  /* 'O' */
	/* 50 */  0x15,  /* 'P' */
	/* 51 */  0x14,  /* 'Q' */
	/* 52 */  0x16,  /* 'R' */
	/* 53 */  0x07,  /* 'S' */
	/* 54 */  0x09,  /* 'T' */
	/* 55 */  0x0c,  /* 'U' */
	/* 56 */  0x19,  /* 'V' */
	/* 57 */  0x1a,  /* 'W' */
	/* 58 */  0x1b,  /* 'X' */
	/* 59 */  0x12,  /* 'Y' */
	/* 5a */  0x1d,  /* 'Z' */
	/* 5b */  0,     /* '[' */
	/* 5c */  0,     /* '\' */
	/* 5d */  0,     /* ']' */
	/* 5e */  0,     /* '^' */
	/* 5f */  0x2d,  /* '_' */
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

/* define the queue length (must be a power of two) */
#define QLEN 128
#define QMASK (QLEN-1)

struct q {
	char q[QLEN];
	uint8_t r; // reader pos
	uint8_t w; // writer pos
	uint8_t c; // count
};

static struct q q;

static uint8_t hid_nq(char c) {
	debug("hid_nq(%d)\r\n", c);
	if (q.c < QLEN) {
		q.c++;
		q.q[q.w] = c;
		q.w = (q.w+1) & QMASK;
		return 1;
	}
	return 0;
}
static uint8_t hid_dq(char *c) {
	if (q.c > 0) {
		*c = q.q[q.r];
		q.r = (q.r+1) & QMASK;
		q.c--;
		return 1;
	}
	return 0;
}

/* This is called periodically to let us report keys if we have any */
bool CALLBACK_HID_Device_CreateHIDReport(
	USB_ClassInfo_HID_Device_t* const iface,
	uint8_t* const report_id,
	const uint8_t report_type,
	void* report_data,
	uint16_t* report_size)
{
	USB_KeyboardReport_Data_t* report = (USB_KeyboardReport_Data_t*)report_data;
	uint8_t key_count = 0;
	uint8_t c;
	
	/* report->Modifier = HID_KEYBOARD_MODIFER_LEFTSHIFT; */
	
	while (hid_dq((char*)&c)) {
		debug("hid_dq() => %d -> %d\r\n", c, pgm_read_byte(&ascii2hid[c]));
		report->KeyCode[key_count++] = pgm_read_byte(&ascii2hid[c]);
	}

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

static uint16_t freq = 440;

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

uint8_t rcli(void) {
	uint8_t ret = SREG;
	cli();
	return ret;
}

uint8_t rsei(void) {
	uint8_t ret = SREG;
	sei();
	return ret;
}

void sreg(uint8_t v) {
	SREG = v;
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

void sw_init(void) {
	ms_tick_init();
	ms_tick_register(usb_work, TICK_USB_WORK, 1);
#ifdef INJECT_STR
	ms_tick_register(inject_string, TICK_INJECT_STR, 60000);
#endif /* INJECT_STR */
	ms_tick_register(toggle_port, TICK_TOGGLE_PORT, 1000);
}

/* Configures the board hardware and chip peripherals for the demo's functionality. */
void hw_init()
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
	cw_init(wpm, (cw_dq_cb_t)&hid_nq);
	cw_set_frequency(freq);
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

