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

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "cw-kbd.h"
#include "cw.h"
#include "util.h"
#include "timer.h"
#include "tick.h"


volatile uint16_t millis;

/*
 * delta_millis
 *
 * this function gives us the difference in milliseconds between
 * two timestamps with the one assumption that the maximum delta
 * will be 2^15-1, or about 32 seconds.
 *
 * it basically returns latter-former, but with the exception of
 * when latter has wrapped around the 2^16-1 horn and is actually
 * smaller than former, then it returns a truncated, double negated
 * former-latter.
 *
 * This results in a range of -2^15 to 2^15-1, or int16_t
 */
int16_t delta_millis(uint16_t latter, uint16_t former) {
	uint16_t ret = latter-former;
	if (ret > ((((uint16_t)1)<<15)-1))
		return -(int16_t)(-ret);
	return (int16_t)ret;
}

static struct tick_event tick_q[TICK_EVENTS];
volatile uint8_t waiting_events;

uint8_t ms_tick_registered(enum tick_events prio) {
	return (tick_q[prio].func != 0);
}

void ms_tick_register(tick_callback_t work, enum tick_events prio, uint16_t freq) {
	ulog("ms_tick_register(%#x, %u, %u): %b\r", work, prio, freq, waiting_events);
	tick_q[prio].freq = freq;
	tick_q[prio].next_fire = millis + freq;
	tick_q[prio].func = work;
	if (work) {
		if (!waiting_events)
			ms_tick_start();
		waiting_events |= _BV(prio);
	} else {
		waiting_events &= ~_BV(prio);
		if (!waiting_events) {
			ms_tick_stop();
		}
	}
}

static void ms_tick(void) {
	enum tick_events i;
	uint16_t pre_ms;
	millis++;
	pre_ms = millis;

	for (i=0; i<TICK_EVENTS; i++) {
		if (tick_q[i].func && tick_q[i].next_fire == millis) {
			tick_q[i].next_fire += tick_q[i].freq;
			tick_q[i].func();
		}
	}
	if (pre_ms != millis) {
		cw_string("eeek!");
	}
}

void ms_tick_init(void) {
	millis = 0;
	memset(tick_q, 0, sizeof(tick_q));
}

void ms_tick_stop(void) {
	ulog("ms_tick_stop\r");
	timer0_set_scale(t8_stopped);
}

void ms_tick_start(void) {
	ulog("ms_tick_start\r");
	timer0_set_compa_callback(&ms_tick);
	timer0_init(t8_divide_by_64, T8_CTC_MODE, T8_INT_OCA);
	timer0_set_top(250); // (16e6 / 64) / 250 == 1000 Hz
}

