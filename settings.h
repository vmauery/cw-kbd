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

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "cw.h"

void settings_init(void);
uint8_t settings_get_wpm(void);
void settings_set_wpm(uint8_t wpm);
uint8_t settings_get_keying_mode(void);
void settings_set_keying_mode(keying_mode_t mode);
uint16_t settings_get_frequency(void);
void settings_set_frequency(uint16_t freq);
didah_queue_t settings_get_left_key(void);
void settings_set_left_key(didah_queue_t didah);
void settings_get_callsign(uint8_t *call);
void settings_set_callsign(const uint8_t *call);
void settings_get_memory(uint8_t id, uint8_t *msg);
void settings_set_memory(uint8_t id, const uint8_t *msg);

#endif /* _SETTINGS_H_ */
