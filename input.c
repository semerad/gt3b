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


void input_init(void) {
    IO_IF(B, 0);   // steering ADC
    IO_IF(B, 1);   // throttle ADC
    IO_IF(B, 2);   // CH3 button
    IO_IF(B, 3);   // battery voltage ADC

    IO_OPF(B, 4);  // button row B4
    IO_OPF(B, 5);  // button row B5
    IO_OPF(C, 4);  // button row C4
    IO_OPF(D, 3);  // button row D3
    IO_IP(C, 5);   // button col C5
    IO_IP(C, 6);   // button col C6
    IO_IP(C, 7);   // button col C7

    // rotate encoder - connected to Timer1, using encoder mode
    IO_IF(C, 1);   // pin TIM1_CH1
    IO_IF(C, 2);   // pin TIM1_CH2
    BSET(CLK_PCKENR1, 7);     // enable master clock to TMR1
    TIM1_SMCR = 0x03;         // encoder mode 3, count on both edges
    TIM1_CCMR1 = 0x01;        // CC1 is input
    TIM1_CCMR2 = 0x01;        // CC2 is input
    TIM1_ARRH = hi8(60000);   // max value
    TIM1_ARRL = lo8(60000);   // max value
    TIM1_CNTRH = hi8(30000);  // start value
    TIM1_CNTRH = lo8(30000);  // start value
    TIM1_CR2 = 0;
    TIM1_CR1 = 0x01;     // only counter enable
}

