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




// limit adc value to -5000..5000 (standard servo signal * 10)
static s16 channel_calib(u16 adc_ovs, u16 call, u16 calm, u16 calr, u16 dead) {
    s16 val;
    if (adc_ovs < calm) {
	// left part
	if (adc_ovs < call) adc_ovs = call;		// limit to calib left
	val = (s16)adc_ovs - (s16)(calm - dead);
	if (val >= 0)  return 0;			// in dead zone
	return (s16)((s32)val * 5000 / ((calm - dead) - call));
    }
    else {
	// right part
	if (adc_ovs > calr) adc_ovs = calr;		// limit to calib right
	val = (s16)adc_ovs - (s16)(calm + dead);
	if (val <= 0)  return 0;			// in dead zone
	return (s16)((s32)val * 5000 / (calr - (calm + dead)));
    }
}
// calculate new PPM values from ADC and internal variables
// called for each PPM cycle
static void calc_loop(void) {
    s16 val;

    while (1) {

	// steering
	val = channel_calib(adc_steering_ovs,
			    cg.calib_steering_left << ADC_OVS_SHIFT,
			    cg.calib_steering_mid << ADC_OVS_SHIFT,
			    cg.calib_steering_right << ADC_OVS_SHIFT,
			    cg.steering_dead_zone << ADC_OVS_SHIFT);
	ppm_set_value(1, (u16)(15000 + val));


	// throttle
	val = channel_calib(adc_throttle_ovs,
			    cg.calib_throttle_fwd << ADC_OVS_SHIFT,
			    cg.calib_throttle_mid << ADC_OVS_SHIFT,
			    cg.calib_throttle_bck << ADC_OVS_SHIFT,
			    cg.throttle_dead_zone << ADC_OVS_SHIFT);
	ppm_set_value(2, (u16)(15000 + val));


	// channel 3
	ppm_set_value(3, btns(BTN_CH3) ? 20000 : 10000);


	// sync signal
	ppm_calc_sync();

	// wait for next cycle
	stop();
    }
}

