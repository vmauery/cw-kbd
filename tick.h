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

#ifndef _TICK_H_
#define _TICK_H_

#include <stdbool.h>
#include <stdint.h>

typedef void (*tick_callback_t)(void);
struct tick_event {
	tick_callback_t func;
	uint16_t freq;
	uint16_t next_fire;
};
/* yes, this enum is for the entire system
 * but it makes allocation and prioritization
 * much easier.  Static allocation of priorities
 * and callbacks only makes sense in an embedded
 * system
 */
enum tick_events {
	TICK_INT6_DEBOUNCE,
	TICK_CW_PARSE,
	TICK_CW_ADVANCE,
	TICK_USB_WORK,
	TICK_TOGGLE_BIT,
	TICK_INJECT_STR,
	TICK_FAUX_WDT,
	TICK_EVENTS
} __attribute__((packed));

int16_t delta_millis(uint16_t latter, uint16_t former);
uint8_t ms_tick_registered(enum tick_events prio);
void ms_tick_register(tick_callback_t work, enum tick_events prio, uint16_t freq);
void ms_tick_unregister(enum tick_events prio);
void ms_tick_init(void);
void ms_tick_start(void);
void ms_tick_stop(void);

extern volatile uint16_t millis;
extern volatile uint8_t waiting_events;

/* return (micros since last tick)/4 */
static inline uint8_t get_micros(void) {
	return timer0_read();
}


#endif /* _TICK_H_ */

