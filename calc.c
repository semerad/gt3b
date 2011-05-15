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
static s16 channel_calib(u16 adc_ovs, u16 call, u16 calm, u16 calr,
                         u16 dead, s16 trim) {
    s16 val;
    if (adc_ovs < calm) {
	// left part
	if (adc_ovs < call) adc_ovs = call;		// limit to calib left
	val = (s16)adc_ovs - (s16)(calm - dead);
	if (val >= 0)  return trim;			// in dead zone
	return (s16)((s32)val * (5000 + trim) / ((calm - dead) - call)) + trim;
    }
    else {
	// right part
	if (adc_ovs > calr) adc_ovs = calr;		// limit to calib right
	val = (s16)adc_ovs - (s16)(calm + dead);
	if (val <= 0)  return trim;			// in dead zone
	return (s16)((s32)val * (5000 - trim) / (calr - (calm + dead))) + trim;
    }
}

static void rev_epo_subtrim(u8 channel, s16 inval) {
    s16 val = (s16)(((s32)inval * cm.endpoint[channel][(u8)(inval < 0 ? 0 : 1)]
                     + 50) / 100);
    val += cm.subtrim[channel] * 10;
    if (cm.reverse & (u8)(1 << (channel - 1)))  val = -val;
    ppm_set_value(channel, (u16)(15000 + val));
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
			    cg.steering_dead_zone << ADC_OVS_SHIFT,
			    cm.trim[0] * 10);
	// XXX apply expos
	rev_epo_subtrim(1, (s16)(((s32)val * cm.dualrate[0] + 50) / 100));


	// throttle
	val = channel_calib(adc_throttle_ovs,
			    cg.calib_throttle_fwd << ADC_OVS_SHIFT,
			    cg.calib_throttle_mid << ADC_OVS_SHIFT,
			    cg.calib_throttle_bck << ADC_OVS_SHIFT,
			    cg.throttle_dead_zone << ADC_OVS_SHIFT,
			    cm.trim[1] * 10);
	// XXX apply expos
	rev_epo_subtrim(2, (s16)(((s32)val * cm.dualrate[1] + 50) / 100));


	// channel 3
	rev_epo_subtrim(3, btns(BTN_CH3) ? 5000 : -5000);


	// sync signal
	ppm_calc_sync();

	// wait for next cycle
	stop();
    }
}

