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
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdint.h>

/* Format a string */
int my_snvprintf(char *str, uint8_t len, PGM_P format, char **arg) {
	int c;
	char buf[20];
	int ret = 0;
	char *p;

	len--;

	while ((c = pgm_read_byte(format++)) != 0 && ret < len)
	{
		if (c != '%') {
			*str++ = c;
			ret++;
		} else {
altformat:
			c = pgm_read_byte(format++);
			switch (c) {
			case 'c':
				*str++ = *((int16_t *)arg++);
				ret++;
				break;

			case 'i':
			case 'd':
				if (*((uint16_t *)arg) < 0) {
					*str++ = '-';
					ret++;
					if (ret >= len)
						break;
				}
				p = utoa (abs(*((int16_t *) arg++)), buf, 10);
				goto string;
				break;
			case 'b':
			case 'o':
			case 'u':
			case 'x':
				if (c == 'b')
					c = 2;
				else if (c == 'o')
					c = 8;
				else if (c == 'x')
					c = 16;
				else
					c = 10;
				p = utoa (*((uint16_t *) arg++), buf, c);
				goto string;
				break;

			case 'S':
				p = (char *)pgm_read_word(*arg++);
				if (! p) {
nullstr:
					p = PSTR("(null)");
				}

				while (pgm_read_byte(p) && ret < len) {
					*str++ = pgm_read_byte(p++);
					ret++;
				}
				break;

			case 's':
				p = *arg++;
				if (! p)
					goto nullstr;

string:
				while (*p && ret < len) {
					*str++ = *p++;
					ret++;
				}
				break;

			case '#':
				if (ret+2 > len)
					goto out;
				*str++ = '0';
				*str++ = pgm_read_byte(format);
				ret += 2;
				goto altformat;
				break;

			default:
				*str++ = *((char *)arg++);
				ret++;
				break;
			}
		}
	}
out:
	*str = 0;

	return ret;
}

int my_snprintf(char *str, uint8_t len, PGM_P format, ...)
{
	return my_snvprintf(str, len, format, ((char **) &format)+1);
}

