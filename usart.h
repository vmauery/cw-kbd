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

#ifndef _USART_H_
#define _USART_H_

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

typedef void(*usart_read_callback_t)(uint8_t data); 
typedef void(*usart_write_callback_t)(void); 
typedef enum { parity_none=0x00, parity_even=0x02, parity_odd=0x03 } parity_type;

/* initialize the usart receiver and transmitter with a default frame format
 * (default frame format of 8 data bits, 1 stop bit, and no parity)
 * baud: the baud rate to use
 */
void usart_init(uint32_t baud);

/* set the frame format
 *   data: the number of data bits (0x05 to 0x08)
 *   stop: the number of stop bits (0x01 or 0x02)
 * parity: parity type (parity_even, parity_odd, or parity_none)
 */
void usart_format(uint8_t data, uint8_t stop, parity_type parity);

/* enable or disable the reciever
 * val: true=enable, false=disable
 */
void usart_rx_enable(bool val);

/* enable or disable the transmitter
 * val: true=enable, false=disable
 */
void usart_tx_enable(bool val);

/* enable or disable the reciever interrupt
 * val: true=enable, false=disable
 */
void usart_rx_interrupt(bool val);

/* enable or disable the transmitter interrupt
 * val: true=enable, false=disable
 */
void usart_tx_interrupt(bool val);

/* read a byte from the usart
 * (should only be used if read interrupt is disabled)
 * data: where to store the byte read
 */
void usart_read_byte(uint8_t* data);

/* write a byte to the usart
 * data: the byte to write
 */
void usart_write_byte(uint8_t data);

/* read a series of bytes from the usart
 * (buffer must be at least len bytes long)
 * data: the buffer to store the bytes
 *  len: the number of bytes to read
 */
void usart_read(uint8_t* data, uint16_t len);

/* write a series of bytes to the usart
 * (buffer must be at least len bytes long)
 * data: an array of bytes to write 
 *  len: the number of bytes to written
 */
void usart_write(const uint8_t* data, uint16_t len);
void usart_write_p(const uint8_t* data, uint16_t len);

/* write a string to the usart
 * data: a null-terminated string
 */
void usart_print(const char* str);

/* set the read interrupt callback
 * callback: the function pointer to use
 */
void usart_read_callback(usart_read_callback_t callback);

/* set the write interrupt callback
 * callback: the function pointer to use
 */
void usart_write_callback(usart_write_callback_t callback);

#endif
