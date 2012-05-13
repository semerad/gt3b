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
#include "timer.h"




#define ABS_THRESHOLD  PPM(50)

@near static s16 last_value[MAX_CHANNELS];





// CALC task
TASK(CALC, 90);
static void calc_loop(void);


// initialize CALC task
void calc_init(void) {
    build(CALC);
    activate(CALC, calc_loop);
    sleep(CALC);	// no work yet, waked up after setting number of channels
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

    // save last value
    last_value[channel - 1] = inval;

    // read trims for channels 1-2 and compute inval offset
    if (channel == 1 && cm.channel_DIG != 1)
	// steering and not dual-ESC steering
	trim = cm.trim_steering;
    else if (channel == 2 || channel == cm.channel_DIG)
	// throttle and also second throttle from channel_DIG
	trim = cm.trim_throttle;
    if (trim && inval)	// abs(inval) * (trim * 10) / 5000 -> 100x more
	trim32 = ((s32)(inval < 0 ? -inval : inval) * trim + 2) / 5;

    // apply endpoint and trim32
    val = (s16)(((s32)inval * cm.endpoint[channel-1][(u8)(inval < 0 ? 0 : 1)] -
		 trim32) / 100);

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
    return (s16)((s32)val * dr / 100);
}


// apply steering speed
static s16 steering_speed(s16 val, u8 channel) {
    s16 last = last_value[channel - 1];
    s16 delta = val - last;
    s16 delta2 = 0;
    s16 max_delta;
    u8 stspd;

    if (!delta)  return val;	// no change from previous val
    if (cm.stspd_turn == 100 && cm.stspd_return == 100)  return val; // max spd

    if (!last)  stspd = cm.stspd_turn;	// it is always turn from centre
    else if (last < 0) {
	// last was left
	if (val < last)  stspd = cm.stspd_turn;	// more left turn
	else {
	    // right from previous
	    if (val <= 0)  stspd = cm.stspd_return;  // return max to centre
	    else {
		// right to right side of centre
		stspd = cm.stspd_return;
		delta = -last;
		delta2 = val;
	    }
	}
    }
    else {
	// last was right
	if (val > last)  stspd = cm.stspd_turn;	// more right turn
	else {
	    // left from previous
	    if (val >= 0)  stspd = cm.stspd_return;  // return max to centre
	    else {
		// left to left side of centre
		stspd = cm.stspd_return;
		delta = -last;
		delta2 = val;
	    }
	}
    }

    // calculate max delta
    if (stspd == 100)  max_delta = PPM(1000);
    else  max_delta = PPM(1000) / 2 / 20 * ppm_frame_length / (100 - stspd);

    // compare delta with max_delta
    if (delta < 0) {
	if (max_delta < -delta) {
	    // over
	    val = last - max_delta;
	    delta2 = 0;			// nothing at turn side
	}
    }
    else {
	if (max_delta < delta) {
	    // over
	    val = last + max_delta;
	    delta2 = 0;			// nothing at turn side
	}
    }

    // check if it is moving from return to turn
    if (delta2) {
	if (cm.stspd_turn == 100)  max_delta = PPM(1000);
	else  max_delta = PPM(1000) / 2 / 20 * ppm_frame_length / (100 - cm.stspd_turn);

	if (delta2 < 0) {
	    if (max_delta < -delta2)  val = -max_delta;
	}
	else {
	    if (max_delta < delta2)   val = max_delta;
	}
    }

    return val;
}


// apply servo speed
static s16 channel_speed(s16 val, u8 channel) {
    s16 last = last_value[channel - 1];
    s16 delta = val - last;
    s16 max_delta;
    u8  speed = cm.speed[channel - 1];

    // for DIG channel use throttle speed
    if (channel == cm.channel_DIG) speed = cm.thspd;

    if (!delta)  return val;		// no change from previous val
    if (speed == 100)  return val;	// max speed

    // special handling for "throttle only for forward"
    if (cm.thspd_onlyfwd && (channel == 2 || channel == cm.channel_DIG)) {
	if (val >= 0 && !cm.brake_off)  return val;	// at brake side
	if (delta > 0)  return val;			// not forwarding
	if (last > 0 && !cm.brake_off) {
	    // apply speed only from centre to forward
	    last = 0;
	    delta = val;
	}
    }

    max_delta = PPM(1000) / 2 / 20 * ppm_frame_length / (100 - speed);
    if (delta < 0) {
	if (max_delta < -delta)  val = last - max_delta;
    }
    else {
	if (max_delta < delta)   val = last + max_delta;
    }
    return val;
}







