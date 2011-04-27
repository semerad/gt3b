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



#include "gt3b.h"


// initialise timer 2 used to count seconds
#define TIMER_1S  (KHZ / 64 * 125 - 1)
void timer_init(void) {
    BSET(CLK_PCKENR1, 5);	// enable clock to TIM2
    TIM2_CNTRH = 0;		// start at 0
    TIM2_CNTRL = 0;
    TIM2_PSCR = 9;		// clock / 512
    TIM2_IER = 0b00000001;	// enable update interrupt
    TIM2_ARRH = hi8(TIMER_1S);	// count till 1s time
    TIM2_ARRL = lo8(TIMER_1S);
    TIM2_CR1 = 0b00000101;	// URS-overflow, enable
}


// count seconds from power on
u16 time;

// interrupt every 1s
// increment time
@interrupt void timer_interrupt(void) {
    BRES(TIM2_SR1, 0);  // erase interrupt flag

    time++;
}

