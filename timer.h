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

#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>
#include <avr/io.h>
#include "util.h"

typedef void(*timer_callback)(void);

/*
 * 8-bit timers
 */

typedef enum {
	t8_stopped = 0,
	t8_no_prescaling,
	t8_divide_by_8,
	t8_divide_by_64,
	t8_divide_by_256,
	t8_divide_by_1024,
	t8_external_clock_falling_edge,
	t8_external_clock_rising_edge,
} __attribute__((packed)) clock_select8_t;

/* Waveform Generation Modes
 * does NOT represent all the modes (i.e. 0-15) */
typedef enum {
	T8_NORMAL_MODE = 0,
	T8_CTC_MODE = 2,
	T8_PWM_PC_MODE = 5,
	T8_FPWM_MODE = 7
} __attribute__((packed)) t8_mode_t;

typedef enum {
	T8_INT_OVF = 1,
	T8_INT_OCA = 2,
	T8_INT_OCB = 4,
} __attribute__((packed)) t8_interrupt_t;

/*
 * 16-bit timers
 */

/* Waveform Generation Modes
 * does NOT represent all the modes (i.e. 0-15) */
typedef enum {
	T16_NORMAL_TIMER = 0,
	T16_CTC_OCRNA = 4,
	T16_PWM_PFC = 8,
	T16_CTC_ICRN = 12,
} __attribute__((packed)) t16_mode_t;

/* NOTE: OVERFLOW does NOT makes sense in CTC mode and OUTPUT_COMPARE_MATCH_A is only valid for PWM_PFC mode.
 * NOTE: OUTPUT_COMPARE_MATCH_B and INPUT_CAPTURE are NOT supported.
 */
typedef enum {
	T16_OVERFLOW = 1,
	T16_COMPA = 2,
	T16_COMPB = 4,
	T16_COMPC = 8,
	T16_CAPT = 32,
} __attribute__((packed)) t16_interrupt_t;

typedef enum {
	t16_stopped = 0,
	t16_no_prescaling,
	t16_divide_by_8,
	t16_divide_by_64,
	t16_divide_by_256,
	t16_divide_by_1024,
	t16_external_clock_falling_edge,
	t16_external_clock_rising_edge,
} __attribute__((packed)) clock_select16_t;

/*
 * Common for all timers
 */

typedef enum {
	DISCONNECTED = 0,
	TOGGLE,
	CLEAR,
	SET
} __attribute__((packed)) compare_output_mode_t;

/*
 * Timer 0
 */

/* initialize timer 0
 * clock_select: either no prescaler, a prescalar (8,64,256,1024) or an eternal clock source
nn * enableInterrupts: true enables the interrupt, false disables the interrupt
 * returns false if something went wrong (e.g. the timer is already in use)
 * NOTE: I flag in the Status Register is NOT set in timer0_init.
 */
bool timer0_init(clock_select8_t clock_select, t8_mode_t mode, t8_interrupt_t interrupt_mask);

/* set clock prescaler */
void timer0_set_scale(clock_select8_t scale);

/* reads and returns the value of timer 0
 */
uint8_t timer0_read(void);

/* set the timer 0 value
 * value: 0 to 0xff (255d)
 */
void timer0_write(uint8_t value);

/* set the interrupt for timer 0
 * enable: true enables the interrupt, false disables the interrupt
 * NOTE: I flag in the Status Register is NOT set in timer0_enable_interrupts
 */
void timer0_set_interrupts(t8_interrupt_t mask);

/* set the callback routine to handle user code for an overflow on timer0
 * callback: a function pointer to the callback
 */
void timer0_set_overflow_callback(timer_callback callback);

/* set the callback routine to handle user code for an compa on timer0
 * callback: a function pointer to the callback
 */
void timer0_set_compa_callback(timer_callback callback);

/* set the callback routine to handle user code for an compb on timer0
 * callback: a function pointer to the callback
 */
void timer0_set_compb_callback(timer_callback callback);

/* stop using timer0
 * Sets the Clock Select to 0 for good coders
 */
void timer0_stop(void);

/* sets the correct "top" value (depending on the mode) for timer 0
 * top: 0 to 0xff (255).  The timer will count until this value and then either start again at 0 (CTC mode),
 * or start to decrement until 0 (PWM_PFC modes).
 *
 * NOTE: the top value is NOT used in normal mode
 */
void timer0_set_top(uint8_t top);

/* reads and returns the top value for timer 1
 *
 * NOTE: the top value is NOT used in normal mode
 */
uint8_t timer0_get_top(void);

/*
 * Timer 1
 */

/* initialize timer 1
 * clock_select: either no prescaler, a prescalar (8,64,256,1024) or an eternal clock source
 * mode: normal, CTC or PWM (phase and freq. correct)
 * interrupt_mask: OVERFLOW only makes sense with NORMAL mode, and OUTPUT_COMPARE_MATCH_A with PWM_PFC (mode 8)
 * returns false if something went wrong (e.g. the timer is already in use)
 * NOTE: I flag in the Status Register is NOT set in timer1_init.
 */
bool timer1_init(clock_select16_t clock_select, t16_mode_t mode, t16_interrupt_t interrupts_mask);

/* read and return the value of timer 1
 */
uint16_t timer1_read(void);

/* set the value of the timer 1 counter
 * value: 0 to 0xffff (65,535)
 */
void timer1_write(uint16_t value);

/* set interrupts for timer 1
 * interrupts_mask: see t16_interrupt_t
 * NOTE: I flag in the Status Register is NOT set in timer1_enable_interrupts.
 */
