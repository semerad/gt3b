/*
    ppm - generating PPM signal
    Copyright (C) 2011 Pavel Semerad

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/



/*
    use timer3 output compare and overflow to generate PPM signal
    use PWM mode 2 to be zero till OC2 and then one to overflow
    after overflow, set new OC2 and overflow values to preload registers
    also change prescaler for sync signal to enable longer time

    ppm frame is driven by 1ms timer (in timer_interrupt), it start new
    frame with servo pulses and wakeups calc task before end of frame
*/


#include <string.h>
#include "ppm.h"
#include "calc.h"
#include "config.h"


// length of whole frame (frame will actually be shorter, this is safe value
//   to not stop generating PPM signal if something goes wrong)
#define PPM_SAFE_FRAME_LENGTH  25000
// constant sync length in ms
#define PPM_SYNC_LENGTH_MIN   3



// channel variables
u8 channels;			// number of channels
static u8 channels2;		// number of channels * 2 (for compare in ppm_interrupt, it is quicker this way)
u8 ppm_channel2;		// next PPM channel to send (0 is SYNC), step 2
_Bool ppm_enabled;		// set to 1 when first ppm values were computed
// PPM values computed for direct seting to TIM3 registers
// 0 is SYNC pulse length and then channels 1-...
u8 ppm_values[2*(MAX_CHANNELS + 1)];  // as bytes for ppm_interrupt and timer_interrupt

// variables for planning when start frame and when awake CALC
u8 ppm_timer;			// timer incremented every 1ms
u8 ppm_start;			// when to start servo pulses
u8 ppm_end;			// when must last PPM frame end (to not start again before)
u8 ppm_calc_awake;		// when to awake CALC task
u8 ppm_frame_length;		// last length of ppm frame




// set number of channels
void ppm_set_channels(u8 n) {
    if (channels == n)  return;		// did not changed

    // disable PPM generation till new values will not be set
    ppm_enabled = 0;
    BRES(TIM3_CR1, 0);	// disable timer
    BSET(PD_ODR, 0);	// set PPM pin to 1
    // set values for timer wakeups
    ppm_start = ppm_timer;				// not now, CALC will compute new one
    ppm_calc_awake = (u8)(ppm_start + PPM_SYNC_LENGTH_MIN);	// SYNC signal min length
    ppm_end = ppm_calc_awake;
    ppm_enabled = 1;

    channels = n;
    channels2 = (u8)(n << 1);  // also 2* value for compare in ppm_interrupt
}


// initialize PPM pin and timer 3
void ppm_init(void) {
    IO_OP(D, 0);	// PPM output pin, TIM3_CH2

    // initialize timer3 used to generate PPM signal
    BSET(CLK_PCKENR1, 6);     // enable master clock to TIM3
    TIM3_CCMR2 = 0b01111000;  // PWM2, OC2 preload, output mode
    TIM3_CCER1 = 0b00010000;  // polarity active-high, OC2 enable
    TIM3_PSCR = PPM_PSC_SYNC;
    TIM3_IER = 1;             // update interrupt enable
    TIM3_CR1 = 0b10000000;    // auto-reload, URS-all, counter-disable
}




/*
    TIM3 overflow interrupt
    set next prescaler, output compare and overflow values to
      preload registers
*/
@interrupt void ppm_interrupt(void) {
    BRES(TIM3_SR1, 0);	// erase interrupt flag

    if (ppm_channel2) {
	// set servo channel
	TIM3_PSCR = PPM_PSC_SERVO;
	TIM3_CCR2H = hi8(PPM_300US_SERVO);
	TIM3_CCR2L = lo8(PPM_300US_SERVO);
	TIM3_ARRH = ppm_values[ppm_channel2];
	ppm_channel2++;
	TIM3_ARRL = ppm_values[ppm_channel2];
	if (++ppm_channel2 > channels2) {
	    // next to SYNC pulse
	    ppm_channel2 = 0;
	}
	return;
    }

    // set SYNC signal
    TIM3_PSCR = PPM_PSC_SYNC;
    TIM3_CCR2H = hi8(PPM_300US_SYNC);
    TIM3_CCR2L = lo8(PPM_300US_SYNC);
    TIM3_ARRH = ppm_values[0];
    TIM3_ARRL = ppm_values[1];
    ppm_channel2 = 2;  // to first channel (step 2 bytes)
}




// set new value for given servo channel (1-...), value in 0.1usec (for
//   eliminating more rounding errors)
static u32 ppm_microsecs01;
void ppm_set_value(u8 channel, u16 microsec01) {
    ppm_microsecs01 += microsec01;
    // ARR must be set to computed value - 1, that is why we are substracting
    //   5000, it is quicker way to "add 5000" and then substract 1 from result
    *(u16 *)(&ppm_values[(u8)(channel << 1)]) =
	(u16)(((u32)microsec01 * PPM_MUL_SERVO - PPM(500)) / PPM(1000));
}


// calculate length of SYNC signal (substract previous channel values)
// also sets flag for ppm_interrupt to use new values
// also starts TIM3 at first call
void ppm_calc_sync(void) {
    u16 ppm_start_last = ppm_start;
    u16 ppm_end16 = ppm_end;
    u16 ppm_tmp;
    u8  ppm_calc_len;

    // ARR must be set to computed value - 1, that is why we are substracting
    //   5000, it is quicker way to "add 5000" and then substract 1 from result
    *(u16 *)(&ppm_values[0]) =
	(u16)(((PPM(1) * (u32)PPM_SAFE_FRAME_LENGTH - ppm_microsecs01) * PPM_MUL_SYNC
	       - PPM(500)) / PPM(1000));

    // calculate ppm_start and set it
    if (ppm_end16 < ppm_start_last)  ppm_end16 += 256;	// to get linear time
    sim();	// with disabled interrupts, timer_interrupt cannot be called now
    ppm_tmp = ppm_calc_len = ppm_timer;
    if (ppm_tmp < ppm_start_last)  ppm_tmp += 256;	// to get linear time
    if (++ppm_tmp < ppm_end16)  ppm_tmp = ppm_end16;	// cannot start before frame end
    ppm_start = (u8)ppm_tmp;				// set it
    rim();

    // calculate time of processing (from CALC wakeup till now) in ms
    //   (ppm_calc_len now has actual timer value)
    ppm_calc_len -= ppm_calc_awake;
    ppm_calc_len++;

    // calculate frame length and new ppm_end
    ppm_frame_length = (u8)((ppm_microsecs01 + 9999) / 10000);
    if (cg.ppm_sync_frame) {
	// constant frame length
	u8 fl = (u8)(cg.ppm_length + 9);
	ppm_frame_length += PPM_SYNC_LENGTH_MIN;	// minimal frame with SYNC signal
	if (ppm_frame_length < fl)  ppm_frame_length = fl;
    }
    else {
	// constant sync length
	ppm_frame_length += (u8)(cg.ppm_length + 3);
    }
    ppm_end = (u8)(ppm_start + ppm_frame_length);
    ppm_microsecs01 = 0;

    // set new ppm_calc_awake
    ppm_calc_awake = (u8)(ppm_end - ppm_calc_len);
}

