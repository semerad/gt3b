/*
    calc - calculate values of ppm signal
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



#include "calc.h"
#include "menu.h"
#include "ppm.h"
#include "config.h"
#include "input.h"



// CALC task
TASK(CALC, 256);
static void calc_loop(void);


// initialize CALC task
void calc_init(void) {
    build(CALC);
    activate(CALC, calc_loop);
    sleep(CALC);	// nothing to do yet
}




// calculate new PPM values from ADC and internal variables
// called for each PPM cycle
static void calc_loop(void) {
    s16 val;
    s32 val2;

    // steering
    val2 = (s32)(adc_steering_ovs - (cg.calib_steering_mid << ADC_OVS_SHIFT))
	   * (5000 / 4);
    if (adc_steering_ovs < (cg.calib_steering_mid << ADC_OVS_SHIFT)) {
	// left turn
	val = (s16)(val2 / (cg.calib_steering_mid - cg.calib_steering_left));
    }
    else {
	// right turn
	val = (s16)(val2 / (cg.calib_steering_right - cg.calib_steering_mid));
    }
    ppm_set_value(1, (u16)(15000 + val));


    // throttle
    val2 = (s32)(adc_throttle_ovs - (cg.calib_throttle_mid << ADC_OVS_SHIFT))
	   * (5000 / 4);
    if (adc_throttle_ovs < (cg.calib_throttle_mid << ADC_OVS_SHIFT)) {
	// forward
	val = (s16)(val2 / (cg.calib_throttle_mid - cg.calib_throttle_fwd));
    }
    else {
	// back
	val = (s16)(val2 / (cg.calib_throttle_bck - cg.calib_throttle_mid));
    }
    ppm_set_value(2, (u16)(15000 + val));


    // channel 3
    ppm_set_value(3, btns(BTN_CH3) ? 20000 : 10000);


    // sync signal
    ppm_calc_sync();
}

