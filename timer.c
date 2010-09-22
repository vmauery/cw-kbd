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

#include <avr/interrupt.h>
#include "timer.h"

#define TIMER0_ENABLED
//#define TIMER1_ENABLED
#define TIMER3_ENABLED

#ifdef TIMER0_ENABLED
/*
 * Timer 0
 */
static timer_callback timer0_overflow_callback_ = NULL;
static timer_callback timer0_compa_callback_ = NULL;
static timer_callback timer0_compb_callback_ = NULL;

/* Timer 0 Overflow */
ISR( TIMER0_OVF_vect)
{
	if( timer0_overflow_callback_) timer0_overflow_callback_();
}

/* Timer 0 Comp A */
ISR( TIMER0_COMPA_vect)
{
	if( timer0_compa_callback_) timer0_compa_callback_();
}

/* Timer 0 Overflow */
ISR( TIMER0_COMPB_vect)
{
	if( timer0_compb_callback_) timer0_compb_callback_();
}


bool timer0_init(clock_select8_t clock_select, t8_mode_t mode, t8_interrupt_t interrupts_mask)
{
	/*check that the clock = 0 (TCCR0A & 0x7) */
	if( TCCR0B & 0x7 ){
		/* timer already in use */
		return false;
	}
	TCNT0 = 0;
	OCR0A = 0xff;

	TIMSK0 = interrupts_mask; /* don't forget to set the "i" bit in the Status Register */

	TCCR0A = (/*wgm11:10*/mode & 0x03);
	TCCR0B = ((/*wgm13:12*/mode & 0x0C)<<1) | clock_select;
	return true;
}

void timer0_set_scale(clock_select8_t scale) {
	TCCR0B = scale & 0x7;
}

uint8_t timer0_read()
{
	return TCNT0;
}

void timer0_write(uint8_t value)
{
	TCNT0 = value;
}

void timer0_set_interrupts(t8_interrupt_t mask)
{
	TIMSK0 = mask; /* don't forget to set the "i" bit in the Status Register */
}

void timer0_set_overflow_callback(timer_callback callback)
{
	timer0_overflow_callback_= callback;
}

void timer0_set_compa_callback(timer_callback callback)
{
	timer0_compa_callback_ = callback;
}

void timer0_set_compb_callback(timer_callback callback)
{
	timer0_compb_callback_ = callback;
}

void timer0_stop(){
	TCCR0B = 0;
}
void timer0_set_top(uint8_t top)
{
	/* NOTE: the top value is NOT used in normal mode
	 * NOTE: only those modes specified in t8_mode_t are implemented
	 */

	OCR0A = top;
}

uint8_t timer0_get_top()
{
	/* NOTE: the top value is NOT used in normal mode
	*/
	return OCR0A;
}

#endif

/*
 * Timer 1
 */
#ifdef TIMER1_ENABLED

static t16_mode_t timer1_mode_value = 0;

static timer_callback timer1_capture_callback_ = NULL;
static timer_callback timer1_compare_a_callback_ = NULL;
static timer_callback timer1_compare_b_callback_ = NULL;
static timer_callback timer1_overflow_callback_ = NULL;

/* Timer 1 Capture event */
ISR( TIMER1_CAPT_vect )
{
	if( timer1_capture_callback_) timer1_capture_callback_();
}

/* Timer 1 Compare match A */
ISR( TIMER1_COMPA_vect )
{
	if( timer1_compare_a_callback_) timer1_compare_a_callback_();
}


/* Timer 1 Compare match B */
ISR( TIMER1_COMPB_vect )
{
	if( timer1_compare_b_callback_) timer1_compare_b_callback_();
}

/* Timer 1 Overflow */
ISR( TIMER1_OVF_vect)
{
	if( timer1_overflow_callback_) timer1_overflow_callback_();
}


bool timer1_init(clock_select16_t clock_select, t16_mode_t mode, t16_interrupt_t interrupts_mask)
{
	timer1_mode_value = mode;

	set_reg_16(ICR1, 65535);
	timer1_set_compare(65535);

	TCCR1A = (/*wgm11:10*/mode & 0x03);
	TCCR1B = ((/*wgm13:12*/mode & 0x0C)<<1) | clock_select;

	TIMSK1 = interrupts_mask & 0x2f; /* don't forget to set the "i" bit in the Status Register */

	timer1_write( 0 );

	return true;
}

void timer1_set_scale(clock_select16_t scale) {
	TCCR1B = (TCCR1B & 0xf8) | scale;
}

uint16_t timer1_read()
{
	return get_reg_16(TCNT1);
}

void timer1_write(uint16_t value)
{
	set_reg_16(TCNT1, value);
}

void timer1_set_interrupts(t16_interrupt_t interrupts_mask)
{
	TIMSK1 = (TIMSK1 & 0x2f) | (interrupts_mask & 0x2f); /* don't forget to set the "i" bit in the Status Register */
}

