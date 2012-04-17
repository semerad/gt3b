/*
    timer - system timer used to count time
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



#include "timer.h"
#include "lcd.h"
#include "buzzer.h"
#include "input.h"
#include "menu.h"
#include "config.h"
#include "ppm.h"
#include "calc.h"



static @near u16 inactivity;



// initialize timer 2 used to count seconds
#define TIMER_5MS  (KHZ / 2 * 5)
#define TIMER_1MS  (KHZ / 2)
void timer_init(void) {
    BSET(CLK_PCKENR1, 5);	// enable clock to TIM2
    TIM2_CNTRH = 0;		// start at 0
    TIM2_CNTRL = 0;
    TIM2_PSCR = 1;		// clock / 2
    TIM2_IER = 0b00000001;	// enable update interrupt
    TIM2_ARRH = hi8(TIMER_1MS - 1);	// count till 1ms time
    TIM2_ARRL = lo8(TIMER_1MS - 1);
    TIM2_CR1 = 0b00000101;	// URS-overflow, enable

    inactivity = 60;		// default value before set to global one
}


// count seconds from power on
volatile u16 time_sec;
volatile u8  time_5ms;
volatile u8  time_1ms;
static u16 menu_delay;		// timer for delay in MENU task


// interrupt every 1ms
@interrupt void timer_interrupt(void) {
    BRES(TIM2_SR1, 0);  // erase interrupt flag

    // read ADC values every 1ms, it had enought time to end conversion
    READ_ADC();

    // process PPM start, CALC awake
    if (ppm_enabled) {
	if (++ppm_timer == ppm_start) {
	    // load values for channel1 to registers and do timer update event
	    TIM3_PSCR = PPM_PSC_SERVO;
	    TIM3_CCR2H = hi8(PPM_300US_SERVO);
	    TIM3_CCR2L = lo8(PPM_300US_SERVO);
	    TIM3_ARRH = ppm_values[2];
	    TIM3_ARRL = ppm_values[3];
	    ppm_channel2 = 4;	// to channel 2 values
	    BSET(TIM3_EGR, 0);	// generate update event
	    BSET(TIM3_CR1, 0);	// enable timer when not running yet
	}
	if (ppm_timer == ppm_calc_awake)
	    awake(CALC);
    }

    // increment 1ms steps
    if (++time_1ms < 5)  return;
    time_1ms = 0;

    // increment time from start in 5ms steps
    if (++time_5ms >= 200) {

	// each 1s
	time_5ms = 0;
	time_sec++;

	// lcd backlight
	if (lcd_bck_on) {
	    if (!--lcd_bck_count) {
		LCD_BCK0;
		lcd_bck_on = 0;
	    }
	}

	// inactivity timer
	if (inactivity && !(--inactivity))
	    buzzer_on(20, 255, BUZZER_MAX);	// expired, buzzer on

    }

    // lcd blink timer
    if (lcd_blink_something) {
	if (++lcd_blink_cnt >= LCD_BLNK_CNT_MAX) {
	    lcd_blink_cnt = 0;
	    lcd_blink_flag = 1;
	    awake(LCD);
	}
	else if (lcd_blink_cnt == LCD_BLNK_CNT_BLANK) {
	    lcd_blink_flag = 1;
	    awake(LCD);
	}
    }

    // buzzer timer
    if (buzzer_running) {
	if (!--buzzer_cnt) {
	    // counted down to 0
	    if (BUZZER_CHK) {
		// was state 1
		BUZZER0;
		if (--buzzer_count) {
		    buzzer_cnt = buzzer_cnt_off;
		}
		else {
		    buzzer_running = 0;
		}
	    }
	    else {
		// was state 0
		BUZZER1;
		buzzer_cnt = buzzer_cnt_on;
	    }
	}
    }

    // menu timers
#define PROCESS_TIMER(tid) \
    if (menu_timer_running & (u8)(1 << tid)) { \
	if (!(time_5ms & 0b00000001)) {  /* only every 10ms */ \
	    if (menu_timer_direction & (u8)(1 << tid)) { \
		/* down timer */ \
		if (menu_timer[tid].hdr) \
		    menu_timer[tid].hdr--; \
		else { \
		    /* hdr is zero */ \
		    if (--menu_timer[tid].sec == 0) { \
			/* trigger alarm */ \
			buzzer_cnt_on = buzzer_cnt = 60; \
			buzzer_cnt_off = 0; \
			buzzer_count = 1; \
			BUZZER1; \
			buzzer_running = 1; \
			/* backlight on */ \
			if (!lcd_bck_on) { \
			    LCD_BCK1; \
			    lcd_bck_count = 5; \
			    lcd_bck_on = 1; \
			} \
			else if (lcd_bck_count < 5)  lcd_bck_count = 5; \
			/* change timer to upcounting, disable up-count alarm */ \
			menu_timer_alarmed |= (u8)(1 << tid); \
			menu_timer_direction &= (u8)~(u8)(1 << tid); \
			menu_timer_alarm[tid] = 0; \
			/* switch main screen */ \
			menu_main_screen = MS_TIMER ## tid ; \
			awake(MENU); \
		    } \
		    else \
			menu_timer[tid].hdr = 99; \
		} \
	    } \
	    else { \
		/* up timer */ \
		if (++menu_timer[tid].hdr == 100) { \
		    menu_timer[tid].hdr = 0; \
		    if (++menu_timer[tid].sec == menu_timer_alarm[tid]) { \
			/* trigger alarm */ \
			buzzer_cnt_on = buzzer_cnt = 60; \
			buzzer_cnt_off = 0; \
			buzzer_count = 1; \
			BUZZER1; \
			buzzer_running = 1; \
			/* backlight on */ \
			if (!lcd_bck_on) { \
			    LCD_BCK1; \
			    lcd_bck_count = 5; \
			    lcd_bck_on = 1; \
			} \
			else if (lcd_bck_count < 5)  lcd_bck_count = 5; \
			/* flag alarm */ \
			menu_timer_alarmed |= (u8)(1 << tid); \
			/* switch main screen */ \
			menu_main_screen = MS_TIMER ## tid ; \
			awake(MENU); \
		    } \
		} \
	    } \
	} \
    }
    PROCESS_TIMER(0);
    PROCESS_TIMER(1);

    // wakeup INPUT task
    awake(INPUT);

    // wakeup MENU task every 40ms when
    // 	 showing battery
    // 	 at calibrate
    // 	 menu timer is displayed and is running
    if ((menu_adc_wakeup || menu_timer_wakeup) && !(time_5ms & 0b00000111))
	awake(MENU);

    // task MENU delay
    if (menu_delay && !--menu_delay)
	awake(MENU);
}


// delay in task MENU - will be interrupted by buttons/ADC
u16 delay_menu(u16 len_5ms) {
    u16 rest;

    menu_delay = len_5ms;
    stop();
    rest = menu_delay;
    menu_delay = 0;	// MENU task can be awaked from input also, so set no delay for sure
    return rest;
}

void delay_menu_always(u8 len_s) {
    u16 delay_time = len_s * 200;
    while (delay_time)
	delay_time = delay_menu(delay_time);
}


// inactivity timer reset
void reset_inactivity_timer(void) {
    // stop beeping when applied previously
    if (cg.inactivity_alarm && !inactivity)  buzzer_off();
    inactivity = cg.inactivity_alarm * 60;
}