// calculate new PPM values from ADC and internal variables
// called for each PPM cycle
static void calc_loop(void) {
    s16 val, val2;
    u8  i, bit;
    s16 DIG_mix;
    u16 adc_steering, adc_throttle;  // last 4 or 1 ADC values

    while (1) {

	// handle channel3 potentiometer, cannot use channel_calib,
	//   because we don't have calib middle and dead zone
	if (cg.ch3_pot && !menu_ch3_pot_disabled) {
	    // do it only if some function is assigned to CH3 pot
	    if (*ck_ch3_pot_func) {
		if (cg.adc_ovs_last)  val = adc_ch3_last << ADC_OVS_SHIFT;
		else		  val = adc_ch3_ovs;
		// map ch3 pot -5000..5000 range
		val -= cg.calib_ch3_left << ADC_OVS_SHIFT;
		val2 = (s16)((s32)(val) * PPM(1000) /
		       ((cg.calib_ch3_right - cg.calib_ch3_left) << ADC_OVS_SHIFT))
		       - PPM(500);
		// safety checks to -5000..5000 limits
		if (val2 < PPM(-500))	   val2 = PPM(-500);
		else if (val2 > PPM(500))  val2 = PPM(500);
		// do reverse
		if (*ck_ch3_pot_rev)  val2 = -val2;
		// set value to function
		menu_et_function_set_from_linear(*ck_ch3_pot_func, val2);
	    }
	}

	DIG_mix = menu_DIG_mix * PPM(5);  // to -5000..5000 range

	// set used ADC values
	if (cg.adc_ovs_last) {
	    // last value
	    adc_steering = adc_steering_last << ADC_OVS_SHIFT;
	    adc_throttle = adc_throttle_last << ADC_OVS_SHIFT;
	}
	else {
	    // oversampled last 4 values
	    adc_steering = adc_steering_ovs;
	    adc_throttle = adc_throttle_ovs;
	}

	// steering
	val = channel_calib(adc_steering,
			    cg.calib_steering_left << ADC_OVS_SHIFT,
			    cg.calib_steering_mid << ADC_OVS_SHIFT,
			    cg.calib_steering_right << ADC_OVS_SHIFT,
			    cg.steering_dead_zone << ADC_OVS_SHIFT);
	val = expo(val, cm.expo_steering);
	val = dualrate(val, cm.dr_steering);
	if (cm.channel_DIG != 1) {
	    // channel 1 is normal servo steering
	    if (!cm.channel_4WS)
		channel_params(1, steering_speed(val, 1));
	    else {
		// 4WS mixing
		val2 = val;
		if (menu_4WS_crab)  val2 = -val2;	// apply crab

		if (menu_4WS_mix < 0)
		    // reduce front steering
		    val = (s16)((s32)val * (100 + menu_4WS_mix) / 100);
		else if (menu_4WS_mix > 0)
		    // reduce rear steering
		    val2 = (s16)((s32)val2 * (100 - menu_4WS_mix) / 100);

		channel_params(1, steering_speed(val, 1));
		channel_params(cm.channel_4WS,
			       steering_speed(val2, cm.channel_4WS));
	    }
	}
	else {
	    // channel 1 is part of dual-ESC steering
	    @near static s16 last_ch1;

	    // apply steering trim to val
	    if (cm.trim_steering) {
		val2 = (s16)(((s32)(val < 0 ? -val : val) *
				    cm.trim_steering + 250) / PPM(500 / 10));
		val = val - val2 + cm.trim_steering * PPM(1);
	    }
	    // return back value from steering wheel to allow to use
	    //   steering speed
	    val2 = last_value[0];
	    last_value[0] = last_ch1;
	    val = steering_speed(val, 1);
	    if (val < PPM(-500))      val = PPM(-500);
	    else if (val > PPM(500))  val = PPM(500);
	    // save steering value for steering speed
	    last_ch1 = val;
	    last_value[0] = val2;
	    // set DIG mix
	    DIG_mix = -val;  // minus, because 100 will reduce channel 1
	    menu_DIG_mix = (s8)(DIG_mix / PPM(5));
	    // set it 2 times more to have contra ESC steering, it can be
	    //   reduced by D/R setting
	    if (!cm.brake_off)  DIG_mix *= 2;
	}




	// throttle
	if (menu_brake)
	    val = PPM(500);	// brake button overrides throttle
	else
	    val = channel_calib(adc_throttle,
				cg.calib_throttle_fwd << ADC_OVS_SHIFT,
				cg.calib_throttle_mid << ADC_OVS_SHIFT,
				cg.calib_throttle_bck << ADC_OVS_SHIFT,
				cg.throttle_dead_zone << ADC_OVS_SHIFT);
	val = expo(val, (u8)(val < 0 ? cm.expo_forward : cm.expo_back));
	if (cm.abs_type) {
	    // apply selected ABS
	    static u8    abs_time;
	    static _Bool abs_state;	// when 1, lower brake value
	    // cycle time in 1ms steps: 120/80/60ms
	    u8 cycle_time = (u8)(cm.abs_type == 1 ? 120 :
				 cm.abs_type == 2 ? 80 : 60);

	    if (val > ABS_THRESHOLD) {
		// check time with 40ms reserve and change abs_state
		if ((u8)(ppm_timer - abs_time) <= 40) {
		    abs_time = (u8)(ppm_timer + cycle_time);
		    abs_state ^= 1;
		}
		// apply ABS
		if (abs_state)
		    val /= 2;
	    }
	    else {
		// no ABS
		abs_time = (u8)(ppm_timer + cycle_time);
		abs_state = 0;
	    }
	}
	val = dualrate(val, (u8)(val < 0 ? cm.dr_forward : cm.dr_back));
	// brake to extra channel
	if (cm.channel_brake) {
	    val2 = val;
	    if (val2 < 0)  val2 = 0;		// eliminate forward
	    val2 = val2 * 2 - PPM(500);		// to whole servo range
	    channel_params(cm.channel_brake, channel_speed(val2, cm.channel_brake));
	}
	// throttle brake cut off
	if (cm.brake_off && val > 0)  val = 0;
	// set to servo
	if (!cm.channel_DIG) {
	    if (cm.brake_off)  val = val * 2 + PPM(500);
	    channel_params(2, channel_speed(val, 2));
	}
	else {
	    // DIG mixing
	    val2 = val;

	    if (menu_DIG_mix < 0)
		// reduce front throttle
		val = (s16)((s32)val * (PPM(500) + DIG_mix) / PPM(500));
	    else if (menu_DIG_mix > 0)
		// reduce rear throttle
		val2 = (s16)((s32)val2 * (PPM(500) - DIG_mix) / PPM(500));

	    if (cm.brake_off) {
		val  = val  * 2 + PPM(500);
		val2 = val2 * 2 + PPM(500);
	    }
	    channel_params(2, channel_speed(val, 2));
	    channel_params(cm.channel_DIG, channel_speed(val2, cm.channel_DIG));
	}




	// channels 3-8, exclude mixed channels in the future
	for (i = 3, bit = 0b100; i <= channels; i++, bit <<= 1) {
	    // check if channel was already mixed before (4WS, DIG)
	    if (menu_channels_mixed & bit)  continue;
	    channel_params(i, channel_speed(menu_channel3_8[i - 3] * PPM(5), i));
	}




	// sync signal
	ppm_calc_sync();

	// wait for next cycle
	stop();
    }
}

