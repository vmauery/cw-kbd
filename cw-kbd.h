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

#ifndef _CW_KBD_H_
#define _CW_KBD_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include "descriptors.h"

#include <LUFA/Version.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/HID.h>

void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_UnhandledControlRequest(void);
bool CALLBACK_HID_Device_CreateHIDReport(
	USB_ClassInfo_HID_Device_t* const iface,
	uint8_t* const report_id,
	const uint8_t report_type,
	void* report_data,
	uint16_t* report_size);

void power_down(void);
void idle(void);

#ifdef DEBUG

#include "usart.h"

void debug_write_bytes(const char *msg);
void debug_write_byte(const char b);
extern uint8_t debug_write;

int my_snprintf(char *str, uint8_t len, PGM_P format, ...);
int my_vsnprintf(char *str, uint8_t len, PGM_P format, va_list ap);

void _debug(PGM_P fmt,...);
#define debug(A,B...) _debug(PSTR(A), ##B)

#define debug_byte(B) debug_write_byte(B)

void _ulog(PGM_P fmt,...);
#define ulog(A,B...) _ulog(PSTR(A), ##B)
#define ulog_byte(B) usart_write_byte(B)

#else

#define debug(A,B...) do {} while(0)
#define debug_byte(B) do {} while(0)

#define ulog(A,B...) do {} while(0)
#define ulog_byte(B) do {} while(0)

#endif /* DEBUG */

#endif
