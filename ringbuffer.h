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

#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include <stdint.h>

struct ringbuffer {
	uint8_t *q;
	uint8_t size;
	uint8_t count;
	uint8_t in;
	uint8_t out;
};

#define DECLARE_RINGBUFFER(NAME, SIZE) \
	uint8_t _##NAME##_q[SIZE]; \
	struct ringbuffer NAME = { _##NAME##_q, SIZE, 0, 0, 0 }

uint8_t ringbuffer_peek(struct ringbuffer *rb);
uint8_t ringbuffer_pop(struct ringbuffer *rb);
void ringbuffer_push(struct ringbuffer *rb, uint8_t val);
void ringbuffer_clear(struct ringbuffer *rb);
uint8_t ringbuffer_count(struct ringbuffer *rb);
uint8_t ringbuffer_empty(struct ringbuffer *rb);
uint8_t ringbuffer_full(struct ringbuffer *rb);

#endif /* _RINGBUFFER_H_ */
