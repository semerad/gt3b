/*
    ppm include file
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


#ifndef _PPM_INCLUDED
#define _PPM_INCLUDED



#include "gt3b.h"



// TIM3 prescalers and multiply (1000x more) values to get raw TIM3 values
//   also SPACE values for 300us space signal
// about 3.5ms max for servo pulse
#define PPM_PSC_SERVO 0x00
#define PPM_MUL_SERVO KHZ
#define PPM_300US_SERVO ((PPM_MUL_SERVO * 3 + 5) / 10)
// about 28.4ms max for sync pulse
#define PPM_PSC_SYNC  0x03
#define PPM_MUL_SYNC  (KHZ >> 3)
#define PPM_300US_SYNC ((PPM_MUL_SYNC * 3 + 5) / 10)


// actual number of channels
extern u8 channels;
extern u8 ppm_channel2;		// next PPM channel to send (0 is SYNC), step 2
extern _Bool ppm_enabled;	// set to 1 when first ppm values were computed
extern u8 ppm_values[];		// as bytes for ppm_interrupt and timer_interrupt

// variables for planning when start frame and when awake CALC
extern u8 ppm_timer;		// timer incremented every 1ms
extern u8 ppm_start;		// when to start servo pulses
extern u8 ppm_calc_awake;	// when to awake CALC task
extern u8 ppm_frame_length;	// last length of ppm frame

// set actual number of channels
extern void ppm_set_channels(u8 n);

// set channel value to microsec01 (in 0.1 microseconds)
extern void ppm_set_value(u8 channel, u16 microsec01);
// macro for converting microseconds to ppm_set_value() microsec01
#define PPM(val)  ((val) * 10)

// after setting each actual channel value, call this to calculate
//   length of sync signal
extern void ppm_calc_sync(void);


#endif

