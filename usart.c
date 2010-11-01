/*
 * ex: set syntax=c tabstop=8 noexpandtab shiftwidth=8:
 *
 * onekay is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as  published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * onekay is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with onekay.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright © 2010, Vernon Mauery (N7OH)
 */

#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <stdarg.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "cw-kbd.h"
#include "usart.h"

static usart_read_callback_t usart_read_callback_;
static usart_write_callback_t usart_write_callback_;

#define min(A,B) ((A)<(B)?(A):(B))
#define max(A,B) ((A)>(B)?(A):(B))

ISR(USART1_RX_vect)
{
	uint8_t data;
	while (!(UCSR1A & _BV(RXC1)));
	data = UDR1;
	if (usart_read_callback_) usart_read_callback_(data);
}

void usart_init(uint32_t baud)
{
	uint8_t sreg = SREG;
	cli();

	/* Set baud rate */
	baud = (uint16_t)((uint32_t)F_CPU/(baud<<3)-1);
	UBRR1H = (uint8_t)(baud << 8);
	UBRR1L = (uint8_t)baud;
	UCSR1A |= _BV(U2X1);
	/* Enable Receiver and Transmitter by default */
	usart_rx_enable(true);
	usart_tx_enable(true);
	usart_rx_interrupt(false);
	usart_tx_interrupt(false);

	/* Set frame format: 8 data bits, 1 stop bit, no parity */
	usart_format(8, 1, parity_none);
	SREG = sreg;
}

void usart_format(uint8_t data, uint8_t stop, parity_type parity)
{
	uint8_t ucsz, usbs, upm;
	uint8_t sreg = SREG;
	cli();

	/* convert to the register bit values and make sure they are in bounds */
	ucsz = 0x03&min(max(data-5, 0), 3);
	usbs = 0x01&min(max(stop-1, 0), 1);
	upm = 0x03&parity;
	/* write the frame format */
	UCSR1C = (ucsz<<UCSZ10)|(usbs<<USBS1)|(parity<<UPM10); 

	SREG = sreg; 
}

void usart_rx_enable(bool val)
{
	uint8_t sreg = SREG;
	cli();
	if (val) UCSR1B |= _BV(RXEN1);
	else UCSR1B &= ~_BV(RXEN1);
	SREG = sreg; 
}

void usart_tx_enable(bool val)
{
	uint8_t sreg = SREG;
	cli();
	if (val) UCSR1B |= _BV(TXEN1);
	else UCSR1B &= ~_BV(TXEN1);
	SREG = sreg; 
}

void usart_rx_interrupt(bool val)
{
	uint8_t sreg = SREG;
	cli();
	if (val) UCSR1B |= _BV(RXCIE1);
	else UCSR1B &= ~_BV(RXCIE1);
	SREG = sreg; 
}

void usart_tx_interrupt(bool val)
{
	uint8_t sreg = SREG;
	cli();
	if (val) UCSR1B |= _BV(TXCIE1);
	else UCSR1B &= ~_BV(TXCIE1);
	SREG = sreg; 
}

void usart_read_byte(uint8_t* data)
{
	while (!(UCSR1A & _BV(RXC1))); 
	*data = UDR1; 
}

void usart_write_byte(uint8_t data)
{
	while (!(UCSR1A & _BV(UDRE1)));
	UDR1 = data;
}

void usart_read(uint8_t* data, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; i++)
	{
		usart_read_byte(&data[i]);
	}
}

void usart_write(const uint8_t* data, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; i++)
	{
		usart_write_byte(data[i]);
	}
}

void usart_write_p(const uint8_t* data, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; i++)
	{
		usart_write_byte(pgm_read_byte(&data[i]));
	}
}

void usart_print(const char* str)
{
	while (str && *str)
	{
		usart_write_byte(*str++);
	}
}

void usart_read_callback(usart_read_callback_t callback)
{
	if (!callback)
		usart_rx_interrupt(false);
	usart_read_callback_ = callback;
	if (callback)
		usart_rx_interrupt(true);
}

void usart_write_callback(usart_write_callback_t callback)
{
	if (!callback)
		usart_tx_interrupt(false);
	usart_write_callback_ = callback;
	if (callback)
		usart_tx_interrupt(true);
}

