/**
 *  @copyright
 *    Copyright (C) 2011 Pavel Semerad
 *
 *  @file
 *    $RCSfile: $
 *
 *  @purpose
 *    gt3b alternative firmware - read keys and ADC values
 *
 *  @cfg_management
 *    $Source: $
 *    $Author: $
 *    $Date: $
 *
 *  @comment
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "input.h"
#include "menu.h"
#include "config.h"
#include "calc.h"
#include "lcd.h"
#include "timer.h"



// autorepeat/long press times in 5ms steps
#define BTN_AUTOREPEAT_DELAY	(500 / 5)
#define BTN_AUTOREPEAT_RATE	(70 / 5)
#define ENCODER_FAST_THRESHOLD  10


// INPUT task, called every 5ms
TASK(INPUT, 40);
static void input_loop(void);




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


    // buttons - rows are inputs with pullups and are switched to output
    //           only when reading that row
    IO_IP(B, 4);  // button row B4
    IO_IP(B, 5);  // button row B5
    IO_IP(C, 4);  // button row C4
    IO_IP(D, 3);  // button row D3
    IO_IP(C, 5);  // button col C5
    IO_IP(C, 6);  // button col C6
    IO_IP(C, 7);  // button col C7
    // and set default row values to 1
    BSET(PB_ODR, 4);
    BSET(PB_ODR, 5);
    BSET(PC_ODR, 4);
    BSET(PD_ODR, 3);

    // rotate encoder - connected to Timer1, using encoder mode
    IO_IF(C, 1);		// pin TIM1_CH1
    IO_IF(C, 2);		// pin TIM1_CH2
    BSET(CLK_PCKENR1, 7);	// enable master clock to TMR1
    TIM1_SMCR = 0x01;		// encoder mode 1, count on TI2 edges
    TIM1_CCMR1 = 0x01;		// CC1 is input
    TIM1_CCMR2 = 0x01;		// CC2 is input
    TIM1_ARRH = 0;		// max value
    TIM1_ARRL = 0xff;		// max value
    TIM1_CNTRH = 0;		// start value
    TIM1_CNTRL = 0;		// start value
    TIM1_IER = 0;		// no interrupts
    TIM1_CR2 = 0;
    TIM1_CR1 = 0x01;		// only counter enable

    // init task
    build(INPUT);
    activate(INPUT, input_loop);
}





// read keys, change value only when last 3 reads are the same

// internal state variables
@near static u16 buttons1, buttons2, buttons3;  // last 3 reads
u16 buttons_state;		// actual combined last buttons state
static u8 buttons_autorepeat;	// autorepeat enable for TRIMs and D/R
@near static u8 buttons_timer[12];	// autorepeat/long press buttons timers
static u8 encoder_timer;	// for rotate encoder slow/fast

// variables representing pressed buttons
u16 buttons;
u16 buttons_long;  // >1s press
// variables for ADC values
@near u16 adc_all_last[3], adc_battery_last;
@near u16 adc_battery;
@near u32 adc_battery_filt;


// reset pressed button
void button_reset(u16 btn) {
    buttons &= ~btn;
    buttons_long &= ~btn;
}
void button_reset_nolong(u16 btn) {
    buttons &= ~btn;
}


/**
   @brief
     set autorepeat
   @detailed
     FIXME
   @param[in] btn
     FIXME
 */
void button_autorepeat(u8 btn) {
    buttons_autorepeat = btn;
}





