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
#include <avr/eeprom.h>
#include "cw-kbd.h"
#include "settings.h"
#include "settings.sig.h"
#include "cw.h"

EEMEM settings_t settings;
PROGMEM settings_with_defaults_t default_settings = {
	.signature = SHA1_SIGNATURE,
	.wpm = 20,
	.keying_mode = keying_mode_bug,
	.left_key = DIT,
	.frequency = 220,
};

/* signature is made by taking the sha1sum of the settings_t struct */
bool settings_valid_signature(void) {
	uint32_t ee_sig = eeprom_read_dword(&settings.signature);
	uint32_t pgm_sig = pgm_read_dword(&default_settings.signature);
	return (ee_sig == pgm_sig);
}

void settings_default(void) {
	uint16_t i;
	uint8_t v;
	for (i=0; i<sizeof(settings_t); i++) {
		v = pgm_read_byte(((PGM_P)&default_settings)+i);
		eeprom_write_byte(((uint8_t*)&settings)+i, v);
	}
	/* clear out the memories */
	for (i=0; i<MEMORY_COUNT; i++) {
		eeprom_write_byte(&settings.memory_repeat[i], 0);
		eeprom_write_byte(&settings.memory[i][0], 0);
	}
}

void settings_init(void) {
	/* check for valid signature */
	if (!settings_valid_signature())
		settings_default();
}

uint8_t settings_get_wpm(void) {
	return eeprom_read_byte(&settings.wpm);
}

void settings_set_wpm(uint8_t wpm) {
	eeprom_update_byte(&settings.wpm, wpm);
}

uint8_t settings_get_keying_mode(void) {
	return (keying_mode_t)eeprom_read_byte(&settings.keying_mode);
}

void settings_set_keying_mode(keying_mode_t mode) {
	eeprom_update_byte((uint8_t *)&settings.keying_mode, (uint8_t)mode);
}

uint16_t settings_get_frequency(void) {
	return eeprom_read_word(&settings.frequency);
}

void settings_set_frequency(uint16_t freq) {
	eeprom_update_word(&settings.frequency, freq);
}

didah_queue_t settings_get_left_key(void) {
	return (didah_queue_t)eeprom_read_byte(&settings.left_key);
}

void settings_set_left_key(didah_queue_t didah) {
	eeprom_update_byte((uint8_t *)&settings.left_key, (uint8_t)didah);
}

void settings_get_memory(uint8_t id, uint8_t *msg) {
	eeprom_read_block(msg, &settings.memory[id][0], MEMORY_LEN);
}

void settings_set_memory(uint8_t id, const uint8_t *msg) {
	eeprom_update_block(msg, &settings.memory[id][0], MEMORY_LEN);
}

uint8_t settings_get_memory_repeat(uint8_t id) {
	return eeprom_read_byte(&settings.memory_repeat[id]);
}

void settings_set_memory_repeat(uint8_t id, const uint8_t freq) {
	eeprom_update_byte(&settings.memory_repeat[id], freq);
}
