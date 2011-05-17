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




#define ABS_THRESHOLD  500





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

// apply reverse, endpoint, subtrim
static void rev_epo_subtrim(u8 channel, s16 inval) {
    s16 val = (s16)(((s32)inval * cm.endpoint[channel-1][(u8)(inval < 0 ? 0 : 1)]
                     + 50) / 100);
    val += cm.subtrim[channel-1] * 10;
    if (cm.reverse & (u8)(1 << (channel - 1)))  val = -val;
    ppm_set_value(channel, (u16)(15000 + val));
}

// expo only for plus values
static s16 expou(u16 x, u8 exp) {
    // (x * x * x * exp / (5000 * 5000) + x * (100 - exp) + 50) / 100
    return (s16)(((u32)x * x / 5000 * x * exp / 5000
                  + x * (100 - exp) + 50) / 100);
}
// apply expo
static s16 expo(s16 inval, s8 exp) {
    u8  neg;
    s16 val;

    if (exp == 0)  return inval;	// no expo

    neg = (u8)(inval < 0 ? 1 : 0);
    if (neg)  inval = -inval;

    if (exp > 0)  val = expou(inval, exp);
    else          val = 5000 - expou(5000 - inval, (u8)-exp);

    return  neg ? -val : val;
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
	val = expo(val, cm.expo_steering);
	rev_epo_subtrim(1, (s16)(((s32)val * cm.dualrate[0] + 50) / 100));




	// throttle
	val = channel_calib(adc_throttle_ovs,
			    cg.calib_throttle_fwd << ADC_OVS_SHIFT,
			    cg.calib_throttle_mid << ADC_OVS_SHIFT,
			    cg.calib_throttle_bck << ADC_OVS_SHIFT,
			    cg.throttle_dead_zone << ADC_OVS_SHIFT,
			    cm.trim[1] * 10);
	if (val < 0)  val = expo(val, cm.expo_forward);
	else          val = expo(val, cm.expo_back);
	if (cm.abs_type) {
	    // apply selected ABS
	    static u8    abs_cnt;
	    static _Bool abs_state;	// when 1, lower brake value

	    if (val > ABS_THRESHOLD) {
		// count ABS
		abs_cnt++;
		if (cm.abs_type == 1 && abs_cnt >= 6
			|| cm.abs_type == 2 && abs_cnt >= 4
			|| cm.abs_type == 3 && abs_cnt >=3) {
		    abs_cnt = 0;
		    abs_state ^= 1;
		}
		// apply ABS
		if (abs_state)
		    val /= 2;
	    }
	    else {
		// no ABS
		abs_cnt = 0;
		abs_state = 0;
	    }
	}
	rev_epo_subtrim(2, (s16)(((s32)val * cm.dualrate[1] + 50) / 100));




	// channel 3
	if (cg.ch3_momentary)  val = btns(BTN_CH3) ? 5000 : -5000;
	else                   val = ch3_state ? 5000 : -5000;
	rev_epo_subtrim(3, val);




	// sync signal
	ppm_calc_sync();

	// wait for next cycle
	stop();
    }
}

