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

#include <util/atomic.h>
#include "ringbuffer.h"

/* #define RB_DEBUG */
#ifdef RB_DEBUG
#include "cw-kbd.h"

void ringbuffer_dump(const char *str, const struct ringbuffer *rb) {
	uint8_t i, j;
	debug("%s: %d items [", str, rb->count);
	for (j=0; j<rb->count; j++) {
		i = (rb->out + j) % rb->size;
		debug("%d, ", rb->q[i]);
	}
	debug("]\r\n");
}
#else
#define ringbuffer_dump(A...)
#endif /* RB_DEBUG */

uint8_t ringbuffer_peek(struct ringbuffer *rb) {
	uint8_t out;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (rb->count > 0)
			out = rb->q[rb->out];
		else
			out = 0;
	}
	return out;
}

uint8_t ringbuffer_pop(struct ringbuffer *rb) {
	uint8_t out;
	ringbuffer_dump("pop-pre ", rb);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (rb->count > 0) {
			out = rb->q[rb->out];
			if (++rb->out == rb->size)
				rb->out = 0;

			rb->count--;
		} else {
			out = 0;
		}
	}
	ringbuffer_dump("pop-post", rb);
	return out;
}

void ringbuffer_push(struct ringbuffer *rb, uint8_t val) {
	ringbuffer_dump("push-pre ", rb);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		rb->q[rb->in] = val;
		if (++rb->in == rb->size)
			rb->in = 0;

		if (++rb->count > rb->size) {
			rb->count = rb->size;
			if (++rb->out == rb->size)
				rb->out = 0;
		}
	}
	ringbuffer_dump("push-post", rb);
}

uint8_t ringbuffer_count(struct ringbuffer *rb) {
	return rb->count;
}

uint8_t ringbuffer_empty(struct ringbuffer *rb) {
	return rb->count == 0;
}

uint8_t ringbuffer_full(struct ringbuffer *rb) {
	return rb->count == rb->size;
}