// read key matrix (11 keys)
#define button_row(port, bit, c5, c6, c7) \
    BSET(P ## port ## _DDR, bit); \
    BSET(P ## port ## _CR2, bit); \
    BRES(P ## port ## _ODR, bit); \
    if (!BCHK(PC_IDR, 5))        btn |= c5; \
    if (!BCHK(PC_IDR, 6))        btn |= c6; \
    if (c7 && !BCHK(PC_IDR, 7))  btn |= c7; \
    BSET(P ## port ## _ODR, bit); \
    BRES(P ## port ## _CR2, bit); \
    BRES(P ## port ## _DDR, bit)



/** @brief
 *    FIXME
 *  @detailed
 *    FIXME
 *  @returns
 *  @retval
 *    FIXME
 */
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

    // rotate last buttons
    buttons3 = buttons2;
    buttons2 = buttons1;

    // read actual keys status
    buttons1 = read_key_matrix();

    // add CH3 button, middle state will be only in buttons_state,
    //   not in buttons
    if (adc_ch3_last <= BTN_CH3_LOW)	   buttons1 |= BTN_CH3;
    else if (adc_ch3_last < BTN_CH3_HIGH)  buttons1 |= BTN_CH3_MID;

    // combine last 3 readed buttons
    buttons_state |= buttons1 & buttons2 & buttons3;
    buttons_state &= buttons1 | buttons2 | buttons3;

    // do autorepeat/long_press only when some keys were pressed
    if (buttons_state_last || buttons_state) {
	// key pressed or released, activate backlight
	backlight_on();

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
		    buttons_timer[i] = cg.long_press_delay;
		}
		// do nothing for now not pressed
	    }
	    else {
		// last was pressed
		if (bs) {
		    // now pressed, check long press delay
		    // if already long or not long enought, skip
		    if (!buttons_timer[i] || --buttons_timer[i])  continue;
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

    }


    // add rotate encoder
    if (encoder_timer)  encoder_timer--;
    if (TIM1_CNTRL) {
	// encoder changed
	if ((s8)TIM1_CNTRL >= 0) {
	    // left
	    buttons |= BTN_ROT_L;
	    if (encoder_timer)  buttons_long |= BTN_ROT_L;
	}
	else {
	    // right
	    buttons |= BTN_ROT_R;
	    if (encoder_timer)  buttons_long |= BTN_ROT_R;
	}
	// set it back to default value
	TIM1_CNTRL = 0;
	// init timer
	encoder_timer = ENCODER_FAST_THRESHOLD;
	backlight_on();
    }


    // if some of the keys changed, wakeup MENU task and reset inactivity timer
    if (buttons_last != buttons || buttons_state_last != buttons_state) {
	awake(MENU);
	reset_inactivity_timer();
    }
}





// ADC buffers, last 4 values for each channel
#define ADC_BUFFERS  4
@near u16 adc_buffer0[ADC_BUFFERS];
@near u16 adc_buffer1[ADC_BUFFERS];
@near u16 adc_buffer2[ADC_BUFFERS];
u16 adc_buffer_pos;


// read ADC values
static void read_ADC(void) {
    READ_ADC();
}


// average battery voltage and check battery low
static void update_battery(void) {
    // ignore very low, which means that it is supplied from SWIM connector
    adc_battery_filt = adc_battery_filt * (ADC_BAT_FILT - 1); // splitted - compiler hack
    adc_battery_filt = (adc_battery_filt + (ADC_BAT_FILT / 2)) / ADC_BAT_FILT
		       + adc_battery_last;
    adc_battery = (u16)((adc_battery_filt + (ADC_BAT_FILT / 2)) / ADC_BAT_FILT);

    // start checking battery after 5s from power on
    if (time_sec >= 5) {
	// wakeup task only when something changed
	if (adc_battery > 50 && adc_battery < battery_low_raw) {
	    // bat low
	    if (!menu_battery_low) {
		menu_battery_low = 1;
		awake(MENU);
	    }
	}
	else {
	    // bat OK, but apply some hysteresis to not switch quickly ON/OFF
	    if (menu_battery_low && adc_battery > battery_low_raw + 100) {
		menu_battery_low = 0;
		awake(MENU);
	    }
	}
    }
}


// reset inactivity timer when some steering or throttle applied
static @near u16 last_steering = 30000;  // more that it can be
static @near u16 last_throttle = 30000;  // more that it can be
#define INACTIVITY_DEAD 20
static void check_inactivity(void) {
    if (adc_steering_last < (last_steering - INACTIVITY_DEAD) ||
        adc_steering_last > (last_steering + INACTIVITY_DEAD)) {
	    reset_inactivity_timer();
	    last_steering = adc_steering_last;
	    last_throttle = adc_throttle_last;
    }
    else if (adc_throttle_last < (last_throttle - INACTIVITY_DEAD) ||
	adc_throttle_last > (last_throttle + INACTIVITY_DEAD)) {
	    reset_inactivity_timer();
	    last_steering = adc_steering_last;
	    last_throttle = adc_throttle_last;
    }
}






// read first ADC values
#define ADC_BUFINIT(id) \
    adc_buffer ## id ## [1] = adc_buffer ## id ## [2] = \
	adc_buffer ## id ## [3] = adc_buffer ## id ## [0];
void input_read_first_values(void) {
    // set end channel to 3 again, sometimes after erasing EEPROM from STVP,
    //   calibrate is called, but only steering ADC is changing
    ADC_CSR = 0b00000011;		// end channel 3, no interrupts
    // read initial ADC values
    BSET(ADC_CR1, 0);			// start conversion
    while (!BCHK(ADC_CSR, 7));		// wait for end of conversion
    read_ADC();

    // put initial values to all buffers
    ADC_BUFINIT(0);
    ADC_BUFINIT(1);
    ADC_BUFINIT(2);
    adc_battery = adc_battery_last;
    adc_battery_filt = (u32)adc_battery * ADC_BAT_FILT;
}




// input task, awaked every 5ms
static void input_loop(void) {
    while (1) {
	read_keys();
	check_inactivity();
	update_battery();
	stop();
    }
}