void timer1_set_interrupts(t16_interrupt_t interrupts_mask);

/* set the compare value for timer 1
 * compare: 0 to 0xffff (65,535).  When the timer reaches this value, the output compare flag is set
 * and the interrupt output compare interrupt is trigger (if it is enabled).
 *
 * NOTE: the compare value is NOT used in normal or CTC modes
 */
void timer1_set_compare(uint16_t compare);

/* reads and returns the compare value for timer 1
 *
 * NOTE: the compare value is NOT used in normal or CTC modes
 */
uint16_t timer1_get_compare(void);

/* sets clock prescaler */
void timer1_set_scale(clock_select16_t scale);

/* sets the correct "top" value (depending on the mode) for timer 1
 * top: 0 to 0xffff (65,535).  The timer will count until this value and then either start again at 0 (CTC mode),
 * or start to decrement until 0 (PWM_PFC modes).
 *
 * NOTE: the top value is NOT used in normal mode
 */
void timer1_set_top(uint16_t top);

/* reads and returns the top value for timer 1
 *
 * NOTE: the top value is NOT used in normal mode
 */
uint16_t timer1_get_top(void);

/* Set Compare Output Mode for timer 1
 * compare_output_mode: Specify what happens to OCnx when the timer matches the compare value.
 * NOTE: TOGGLE is probably what you want for a simple PWM.
*/
void timer1_set_compare_output_mode(compare_output_mode_t compare_output_mode);

/* set the callback routine to handle user code for a capture on timer1
 * callback: a function pointer to the callback
 */
void timer1_set_capture_callback(timer_callback callback);

/* set the callback routine to handle user code for a compare (A) on timer1
 * callback: a function pointer to the callback
 */
void timer1_set_compare_a_callback(timer_callback callback);

/* set the callback routine to handle user code for a compare (B) on timer1
 * callback: a function pointer to the callback
 */
void timer1_set_compare_b_callback(timer_callback callback);

/* set the callback routine to handle user code for an overflow on timer1
 * callback: a function pointer to the callback
 */
void timer1_set_overflow_callback(timer_callback callback);

/* stop using timer1
 * Sets the Clock Select to 0 for good coders
 */
void timer1_stop(void);

/*
 * Timer 3
 */

/* initialize timer 3
 * clock_select: either no prescaler, a prescalar (8,64,256,1024) or an eternal clock source
 * mode: normal, CTC or PWM (phase and freq. correct)
 * interrupt_mask: OVERFLOW only makes sense with NORMAL mode, and OUTPUT_COMPARE_MATCH_A with PWM_PFC (mode 8)
 * returns false if something went wrong (e.g. the timer is already in use)
 * NOTE: I flag in the Status Register is NOT set in timer3_init.
 */
bool timer3_init(clock_select16_t clock_select, t16_mode_t mode, t16_interrupt_t interrupts_mask);

/* read and return the value of timer 3
 */
uint16_t timer3_read(void);

/* set the value of the timer 3 counter
 * value: 0 to 0xffff (65,535)
 */
void timer3_write(uint16_t value);

/* set interrupts for timer 3
 * interrupts_mask: see t16_interrupt_t
 * NOTE: I flag in the Status Register is NOT set in timer3_enable_interrupts.
 */
void timer3_set_interrupts(t16_interrupt_t interrupts_mask);

/* set the compare value for timer 3
 * compare: 0 to 0xffff (65,535).  When the timer reaches this value, the output compare flag is set
 * and the interrupt output compare interrupt is trigger (if it is enabled).
 *
 * NOTE: the compare value is NOT used in normal or CTC modes
 */
void timer3_set_compare(uint16_t compare);

/* reads and returns the compare value for timer 3
 *
 * NOTE: the compare value is NOT used in normal or CTC modes
 */
uint16_t timer3_get_compare(void);

/* sets clock prescaler */
void timer3_set_scale(clock_select16_t scale);

/* sets the correct "top" value (depending on the mode) for timer 3
 * top: 0 to 0xffff (65,535).  The timer will count until this value and then either start again at 0 (CTC mode),
 * or start to decrement until 0 (PWM_PFC modes).
 *
 * NOTE: the top value is NOT used in normal mode
 */
void timer3_set_top(uint16_t top);

/* reads and returns the top value for timer 3
 *
 * NOTE: the top value is NOT used in normal mode
 */
uint16_t timer3_get_top(void);

/* Set Compare Output Mode for timer 3
 * compare_output_mode: Specify what happens to OCnx when the timer matches the compare value.
 * NOTE: TOGGLE is probably what you want for a simple PWM.
*/
void timer3_set_compare_output_mode(compare_output_mode_t compare_output_mode);

/* set the callback routine to handle user code for a capture on timer3
 * callback: a function pointer to the callback
 */
void timer3_set_capture_callback(timer_callback callback);

/* set the callback routine to handle user code for a compare (A) on timer3
 * callback: a function pointer to the callback
 */
void timer3_set_compare_a_callback(timer_callback callback);

/* set the callback routine to handle user code for a compare (B) on timer3
 * callback: a function pointer to the callback
 */
void timer3_set_compare_b_callback(timer_callback callback);

/* set the callback routine to handle user code for an overflow on timer3
 * callback: a function pointer to the callback
 */
void timer3_set_overflow_callback(timer_callback callback);

/* stop using timer3
 * Sets the Clock Select to 0 for good coders
 */
void timer3_stop(void);

#endif /* TIMER_H */
