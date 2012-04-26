/*
    input include file
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


#ifndef _INPUT_INCLUDED
#define _INPUT_INCLUDED


#include "gt3b.h"


// buttons IDs
#define BTN_TRIM_LEFT	0x0001
#define BTN_TRIM_RIGHT	0x0002
#define BTN_TRIM_1	0x0003
#define BTN_TRIM_FWD	0x0004
#define BTN_TRIM_BCK	0x0008
#define BTN_TRIM_2	0x000c
#define BTN_TRIM_CH3_L	0x0010
#define BTN_TRIM_CH3_R	0x0020
#define BTN_TRIM_3	0x0030
#define BTN_TRIM_ALL	0x003f

#define BTN_DR_L	0x0040
#define BTN_DR_R	0x0080
#define BTN_DR_ALL	0x00c0

#define BTN_ENTER	0x0100
#define BTN_BACK	0x0200
#define BTN_END		0x0400

#define BTN_CH3		0x0800
#define BTN_CH3_MID	0x1000

#define BTN_ROT_L	0x4000
#define BTN_ROT_R	0x8000
#define BTN_ROT_ALL	0xc000

#define BTN_ALL		0xffff


// limits for BTN_CH3_MID
#define BTN_CH3_LOW	256
#define BTN_CH3_HIGH	768


// variables representing pressed buttons
extern volatile u16 buttons_state;	// actual state of buttons
extern volatile u16 buttons;		// pressed buttons (must be cleared by SW)
extern volatile u16 buttons_long;	// >1s presses buttons
#define btn(mask)	(buttons & (mask))
#define btns(mask)	(buttons_state & (mask))
#define btns_all(mask)	((buttons_state & (mask)) == (mask))
#define btnl(mask)	(buttons_long & (mask))
#define btnl_all(mask)  ((buttons_long & (mask)) == (mask))


// reset pressed button(s)
// do it after used that button in code
extern void button_reset(u16 btn);
#define btnr(mask)  button_reset(mask)
#define btnra()     button_reset(BTN_ALL)
extern void button_reset_nolong(u16 btn);
#define btnr_nolong(mask)  button_reset_nolong(mask)


// set autorepeat
extern void button_autorepeat(u8 btn);





// variables for ADC values

// define missing ADC buffer registers
volatile u16 ADC_DB0R @0x53e0;
volatile u16 ADC_DB1R @0x53e2;
volatile u16 ADC_DB2R @0x53e4;
volatile u16 ADC_DB3R @0x53e6;

// last readed values
extern volatile u16 adc_all_last[3];
#define adc_steering_last  adc_all_last[0]
#define adc_throttle_last  adc_all_last[1]
#define adc_ch3_last       adc_all_last[2]
extern volatile u16 adc_battery_last;

// ADC buffers, last 4 values for each channel
//   average will be computed when used
#define ADC_BUFFERS  4
extern @near u16 adc_buffer0[ADC_BUFFERS];
extern @near u16 adc_buffer1[ADC_BUFFERS];
extern @near u16 adc_buffer2[ADC_BUFFERS];
#define ADC_OVS_SHIFT 2
#define ADC_OVS_ROUND 2
#define adc_steering_ovs   (adc_buffer0[0] + adc_buffer0[1] + adc_buffer0[2] \
			    + adc_buffer0[3])
#define adc_throttle_ovs   (adc_buffer1[0] + adc_buffer1[1] + adc_buffer1[2] \
			    + adc_buffer1[3])
#define adc_ch3_ovs        (adc_buffer2[0] + adc_buffer2[1] + adc_buffer2[2] \
			    + adc_buffer2[3])
#define ADC_OVS(name) \
    ((adc_ ## name ## _ovs + ADC_OVS_ROUND) >> ADC_OVS_SHIFT)

// battery will be filtered more times
extern @near volatile u32 adc_battery_filt;
#define ADC_BAT_FILT  512
extern @near volatile u16 adc_battery;	// adc_battery_filt / ADC_BAT_FILT

// code reading last ADC values
//   retypes to force more optimized code produced by compiler
extern u16 adc_buffer_pos;	// step 2 (skip 16bit values)
#define ADC_NEWVAL(id) \
    *(u16 *)((u8 *)adc_buffer ## id ## + adc_buffer_pos) = \
	adc_all_last[id] = \
	ADC_DB ## id ## R;
#define READ_ADC() \
    ADC_NEWVAL(0); \
    ADC_NEWVAL(1); \
    ADC_NEWVAL(2); \
    adc_battery_last = ADC_DB3R; \
    *((u8 *)&adc_buffer_pos + 1) = (u8)((u8)((u8)adc_buffer_pos + 2) & 7); \
    ADC_CSR = 0b00000011;	/* remove EOC flag, 3 channels */          \
    BSET(ADC_CR1, 0);		// start new conversion





// INPUT task
E_TASK(INPUT);


#endif

