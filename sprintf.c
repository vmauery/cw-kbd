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
#include <stdlib.h>

/* Format a string */
int sprintf(char *str, const char *format, ...)
{
	char **arg = (char **) &format;
	int c;
	char buf[20];
	int ret = 0;
	char *p;

	arg++;

	while ((c = *format++) != 0)
	{
		if (c != '%') {
			*str++ = c;
			ret++;
		} else {
multiformat:
			c = *format++;
			switch (c) {
			case 'c':
				*str++ = *((int *)arg++);
				ret++;
				break;

			case 'd':
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
				p = utoa (*((unsigned int *) arg++), buf, c);
				goto string;
				break;

			case 's':
				p = *arg++;
				if (! p)
					p = "(null)";

string:
				while (*p) {
					*str++ = *p++;
					ret++;
				}
				break;

			case '#':
				*str++ = '0';
				*str++ = 'x';
				ret += 2;
				goto multiformat;
				break;

			default:
				*str++ = *((int *)arg++);
				ret++;
				break;
			}
		}
	}
	*str = 0;

	return ret;
}