void timer1_set_compare(uint16_t compare)
{
	/* NOTE: the compare value is NOT used in normal or CTC modes */
	set_reg_16(OCR1A, compare);
}

uint16_t timer1_get_compare()
{
	/* NOTE: the compare value is NOT used in normal or CTC modes */
	return get_reg_16(OCR1A);
}

void timer1_set_top(uint16_t top)
{
	/* NOTE: the top value is NOT used in normal mode
	 * NOTE: only those modes specified in t16_mode_t are implemented
	 */

	set_reg_16(OCR1A, top);
}

uint16_t timer1_get_top()
{
	/* NOTE: the top value is NOT used in normal mode
	*/
	return get_reg_16(OCR1A);
}

void timer1_set_compare_output_mode(compare_output_mode_t compare_output_mode)
{
	TCCR1A = (TCCR1A & 0x3f) | ( compare_output_mode << 6 );
}

void timer1_set_capture_callback(timer_callback callback)
{
	timer1_capture_callback_ = callback;
}

void timer1_set_compare_a_callback(timer_callback callback)
{
	timer1_compare_a_callback_ = callback;
}

void timer1_set_compare_b_callback(timer_callback callback)
{
	timer1_compare_b_callback_ = callback;
}

void timer1_set_overflow_callback(timer_callback callback){
	timer1_overflow_callback_ = callback;
}

void timer1_stop(){
	TCCR1B &= 0xf8;
}
#endif

/*
 * Timer 3
 */
#ifdef TIMER3_ENABLED

static t16_mode_t timer3_mode_value = 0;

static timer_callback timer3_capture_callback_ = NULL;
static timer_callback timer3_compare_a_callback_ = NULL;
static timer_callback timer3_compare_b_callback_ = NULL;
static timer_callback timer3_overflow_callback_ = NULL;

/* Timer 3 Capture event */
ISR( TIMER3_CAPT_vect )
{
	if( timer3_capture_callback_) timer3_capture_callback_();
}

/* Timer 3 Compare match A */
ISR( TIMER3_COMPA_vect )
{
	if( timer3_compare_a_callback_) timer3_compare_a_callback_();
}


/* Timer 3 Compare match B */
ISR( TIMER3_COMPB_vect )
{
	if( timer3_compare_b_callback_) timer3_compare_b_callback_();
}

/* Timer 3 Overflow */
ISR( TIMER3_OVF_vect)
{
	if( timer3_overflow_callback_) timer3_overflow_callback_();
}


bool timer3_init(clock_select16_t clock_select, t16_mode_t mode, t16_interrupt_t interrupts_mask)
{
	timer3_mode_value = mode;

	set_reg_16(ICR3, 65535);
	timer3_set_compare(65535);

	TCCR3A = (/*wgm11:10*/mode & 0x03);
	TCCR3B = ((/*wgm13:12*/mode & 0x0C)<<1) | clock_select;

	TIMSK3 = interrupts_mask & 0x2f; /* don't forget to set the "i" bit in the Status Register */

	timer3_write( 0 );

	return true;
}

void timer3_set_scale(clock_select16_t scale) {
	TCCR3B = (TCCR3B & 0xf8) | scale;
}

uint16_t timer3_read()
{
	return get_reg_16(TCNT3);
}

void timer3_write(uint16_t value)
{
	set_reg_16(TCNT3, value);
}

void timer3_set_interrupts(t16_interrupt_t interrupts_mask)
{
	TIMSK3 = (TIMSK3 & 0x2f) | (interrupts_mask & 0x2f); /* don't forget to set the "i" bit in the Status Register */
}

void timer3_set_compare(uint16_t compare)
{
	/* NOTE: the compare value is NOT used in normal or CTC modes */
	set_reg_16(OCR3A, compare);
}

uint16_t timer3_get_compare()
{
	/* NOTE: the compare value is NOT used in normal or CTC modes */
	return get_reg_16(OCR3A);
}

void timer3_set_top(uint16_t top)
{
	/* NOTE: the top value is NOT used in normal mode
	 * NOTE: only those modes specified in t16_mode_t are implemented
	 */

	set_reg_16(OCR3A, top);
}

uint16_t timer3_get_top()
{
	/* NOTE: the top value is NOT used in normal mode
	*/
	return get_reg_16(OCR3A);
}

void timer3_set_compare_output_mode(compare_output_mode_t compare_output_mode)
{
	TCCR3A = (TCCR3A & 0x3f) | ( compare_output_mode << 6 );
}

void timer3_set_capture_callback(timer_callback callback)
{
	timer3_capture_callback_ = callback;
}

void timer3_set_compare_a_callback(timer_callback callback)
{
	timer3_compare_a_callback_ = callback;
}

void timer3_set_compare_b_callback(timer_callback callback)
{
	timer3_compare_b_callback_ = callback;
}

void timer3_set_overflow_callback(timer_callback callback){
	timer3_overflow_callback_ = callback;
}

void timer3_stop(){
	TCCR3B &= 0xf8;
}
#endif
