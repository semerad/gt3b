/*
    input - read keys and ADC values
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



#include "gt3b.h"
#include "input.h"


void input_init(void) {
    // ADC inputs
    IO_IF(B, 0);		// steering ADC
    IO_IF(B, 1);		// throttle ADC
    IO_IF(B, 2);		// CH3 button
    IO_IF(B, 3);		// battery voltage ADC
    // ADC init
    BSET(CLK_PCKENR2, 3);	// enable clock to ADC
    ADC_TDRL = 0b00001111;	// schmitt-trigger disable 0-3
    ADC_CSR = 0b00000011;	// end channel 3, no interrupts
    ADC_CR3 = 0b00000000;	// no-DBUF
    ADC_CR2 = 0b00001010;	// right align, SCAN
    ADC_CR1 = 0b01110001;	// clock/18, no-CONT, enable


    // buttons
    IO_OPF(B, 4);  // button row B4
    IO_OPF(B, 5);  // button row B5
    IO_OPF(C, 4);  // button row C4
    IO_OPF(D, 3);  // button row D3
    IO_IP(C, 5);   // button col C5
    IO_IP(C, 6);   // button col C6
    IO_IP(C, 7);   // button col C7
    // and set default row values
    BSET(PB_ODR, 4);
    BSET(PB_ODR, 5);
    BSET(PC_ODR, 4);
    BSET(PD_ODR, 3);

    // rotate encoder - connected to Timer1, using encoder mode
    IO_IF(C, 1);		// pin TIM1_CH1
    IO_IF(C, 2);		// pin TIM1_CH2
    BSET(CLK_PCKENR1, 7);	// enable master clock to TMR1
    TIM1_SMCR = 0x03;		// encoder mode 3, count on both edges
    TIM1_CCMR1 = 0x01;		// CC1 is input
    TIM1_CCMR2 = 0x01;		// CC2 is input
    TIM1_ARRH = hi8(60000);	// max value
    TIM1_ARRL = lo8(60000);	// max value
    TIM1_CNTRH = hi8(30000);	// start value
    TIM1_CNTRH = lo8(30000);	// start value
    TIM1_CR2 = 0;
    TIM1_CR1 = 0x01;		// only counter enable
}





// read keys, change value only when last 3 reads are the same

// internal state variables
static u16 buttons1, buttons2, buttons3;  // last 3 reads
static u16 buttons_state;	// actual combined last buttons state
static u8 buttons_autorepeat;	// autorepeat enable for TRIMs and D/R
static u8 buttons_timer[12];	// autorepeat/long press buttons timers

// variables representing pressed buttons
u16 btn_press;
u16 btn_press_long;  // >1s press


// reset pressed button
void button_reset(u16 btn) {
    btn_press &= ~btn;
    btn_press_long &= ~btn;
}


// set autorepeat
void button_autorepeat(u8 btn) {
    buttons_autorepeat = btn;
}


// read key matrix (11 keys)
static u16 read_key_matrix(void) {
    return 0;
}


// read all keys - called each 5ms
static void read_keys(void) {
    u16 buttons_state_last = buttons_state;
    u16 btn_press_last = btn_press;
    u16 bit;
    u8 i, lbs, bs;

    // rotate last buttons
    buttons3 = buttons2;
    buttons2 = buttons1;

    // read actual keys status
    buttons1 = read_key_matrix();
    // XXX add CH3 button
    // XXX add rotate encoder

    // combine last 3 readed buttons
    buttons_state |= buttons1 & buttons2 & buttons3;
    buttons_state &= buttons1 | buttons2 | buttons3;

    // handle autorepeat for first 8 keys (TRIMs and D/R)
    if (buttons_autorepeat) {
	for (i = 0, bit = 1; i < 8; i++, bit <<= 1) {
	    if (!(bit & buttons_autorepeat))  continue;  // not autorepeated
	    lbs = (u8)(buttons_state_last & bit ? 1 : 0);
	    bs = (u8)(buttons_state & bit ? 1 : 0);
	    if (!lbs) {
		// last not pressed
		if (bs) {
		    // now pressed, set it pressed and set autorepeat delay
		    btn_press |= bit;
		    buttons_timer[i] = BTN_AUTOREPEAT_DELAY;
		}
		// do nothing for now not pressed
	    }
	    else {
		// last was pressed
		if (bs) {
		    // now pressed
		    if (--buttons_timer[i])  continue;  // not expired yet
		    // timer expired, set it pressed and set autorepeat rate
		    btn_press |= bit;
		    buttons_timer[i] = BTN_AUTOREPEAT_RATE;
		}
		// do nothing for now not pressed
	    }
	}
    }

    // handle long presses for first 12 keys
    // exclude keys with autorepeat ON
    for (i = 0, bit = 1; i < 12; i++, bit <<= 1) {
	if (bit & buttons_autorepeat)  continue;  // handled in autorepeat
	lbs = (u8)(buttons_state_last & bit ? 1 : 0);
	bs = (u8)(buttons_state & bit ? 1 : 0);
	if (!lbs) {
	    // last not pressed
	    if (bs) {
		// now pressed, set long press delay
		buttons_timer[i] = BTN_LONG_PRESS_DELAY;
	    }
	    // do nothing for now not pressed
	}
	else {
	    // last was pressed
	    if (bs) {
		// now pressed, check long press delay
		if (--buttons_timer[i])  continue;  // not long enought yet
		// set as pressed and long pressed
		btn_press |= bit;
		btn_press_long |= bit;
	    }
	    else {
		// now not pressed, set as pressed when no long press was applied
		if (!buttons_timer[i])  continue;  // was long before
		btn_press |= bit;
	    }
	}
    }

    // if some of the keys changed, wakeup DISPLAY task
    if (btn_press_last != btn_press) {
	awake(DISPLAY);
    }
}

