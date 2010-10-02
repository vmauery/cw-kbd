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

#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// project/system dependent defines

#include <util/delay.h>

#define CYCLES_PER_US ((F_CPU+500000)/1000000) 	// cpu cycles per microsecond
#define UART_BAUD_RATE   19200
#define UART_BAUD_SEL    (F_CPU/(UART_BAUD_RATE*16L)-1)

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned int u16;
typedef signed int s16;
typedef unsigned long u32;
typedef signed long s32;

#ifndef MAX
#define MAX(A,B) (((A)>(B))?(A):(B))
#endif

#ifndef MIN
#define MIN(A,B) (((A)<(B))?(A):(B))
#endif

#ifndef ABS
#define ABS(x)				((x>0)?(x):(-x))
#endif

#define __ccat(a,b) a##b
#define _PORT(x)  __ccat(PORT, x)
#define _PIN(x)   __ccat(PIN, x)
#define _DDR(x)   __ccat(DDR, x)

// macro for reading 16bit words from program memory
#ifndef PRG_RDW
#define PRG_RDW(a)		( (PRG_RDB((unsigned char*)(a)) & 0x00ff) | ((PRG_RDB((unsigned char*)(a)+1))<<8) )
#endif

// constants
#define PI		3.14159265359

static inline void msleep(uint16_t msecs)
{
	while (msecs-- > 0) {
		_delay_ms(1);
	}
}

/* interrupt stuff */
static inline uint8_t rcli(void) {
	uint8_t ret = SREG;
	cli();
	return ret;
}

static inline uint8_t rsei(void) {
	uint8_t ret = SREG;
	sei();
	return ret;
}

static inline void sreg(uint8_t v) {
	SREG = v;
}


/*
 * atomic 16 bit register access
 */
#define get_reg_16(REG) \
({                      \
    uint8_t sreg;       \
    uint16_t i;         \
    sreg = SREG;        \
    __asm__ ("cli");    \
    i = REG;            \
    SREG = sreg;        \
    i;\
})

#define set_reg_16(REG,VAL) \
({                          \
    uint8_t sreg;           \
    sreg = SREG;            \
    __asm__ ("cli");        \
    REG = VAL;              \
    SREG = sreg;            \
})


#endif // UTIL_H
