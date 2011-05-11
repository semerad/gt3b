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



#include "input.h"
#include "menu.h"
#include "config.h"



// INPUT task, called every 5ms
TASK(INPUT, 128);
static void input_loop(void);


#define ENCODER_VALUE  30000


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
    TIM1_CNTRH = hi8(ENCODER_VALUE);	// start value
    TIM1_CNTRH = lo8(ENCODER_VALUE);	// start value
    TIM1_CR2 = 0;
    TIM1_CR1 = 0x01;		// only counter enable

    // init task
    build(INPUT);
    activate(INPUT, input_loop);
}





// read keys, change value only when last 3 reads are the same

// internal state variables
static u16 buttons1, buttons2, buttons3;  // last 3 reads
static u16 buttons_state;	// actual combined last buttons state
static u8 buttons_autorepeat;	// autorepeat enable for TRIMs and D/R
static u8 buttons_timer[12];	// autorepeat/long press buttons timers
static u8 encoder_timer;	// for rotate encoder slow/fast
static u8 ENCODER_FAST_THRESHOLD = 10;	// XXX change to #define

// variables representing pressed buttons
u16 buttons;
u16 buttons_long;  // >1s press
// variables for ADC values
u16 adc_all_ovs[3], adc_battery_filt, adc_battery;
u16 adc_all_last[3], adc_battery_last;


// reset pressed button
void button_reset(u16 btn) {
    buttons &= ~btn;
    buttons_long &= ~btn;
}


// set autorepeat
void button_autorepeat(u8 btn) {
    buttons_autorepeat = btn;
}





// read key matrix (11 keys)
#define button_row(odr, bit, c5, c6, c7) \
    BRES(P ## odr ## _ODR, bit); \
    if BCHK(PC_IDR, 5)  btn |= c5; \
    if BCHK(PC_IDR, 6)  btn |= c6; \
    if (c7 && BCHK(PC_IDR, 7))  btn |= c7; \
    BSET(P ## odr ## _ODR, bit)
static u16 read_key_matrix(void) {
    u16 btn = 0;
    button_row(B, 4, BTN_TRIM_LEFT,  BTN_TRIM_CH3_L, BTN_END);
    button_row(B, 5, BTN_TRIM_RIGHT, BTN_TRIM_CH3_R, BTN_BACK);
    button_row(D, 3, BTN_TRIM_FWD,   BTN_DR_R,       0);
    button_row(C, 4, BTN_TRIM_BCK,   BTN_DR_L,       BTN_ENTER);
    return btn;
}


// read all keys
static void read_keys(void) {
    u16 buttons_state_last = buttons_state;
    u16 buttons_last = buttons;
    u16 bit;
    u8 i, lbs, bs;
    u16 enc;

    // rotate last buttons
    buttons3 = buttons2;
    buttons2 = buttons1;

    // read actual keys status
    buttons1 = read_key_matrix();

    // add CH3 button
    if (adc_ch3_last < 50)  buttons1 |= BTN_CH3;

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
		    buttons |= bit;
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
		    buttons |= bit;
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
		buttons |= bit;
		buttons_long |= bit;
	    }
	    else {
		// now not pressed, set as pressed when no long press was applied
		if (!buttons_timer[i])  continue;  // was long before
		buttons |= bit;
	    }
	}
    }


    // add rotate encoder
    if (encoder_timer)  encoder_timer--;
    enc = (TIM1_CNTRH << 8) | TIM1_CNTRL;
    if (enc != ENCODER_VALUE) {
	// encoder changed
	if (enc > ENCODER_VALUE) {
	    // left
	    buttons |= BTN_ROT_L;
	    if (encoder_timer)  buttons_long != BTN_ROT_L;
	}
	else {
	    // right
	    buttons |= BTN_ROT_R;
	    if (encoder_timer)  buttons_long != BTN_ROT_R;
	}
	// set it back to default value
	TIM1_CNTRH = hi8(ENCODER_VALUE);
	TIM1_CNTRL = hi8(ENCODER_VALUE);
	// init timer
	encoder_timer = ENCODER_FAST_THRESHOLD;
    }


    // if some of the keys changed, wakeup MENU task
    if (buttons_last != buttons) {
	awake(MENU);
    }

}





// ADC buffers, last 4 values for each channel
#define ADC_BUFFERS  4
@near static u16 adc_buffer[ADC_BUFFERS][3];
static u8  adc_buffer_pos;


// define missing ADC buffer registers
volatile u16 ADC_DB0R @0x53e0;
volatile u16 ADC_DB1R @0x53e2;
volatile u16 ADC_DB2R @0x53e4;
volatile u16 ADC_DB3R @0x53e6;

// read ADC values and compute sum of last 4 for each channel
#define ADC_NEWVAL(id) \
    adc_all_last[id] = ADC_DB ## id ## R; \
    buf[id] = adc_all_last[id]; \
    adc_all_ovs[id] = adc_buffer[0][id] + adc_buffer[1][id] \
                      + adc_buffer[2][id] + adc_buffer[3][id];
static void read_ADC(void) {
    u16 *buf = adc_buffer[adc_buffer_pos];

    ADC_NEWVAL(0);
    ADC_NEWVAL(1);
    ADC_NEWVAL(2);
    adc_battery_last = ADC_DB3R;

    adc_buffer_pos++;
    adc_buffer_pos &= 0x03;

    BRES(ADC_CSR, 7);		// remove EOC flag
    BSET(ADC_CR1, 0);		// start new conversion

    // average battery voltage and check battery low
    // ignore very low, which means that it is supplied from SWIM connector
    adc_battery_filt = (u16)((u32)adc_battery_filt * 63 / 64)
		       + adc_battery_last;
    adc_battery = adc_battery_filt >> ADC_BATTERY_SHIFT;
    // wakeup task only when something changed
    if (adc_battery > 50 && adc_battery < cg.battery_low) {
	// bat low
	if (!menu_battery_low) {
	    menu_battery_low = 1;
	    awake(MENU);
	}
    }
    else {
	// bat OK, but apply some hysteresis to not switch quickly ON/OFF
	if (menu_battery_low && adc_battery > cg.battery_low + 5) {
	    menu_battery_low = 0;
	    awake(MENU);
	}
    }
    // wakeup task when showing battery
    if (menu_wants_battery || menu_takes_adc)
	awake(MENU);
}






// input task, awaked every 5ms
#define ADC_BUFINIT(id) \
    adc_buffer[1][id] = adc_buffer[2][id] = adc_buffer[3][id] = \
                        adc_buffer[0][id]; \
    adc_all_ovs[id] = adc_buffer[0][id] << ADC_OVS_SHIFT;
static void input_loop(void) {

    // read initial ADC values
    BSET(ADC_CR1, 0);			// start conversion
    while (!BCHK(ADC_CSR, 7))  pause();	// wait for end of conversion
    read_ADC();

    // put initial values to all buffers
    ADC_BUFINIT(0);
    ADC_BUFINIT(1);
    ADC_BUFINIT(2);
    adc_battery = adc_battery_last;
    adc_battery_filt = adc_battery << ADC_BATTERY_SHIFT;

    while (1) {
	// read ADC only when EOC flag (normally will be set)
	if (BCHK(ADC_CSR, 7))
	    read_ADC();
	read_keys();
	stop();
    }
}

