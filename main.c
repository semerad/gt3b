/*
    main - main program, initialize and start
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
#include "timer.h"



// init functions from other source files
extern void ppm_init(void);
extern void lcd_init(void);
extern void input_init(void);
extern void buzzer_init(void);
extern void timer_init(void);
extern void task_init(void);



// switch to clock from external crystal
static void clock_init(void) {
    CLK_PCKENR1 = 0;	// disable clocks to peripherals
    CLK_PCKENR2 = 0;
    BSET(CLK_ECKCR, 0);	// enable HSE
    while (!(CLK_ECKCR & 0x02)) {}  // wait for HSE ready
    CLK_CKDIVR = 0;	// no clock divide
    BSET(CLK_SWCR, 1);	// start switching clock
    CLK_SWR = 0xB4;	// select HSE
    while (!(CLK_SWCR & 0x08)) {}  // wait for clock switch
    if (CLK_CMSR != 0xB4) {
	// switch to HSE NOT done, stop
	while (1) {}
    }
    BSET(CLK_CSSR, 0);	// clock security system on
}





// main program
void main(void) {
    clock_init();
    task_init();
    //ppm_init();
    //buzzer_init();
    //input_init();
    timer_init();

    rim();

    lcd_init();  // with interrupts enabled, because of using timer 4



    {
	u16 last_time = time_sec;
	IO_OP(D, 0);

	while (1) {
	    while (last_time == time_sec)  pause();
	    BSET(PD_ODR, 0);
	    last_time = time_sec;
	    while (last_time == time_sec)  pause();
	    BRES(PD_ODR, 0);
	    last_time = time_sec;
	}
    }


#ifdef PPM_TEST
    // test 8 channels, switch values each 5 seconds
    ppm_set_channels(8);
    {
    extern volatile _Bool ppm_update_values;
    u8 i, j;
    while (1) {
	for (i = 0; i < 222; i++) {
	    for (j = 1; j <= channels; j++) {
		ppm_set_value(j, 10000);
	    }
	    ppm_calc_sync();
	    while (ppm_update_values) {}  // wait for interrupt routine
	}
	for (i = 0; i < 222; i++) {
	    for (j = 1; j <= channels; j++) {
		ppm_set_value(j, 20000);
	    }
	    ppm_calc_sync();
	    while (ppm_update_values) {}  // wait for interrupt routine
	}
    }
    }
#endif
}

