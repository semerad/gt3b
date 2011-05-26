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




#define ABS_THRESHOLD  PPM(50)





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
	return (s16)((s32)val * PPM(500) / ((calm - dead) - call));
    }
    else {
	// right part
	if (adc_ovs > calr) adc_ovs = calr;		// limit to calib right
	val = (s16)adc_ovs - (s16)(calm + dead);
	if (val <= 0)  return 0;			// in dead zone
	return (s16)((s32)val * PPM(500) / (calr - (calm + dead)));
    }
}

// apply reverse, endpoint, subtrim, trim (for channel 1-2)
// set value to ppm channel
static void channel_params(u8 channel, s16 inval) {
    s8 trim = 0;
    s16 val;
    s32 trim32 = 0;

    // if value forced from menu (settting endpoints, subtrims, ...), set it
    if (menu_force_value_channel == channel)
	inval = menu_force_value;
    
    // check limits -5000..5000
    if (inval < PPM(-500))       inval = PPM(-500);
    else if (inval > PPM(500))   inval = PPM(500);

    // read trims for channels 1-2 and compute inval offset
    if (channel < 3) {
	trim = cm.trim[channel-1];
	if (trim && inval)	// abs(inval) * (trim * 10) / 5000 -> 100x more
	    trim32 = ((s32)(inval < 0 ? -inval : inval) * trim + 2) / 5;
    }

    // apply endpoint and trim32
    val = (s16)(((s32)inval * cm.endpoint[channel-1][(u8)(inval < 0 ? 0 : 1)] -
		 trim32 + 50) / 100);

    // add subtrim, trim and reverse
    val += (cm.subtrim[channel-1] + trim) * PPM(1);
    if (cm.reverse & (u8)(1 << (channel - 1)))  val = -val;

    // set value for this ppm channel
    ppm_set_value(channel, (u16)(PPM(1500) + val));
}

// expo only for plus values: x: 0..5000, exp: 1..99
static s16 expou(u16 x, u8 exp) {
    // (x * x * x * exp / (5000 * 5000) + x * (100 - exp) + 50) / 100
    return (s16)(((u32)x * x / PPM(500) * x * exp / PPM(500)
                  + (u32)x * (u8)(100 - exp) + 50) / 100);
}
// apply expo: inval: -5000..5000, exp: -99..99
static s16 expo(s16 inval, s8 exp) {
    u8  neg;
    s16 val;

    if (exp == 0)    return inval;	// no expo
    if (inval == 0)  return inval;	// 0 don't change

    neg = (u8)(inval < 0 ? 1 : 0);
    if (neg)  inval = -inval;

    if (exp > 0)  val = expou(inval, exp);
    else          val = PPM(500) - expou(PPM(500) - inval, (u8)-exp);

    return  neg ? -val : val;
}

// apply dualrate
@inline static s16 dualrate(s16 val, u8 dr) {
    return (s16)(((s32)val * dr + 50) / 100);
}







// calculate new PPM values from ADC and internal variables
// called for each PPM cycle
static void calc_loop(void) {
    s16 val;
    u8  i;

    while (1) {

	// steering
	val = channel_calib(adc_steering_ovs,
			    cg.calib_steering_left << ADC_OVS_SHIFT,
			    cg.calib_steering_mid << ADC_OVS_SHIFT,
			    cg.calib_steering_right << ADC_OVS_SHIFT,
			    cg.steering_dead_zone << ADC_OVS_SHIFT);
	val = expo(val, cm.expo_steering);
	channel_params(1, dualrate(val, cm.dr_steering));




	// throttle
	val = channel_calib(adc_throttle_ovs,
			    cg.calib_throttle_fwd << ADC_OVS_SHIFT,
			    cg.calib_throttle_mid << ADC_OVS_SHIFT,
			    cg.calib_throttle_bck << ADC_OVS_SHIFT,
			    cg.throttle_dead_zone << ADC_OVS_SHIFT);
	val = expo(val, (u8)(val < 0 ? cm.expo_forward : cm.expo_back));
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
	channel_params(2, dualrate(val,
	               (u8)(val < 0 ? cm.dr_forward : cm.dr_back)));




	// channels 3-8, exclude mixed channels in the future
	for (i = 3; i <= MAX_CHANNELS; i++)
	    channel_params(i, menu_channel3_8[i - 3] * PPM(5));




	// sync signal
	ppm_calc_sync();

	// wait for next cycle
	stop();
    }
}

