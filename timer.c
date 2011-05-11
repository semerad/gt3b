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


// initialize timer 2 used to count seconds
#define TIMER_5MS  (KHZ / 2 * 5)
void timer_init(void) {
    BSET(CLK_PCKENR1, 5);	// enable clock to TIM2
    TIM2_CNTRH = 0;		// start at 0
    TIM2_CNTRL = 0;
    TIM2_PSCR = 1;		// clock / 2
    TIM2_IER = 0b00000001;	// enable update interrupt
    TIM2_ARRH = hi8(TIMER_5MS - 1);	// count till 5ms time
    TIM2_ARRL = lo8(TIMER_5MS - 1);
    TIM2_CR1 = 0b00000101;	// URS-overflow, enable
}


// count seconds from power on
volatile u16 time_sec;
volatile u8  time_5ms;
static u8 menu_delay;		// timer for delay in MENU task


// interrupt every 5ms
@interrupt void timer_interrupt(void) {
    BRES(TIM2_SR1, 0);  // erase interrupt flag

    // increment time from start
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

    // wakeup INPUT task
    awake(INPUT);

    // task MENU delay
    if (menu_delay && !--menu_delay)
	awake(MENU);
}


// delay in task MENU
void delay_menu(u8 len_5ms) {
    menu_delay = len_5ms;
    stop();
    menu_delay = 0;	// MENU task can be awaked from input also, so set no delay for sure
}

