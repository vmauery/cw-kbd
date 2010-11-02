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
#include <stdint.h>
#include <stdbool.h>
#include "cw-kbd.h"
#include "settings.h"
#include "settings.sig.h"
#include "cw.h"

EEMEM settings_t settings;
PROGMEM struct default_preset default_settings = {
	.signature = SHA1_SIGNATURE,
	.wpm = 20,
	.keying_mode = keying_mode_bug,
	.left_key = DIT,
	.frequency = 220,
	.beeper = true,
};
static uint8_t cp;

/* signature is made by taking the sha1sum of the settings_t struct */
bool settings_valid_signature(void) {
	uint32_t ee_sig = eeprom_read_dword(&settings.signature);
	uint32_t pgm_sig = pgm_read_dword(&default_settings.signature);
	return (ee_sig == pgm_sig);
}

void settings_choose_sanity(void) {
	uint8_t i, v;
	for (i=0; i<sizeof(struct preset); i++) {
		v = pgm_read_byte(((PGM_P)&default_settings)+i+4);
		eeprom_update_byte(((uint8_t*)&settings.presets[cp])+i, v);
	}
}

void settings_default(void) {
	uint8_t i, j;
	eeprom_update_dword(&settings.signature,
		pgm_read_dword(&default_settings.signature));
	j = cp;
	for (i=0; i<MEMORY_COUNT; i++) {
		cp = i;
		settings_choose_sanity();
	}
	cp = j;
	eeprom_update_byte(&settings.current_preset, 0);
	/* clear out the memories */
	for (i=0; i<MEMORY_COUNT; i++) {
		eeprom_update_byte(&settings.memory_repeat[i], 0);
		for (j=0; j<MEMORY_LEN; j++)
			eeprom_update_byte(&settings.memory[i][j], 0);
	}
}

void settings_init(void) {
	/* check for valid signature */
	if (!settings_valid_signature())
		settings_default();
	cp = settings_get_preset();
}

uint8_t settings_get_wpm(void) {
	return eeprom_read_byte(&settings.presets[cp].wpm);
}

void settings_set_wpm(uint8_t wpm) {
	eeprom_update_byte(&settings.presets[cp].wpm, wpm);
}

uint8_t settings_get_keying_mode(void) {
	return (keying_mode_t)eeprom_read_byte(&settings.presets[cp].keying_mode);
}

void settings_set_keying_mode(keying_mode_t mode) {
	eeprom_update_byte((uint8_t *)&settings.presets[cp].keying_mode, (uint8_t)mode);
}

uint16_t settings_get_frequency(void) {
	return eeprom_read_word(&settings.presets[cp].frequency);
}

void settings_set_frequency(uint16_t freq) {
	eeprom_update_word(&settings.presets[cp].frequency, freq);
}

didah_queue_t settings_get_left_key(void) {
	return (didah_queue_t)eeprom_read_byte(&settings.presets[cp].left_key);
}

void settings_set_left_key(didah_queue_t didah) {
	eeprom_update_byte((uint8_t *)&settings.presets[cp].left_key, (uint8_t)didah);
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

bool settings_get_beeper(void) {
	return (bool)eeprom_read_byte((uint8_t*)&settings.presets[cp].beeper);
}

void settings_set_beeper(bool beep) {
	eeprom_update_byte((uint8_t*)&settings.presets[cp].beeper, (uint8_t)beep);
}

uint8_t settings_get_preset(void) {
	return eeprom_read_byte(&settings.current_preset);
}

void restore_preset(uint8_t pid) {
	if (pid > 9)
		return;
	cp = pid;
	eeprom_update_byte(&settings.current_preset, pid);
	cw_set_speed(settings_get_wpm());
	cw_set_keying_mode(settings_get_keying_mode());
	cw_set_left_key(settings_get_left_key());
	cw_set_frequency(settings_get_frequency());
}

#ifdef DEBUG
void settings_dump(void) {
	uint8_t i;
	uint32_t sig = eeprom_read_dword(&settings.signature);
	uint8_t msg[MEMORY_LEN];
	ulog("signature: %#x%x\r", (uint16_t)(sig >> 16), (uint16_t)(sig & 0xffff));
	_delay_ms(1);
	ulog("current_preset: %u\r", eeprom_read_byte(&settings.current_preset));
	_delay_ms(1);
	for (i=0; i<MEMORY_COUNT; i++) {
		ulog("preset %u:\r", i);
		_delay_ms(1);
		ulog("  wpm: %u\r", eeprom_read_byte(&settings.presets[i].wpm));
		_delay_ms(1);
		ulog("  key mode: %u\r", eeprom_read_byte(&settings.presets[i].keying_mode));
		_delay_ms(1);
		ulog("  left key: %u\r", eeprom_read_byte(&settings.presets[i].left_key));
		_delay_ms(1);
		ulog("  freq: %u\r", eeprom_read_word(&settings.presets[i].frequency));
		_delay_ms(1);
		ulog("  beeper: %u\r", eeprom_read_byte((uint8_t*)&settings.presets[i].beeper));
		_delay_ms(1);
	}
	for (i=0; i<10; i++) {
		settings_get_memory(i, msg);
		ulog("memory %u (r %u)\r", i, eeprom_read_byte(&settings.memory_repeat[i]));
		_delay_ms(1);
		ulog("  [%s]\r", msg);
		_delay_ms(10);
	}
}
#endif /* DEBUG */
