/*
    menu - handle showing to display, menus
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



#include <string.h>
#include "menu.h"
#include "config.h"
#include "calc.h"
#include "timer.h"
#include "ppm.h"
#include "lcd.h"
#include "buzzer.h"
#include "input.h"






// variables to be used in CALC task
u8  menu_force_value_channel;	// set PPM value for this channel
s16 menu_force_value;		//   to this value (-500..500)
s8  menu_channel3_8[MAX_CHANNELS - 2];	// values -100..100 for channels >=3
u8  menu_channels_mixed;	// channel with 1 here will not be set from
				//   menu_channel3_8
s8  menu_4WS_mix;		// mix -100..100
_Bool menu_4WS_crab;		// when 1, crab steering
s8  menu_DIG_mix;		// mix -100..100





// flags for wakeup after each ADC measure
_Bool menu_wants_adc;
// battery low flag
_Bool menu_battery_low;
// raw battery ADC value for check to battery low
u16 battery_low_raw;








// ****************** UTILITY FUNCTIONS ******************************


// apply model settings to variables
static _Bool awake_calc_allowed;
void apply_model_config(void) {
    u8 i, autorepeat = 0;

    // set number of channels for this model
    if (channels != MAX_CHANNELS) {
	ppm_set_channels(MAX_CHANNELS);  // maybe sometime cm.channels
	// task CALC must be awaked to do first PPM calculation
	if (awake_calc_allowed)  awake(CALC);
    }

    // set mixed channels to ignore them from menu_channel3_8
    menu_channels_mixed = 0;
    if (cm.channel_4WS)
	menu_channels_mixed |= (u8)(1 << (u8)(cm.channel_4WS - 1));
    if (cm.channel_DIG)
	menu_channels_mixed |= (u8)(1 << (u8)(cm.channel_DIG - 1));

    // set autorepeat
    for (i = 0; i < 4; i++) {
	if (ck.et_off & (1 << i))  continue;  // trim is off, skip
	if (ck.et_map[i].buttons == ETB_AUTORPT)
	    autorepeat |= (u8)((u8)et_buttons[i][0] | (u8)et_buttons[i][1]);
    }
    button_autorepeat(autorepeat);
}


// load model config from eeprom and set model settings
void menu_load_model(void) {
    u8 i;
    // load config
    config_model_read();

    // set values of channels >= 3 to default left state
    for (i = 0; i <= MAX_CHANNELS - 3; i++)
	menu_channel3_8[i] = -100;

    // set 4WS and DIG to defaults
    menu_4WS_mix = 100;
    menu_4WS_crab = 0;
    menu_DIG_mix = 0;

    // set other values: mixers, ...

    // apply config to radio setting
    apply_model_config();
}


// apply global setting to variables
void apply_global_config(void) {
    backlight_set_default(cg.backlight_time);
    backlight_on();
    // compute raw value for battery low voltage
    battery_low_raw = (u16)(((u32)cg.battery_calib * cg.battery_low + 50) / 100);
}


// show model number, extra function to handle more than 10 models
static void show_model_number(u8 model) {
    lcd_7seg((u8)(model % 10));
    lcd_segment(LS_SYM_RIGHT, (u8)((u8)(model / 10) & 1));
    lcd_segment(LS_SYM_LEFT, (u8)((u8)(model / 20) & 1));
}


// menu stop - checks low battery
static void menu_stop(void) {
    static _Bool battery_low_on;
    stop();
    if (battery_low_on == menu_battery_low)  return;  // no change

    // battery low status changed
    if (menu_battery_low) {
	// battery low firstly
	battery_low_on = 1;
	lcd_segment(LS_SYM_LOWPWR, LS_ON);
	lcd_segment_blink(LS_SYM_LOWPWR, LB_SPC);
	buzzer_on(40, 160, BUZZER_MAX);
    }
    else {
	// battery low now OK
	battery_low_on = 0;
	lcd_segment(LS_SYM_LOWPWR, LS_OFF);
	buzzer_off();
    }
    lcd_update();
}





// show main screen (model number and name/battery/...)
#define MS_NAME		0
#define MS_BATTERY	1
#define MS_MAX		2
static void main_screen(u8 item) {
    lcd_segment(LS_SYM_MODELNO, LS_ON);
    lcd_segment(LS_SYM_CHANNEL, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
    show_model_number(cg.model);

    menu_wants_adc = 0;

    // chars is item dependent
    if (item == MS_NAME) {
	// model name
	lcd_segment(LS_SYM_DOT, LS_OFF);
	lcd_segment(LS_SYM_VOLTS, LS_OFF);
	lcd_chars(cm.name);
    }
    else if (item == MS_BATTERY) {
	static u16 bat_val = 0;
	static u16 bat_time = 0;

	// battery voltage
	lcd_segment(LS_SYM_DOT, LS_ON);
	lcd_segment(LS_SYM_VOLTS, LS_ON);
	// calculate voltage from current raw value and calib value
	if (time_sec >= bat_time) {
	    bat_time = time_sec + 2;
	    bat_val = (u16)(((u32)adc_battery * 100 + 300) / cg.battery_calib);
	}
	lcd_char_num3(bat_val);
	menu_wants_adc = 1;
    }
    lcd_update();
}




// common menu processing, selecting channel and then changing values
static _Bool menu_adc_direction;
static void menu_set_adc_direction(u8 channel) {
    u16 adc, calm;

    // for channel 2 use throttle, for others steering
    if (channel == 2) {
	adc = adc_throttle_ovs;
	calm = cg.calib_throttle_mid;
    }
    else {
	adc = adc_steering_ovs;
	calm = cg.calib_steering_mid;
    }

    // if over threshold to one side, set menu_adc_direction
    if (adc < ((calm - 40) << ADC_OVS_SHIFT)) {
	if (menu_adc_direction) {
	    menu_adc_direction = 0;
	    lcd_segment(LS_SYM_LEFT, LS_ON);
	    lcd_segment(LS_SYM_RIGHT, LS_OFF);
	}
    }
    else if (adc > ((calm + 40) << ADC_OVS_SHIFT)) {
	if (!menu_adc_direction) {
	    menu_adc_direction = 1;
	    lcd_segment(LS_SYM_LEFT, LS_OFF);
	    lcd_segment(LS_SYM_RIGHT, LS_ON);
	}
    }
    else if (channel > 2 && btn(BTN_CH3)) {
	// use CH3 button to toggle also
	menu_adc_direction ^= 1;
	lcd_segment(LS_SYM_LEFT, (u8)(menu_adc_direction ? LS_OFF : LS_ON));
	lcd_segment(LS_SYM_RIGHT, (u8)(menu_adc_direction ? LS_ON : LS_OFF));
    }

    // if this channel is using forced values, set it to left/right
    if (menu_force_value_channel)
	menu_force_value = menu_adc_direction ? PPM(500) : PPM(-500);
}
static _Bool menu_set_adc(u8 channel, u8 use_adc, u8 force_values) {
    // check if force servos to positions (left/center/right)
    if ((u8)(force_values & (1 << (channel - 1)))) {
	menu_force_value_channel = channel;
	menu_force_value = 0;	// to center by default
    }
    else menu_force_value_channel = 0;

    // check use of ADC
    if ((u8)(use_adc & (1 << (channel - 1)))) {
	// use ADC
	if (menu_adc_direction) {
	    lcd_segment(LS_SYM_LEFT, LS_OFF);
	    lcd_segment(LS_SYM_RIGHT, LS_ON);
	}
	else {
	    lcd_segment(LS_SYM_LEFT, LS_ON);
	    lcd_segment(LS_SYM_RIGHT, LS_OFF);
	}
	menu_wants_adc = 1;
	menu_set_adc_direction(channel);
	return 1;
    }
    else {
	// don't use ADC
	lcd_segment(LS_SYM_LEFT,    LS_OFF);
	lcd_segment(LS_SYM_RIGHT,   LS_OFF);
	menu_wants_adc = 0;
	return 0;
    }
}
static void menu_channel(u8 end_channel, u8 use_adc, u8 forced_values,
			 void (*subfunc)(u8, u8)) {
    u8 channel = 1;
    _Bool chan_val = 0;			// now in channel
    _Bool adc_active;
    u8 last_direction;

    // show CHANNEL
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_CHANNEL, LS_ON);

    // show channel number and possible direction
    menu_adc_direction = 0;
    lcd_7seg(channel);
    lcd_set_blink(L7SEG, LB_SPC);
    adc_active = menu_set_adc(channel, use_adc, forced_values);
    subfunc((u8)(channel - 1), 0);	// show current value
    lcd_update();

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END) || btnl(BTN_ENTER))  break;

	last_direction = menu_adc_direction;
	if (adc_active)  menu_set_adc_direction(channel);

	if (btn(BTN_ROT_ALL)) {
	    if (chan_val) {
		// change value
		subfunc((u8)(channel - 1), 1);
		lcd_chars_blink(LB_SPC);
	    }
	    else {
		// change channel number
		if (btn(BTN_ROT_L)) {
		    if (!--channel)  channel = end_channel;
		}
		else {
		    if (++channel > end_channel)  channel = 1;
		}
		lcd_7seg(channel);
		lcd_set_blink(L7SEG, LB_SPC);
		adc_active = menu_set_adc(channel, use_adc, forced_values);
		subfunc((u8)(channel - 1), 0);
	    }
	    lcd_update();
	    last_direction = menu_adc_direction;  // was already showed
	}

	else if (btn(BTN_ENTER)) {
	    // switch channel/value
	    key_beep();
	    if (chan_val) {
		// switch to channel number
		lcd_set_blink(L7SEG, LB_SPC);
		lcd_chars_blink(LB_OFF);
		chan_val = 0;
	    }
	    else {
		// switch to value
		lcd_set_blink(L7SEG, LB_OFF);
		lcd_chars_blink(LB_SPC);
		chan_val = 1;
	    }
	}

	if (last_direction != menu_adc_direction) {
	    // show other dir value
	    subfunc((u8)(channel - 1), 0);
	    lcd_update();
	    if (chan_val) {
		lcd_chars_blink(LB_SPC);
	    }
	}
    }

    menu_wants_adc = 0;
    menu_force_value_channel = 0;
    key_beep();
    config_model_save();
}


// change value based on state of rotate encoder
s16 menu_change_val(s16 val, s16 min, s16 max, u8 amount_fast, u8 rotate) {
    u8 amount = 1;

    if (btn(BTN_ROT_L)) {
	// left
	if (btnl(BTN_ROT_L))  amount = amount_fast;
	val -= amount;
	if (val < min)
	    if (rotate)	 val = max;
	    else         val = min;
    }
    else {
	// right
	if (btnl(BTN_ROT_R))  amount = amount_fast;
	val += amount;
	if (val > max)
	    if (rotate)  val = min;
	    else         val = max;
    }
    return val;
}







// *************************** MODEL MENUS *******************************

// select model/save model as (to selected model position)
#define MIN(a, b)  (a < b ? a : b)
static void menu_model(u8 saveas) {
    u8 model = cg.model;

    if (saveas)  lcd_set_blink(LMENU, LB_SPC);
    lcd_set_blink(L7SEG, LB_SPC);

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END | BTN_ENTER))  break;
	if (btn(BTN_ROT_ALL)) {
	    model = (u8)menu_change_val((s16)model, 0,
					MIN(CONFIG_MODEL_MAX, 40) - 1,
					MODEL_FAST, 1);
	    show_model_number(model);
	    lcd_set_blink(L7SEG, LB_SPC);
	    lcd_chars(config_model_name(model));
	    lcd_update();
	}
    }

    key_beep();
    // if new model choosed, save it
    if (model != cg.model) {
	cg.model = model;
	config_global_save();
	if (saveas) {
	    // save to new model position
	    config_model_save();
	}
	else {
	    // load selected model
	    menu_load_model();
	}
    }
    if (saveas)  lcd_set_blink(LMENU, LB_OFF);
}


// change model name
static void menu_name(void) {
    u8 pos = LCHR1;
    u8 letter = cm.name[0];

    lcd_set_blink(LCHR1, LB_SPC);

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END) || btnl(BTN_ENTER))  break;
	if (btn(BTN_ENTER)) {
	    key_beep();
	    // to next char
	    lcd_set_blink(pos, LB_OFF);
	    if (++pos > LCHR3)  pos = LCHR1;
	    lcd_set_blink(pos, LB_SPC);
	    lcd_update();
	    letter = cm.name[pos];
	}
	else if (btn(BTN_ROT_ALL)) {
	    // change letter
	    if (btn(BTN_ROT_L)) {
		// lower
		if (letter == '0')      letter = 'Z';
		else if (letter == 'A')	letter = '9';
		else                    letter--;
	    }
	    else {
		// upper
		if (letter == '9')      letter = 'A';
		else if (letter == 'Z')	letter = '0';
		else                    letter++;
	    }
	    cm.name[pos] = letter;
	    lcd_char(pos, letter);
	    lcd_set_blink(pos, LB_SPC);
	    lcd_update();
	}
    }

    key_beep();
    config_model_save();
}


// reset model to default values
static void menu_reset_model(void) {
    config_model_set_default();
    config_model_save();
    apply_model_config();
}


// set reverse
void sf_reverse(u8 channel, u8 change) {
    u8 bit = (u8)(1 << channel);
    if (change)  cm.reverse ^= bit;
    if (cm.reverse & bit)  lcd_chars("REV");
    else                   lcd_chars("NOR");
}
@inline static void menu_reverse(void) {
    menu_channel(MAX_CHANNELS, 0, 0, sf_reverse);
}


// set endpoints
void sf_endpoint(u8 channel, u8 change) {
    u8 *addr = &cm.endpoint[channel][menu_adc_direction];
    if (change)  *addr = (u8)menu_change_val(*addr, 0, cg.endpoint_max,
                                             ENDPOINT_FAST, 0);
    lcd_char_num3(*addr);
}
static void menu_endpoint(void) {
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    menu_channel(MAX_CHANNELS, 0xff, 0xfc, sf_endpoint);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
}


// set trims
static void sf_trim(u8 channel, u8 change) {
    s8 *addr = &cm.trim[channel];
    if (change)  *addr = (s8)menu_change_val(*addr, -TRIM_MAX, TRIM_MAX,
                                             TRIM_FAST, 0);
    if (channel == 0)  lcd_char_num2_lbl(*addr, "LNR");
    else               lcd_char_num2_lbl(*addr, "FNB");
}
@inline static void menu_trim(void) {
    menu_channel(2, 0, 0, sf_trim);
}


// set subtrims
static void sf_subtrim(u8 channel, u8 change) {
    s8 *addr = &cm.subtrim[channel];
    if (change)
	*addr = (s8)menu_change_val(*addr, -SUBTRIM_MAX, SUBTRIM_MAX,
	                            SUBTRIM_FAST, 0);
    lcd_char_num3(*addr);
}
static void menu_subtrim(void) {
    lcd_set_blink(LMENU, LB_SPC);
    menu_channel(MAX_CHANNELS, 0, 0xfc, sf_subtrim);
    lcd_set_blink(LMENU, LB_OFF);
}


// set dualrate
static void sf_dualrate(u8 channel, u8 change) {
    u8 *addr = &cm.dualrate[channel];
    if (channel == 1 && menu_adc_direction)  addr = &cm.dualrate[2];
    if (change)  *addr = (u8)menu_change_val(*addr, 0, 100, DUALRATE_FAST, 0);
    lcd_char_num3(*addr);
}
static void menu_dualrate(void) {
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    menu_channel(2, 0x2, 0, sf_dualrate);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
}


// set servo speed
static void sf_speed(u8 channel, u8 change) {
    u8 *addr = menu_adc_direction ? &cm.stspd_return : &cm.stspd_turn;
    if (change)  *addr = (u8)menu_change_val(*addr, 1, 100, SPEED_FAST, 0);
    lcd_char_num3(*addr);
}
static void menu_speed(void) {
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    menu_channel(1, 0x1, 0, sf_speed);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
}


// set expos
static void sf_expo(u8 channel, u8 change) {
    s8 *addr = &cm.expo[channel];
    if (channel == 1 && menu_adc_direction)  addr = &cm.expo[2];
    if (change)  *addr = (s8)menu_change_val(*addr, -EXPO_MAX, EXPO_MAX,
                                             EXPO_FAST, 0);
    lcd_char_num3(*addr);
}
static void menu_expo(void) {
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    menu_channel(2, 0x2, 0, sf_expo);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
}


// set abs: OFF, SLO(6), NOR(4), FAS(3)
// pulses between full brake and 1/2 brake and only when enought brake applied
static const u8 *abs_labels[] = {
    "OFF", "SLO", "NOR", "FAS"
};
#define ABS_LABEL_SIZE  (sizeof(abs_labels) / sizeof(u8 *))
static void menu_abs(void) {
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_segment(LS_SYM_CHANNEL, LS_ON);

    lcd_7seg(2);
    lcd_chars(abs_labels[cm.abs_type]);
    lcd_chars_blink(LB_SPC);
    lcd_update();

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END | BTN_ENTER))  break;

	if (btn(BTN_ROT_ALL)) {
	    cm.abs_type = (u8)menu_change_val(cm.abs_type, 0, ABS_LABEL_SIZE-1,
	                                      1, 1);
	    lcd_chars(abs_labels[cm.abs_type]);
	    lcd_chars_blink(LB_SPC);
	    lcd_update();
	}
    }

    key_beep();
    config_model_save();
}







// set mapping of keys
// key_id is: 4 trims, 3 buttons, 8 individual buttons of trims
// individual buttons of trims are available only when corresponding trim
// is off

#define TRIM_FUNCTIONS_SIZE  128
@near static u8 trim_functions[TRIM_FUNCTIONS_SIZE];
@near static u8 trim_functions_max;
static const u8 *trim_buttons[] = {
    "NOL", "RPT", "MOM", "RES", "END", "SPC"
};
#define TRIM_BUTTONS_SIZE  (sizeof(trim_buttons) / sizeof(u8 *))
// 7seg:  1 2 3 d
// chars: function
//          OFF					  (NOR/REV)  (NOO/RES)
//          other -> buttons
//                     MOM	       -> 	  reverse
// 		       NOL/RPT/RES/END -> step -> reverse -> opp_reset
// id:                 %                  % V     V          V blink
static u8 km_trim(u8 trim_id, u8 val_id, u8 action) {
    config_et_map_s *etm = &ck.et_map[trim_id];
    u8 trim_bit = (u8)(1 << trim_id);
    u16 opposite_reset = 1 << (u8)(trim_id * 2 + NUM_KEYS);
    u8 show = 0;
    u8 id = val_id;
    u8 idx;

    if (!action)  show = 1;
    else if (action == 1) {
	// change value
	switch (id) {
	    case 1:
		if (ck.et_off & trim_bit) {
		    // change from OFF, set to defaults
		    ck.et_off ^= trim_bit;
		    etm->step = 1;
		    etm->reverse = 0;
		    etm->buttons = 0;
		    etm->function = 0;
		    ck.momentary &= ~opposite_reset;
		    ck.momentary &= ~(opposite_reset << 1);  // not-used bit
		}
		// select new function, map through trim_functions
		idx = menu_et_function_idx(etm->function);
		while (1) {
		    if (btn(BTN_ROT_L)) {
			if (idx)  idx--;
			else      idx = trim_functions_max;
		    }
		    else {
			if (++idx > trim_functions_max)  idx = 0;
		    }
		    if (trim_functions[idx])  break;  // we have it
		}
		etm->function = (u8)(trim_functions[idx] - 1);
		if (!etm->function) {
		    // change to OFF, set to zeroes for setting as keys
		    etm->step = 0;
		    etm->reverse = 0;
		    etm->buttons = 0;
		    ck.et_off |= trim_bit;
		    ck.momentary &= ~opposite_reset;
		    ck.momentary &= ~(opposite_reset << 1);  // not-used bit
		}
		break;
	    case 2:
		// show special ("SPC") only when selected function has it
		if (menu_et_function_long_special(etm->function))
			idx = 1;
		else	idx = 2;
		etm->buttons = (u8)menu_change_val(etm->buttons, 0,
						 TRIM_BUTTONS_SIZE - idx, 1, 1);
		break;
	    case 3:
		etm->step = (u8)menu_change_val(etm->step, 0,
						STEPS_MAP_SIZE - 1, 1, 0);
		break;
	    case 4:
		etm->reverse ^= 1;
		break;
	    case 5:
		ck.momentary ^= opposite_reset;
		break;
	}
	show = 1;
    }
    else {
	// switch to next setting
	if (!(id == 1 && (ck.et_off & trim_bit))) {
	    if (etm->buttons == ETB_MOMENTARY) {
		if (++id > 4)  id = 1;
		if (id == 3)   id = 4;  // skip "step" for momentary
	    }
	    else {
		if (++id > 5)  id = 1;
	    }
	    show = 1;
	}
    }

    if (show) {
	// show value of val_id
	switch (id) {
	    case 1:
		if (ck.et_off & trim_bit)  lcd_chars("OFF");
		else  lcd_chars(menu_et_function_name(etm->function));
		lcd_segment(LS_SYM_PERCENT, LS_OFF);
		lcd_segment(LS_SYM_VOLTS, LS_OFF);
		break;
	    case 2:
		lcd_chars(trim_buttons[etm->buttons]);
		lcd_segment(LS_SYM_PERCENT, LS_ON);
		lcd_segment(LS_SYM_VOLTS, LS_OFF);
		break;
	    case 3:
		lcd_char_num3(steps_map[etm->step]);
		lcd_segment(LS_SYM_PERCENT, LS_ON);
		lcd_segment(LS_SYM_VOLTS, LS_ON);
		break;
	    case 4:
		lcd_chars(etm->reverse ? "REV" : "NOR");
		lcd_segment(LS_SYM_PERCENT, LS_OFF);
		lcd_segment(LS_SYM_VOLTS, LS_ON);
		break;
	    case 5:
		lcd_chars(ck.momentary & opposite_reset ? "RES" : "NOO");
		lcd_segment(LS_SYM_PERCENT, LS_OFF);
		lcd_segment(LS_SYM_VOLTS, LS_ON);
		lcd_segment_blink(LS_SYM_VOLTS, LB_SPC);
		break;
	}
    }
    return id;
}

#define KEY_FUNCTIONS_SIZE  32
@near static u8 key_functions[KEY_FUNCTIONS_SIZE];
@near static u8 key_functions_max;
// 7seg:  C b E 1l 1r 2l 2r 3l 3r dl dr
// chars: function
//          OFF    -> function_long (% V)
//          2STATE -> momentary (%)
//                      SWI -> function_long (% V)
//                      MOM -> reverse(NOR/REV) (V)
//          other  -> function_long (% V)
static u8 km_key(u8 key_id, u8 val_id, u8 action) {
    config_key_map_s *km = &ck.key_map[key_id];
    u16 key_bit = 1 << key_id;
    u8 id = val_id;
    u8 idx;

    if (action == 1) {
	// change value
	if (id == 1) {
	    // function
	    // select new function, map through key_functions
	    idx = menu_key_function_idx(km->function);
	    while (1) {
		if (btn(BTN_ROT_L)) {
		    if (idx)  idx--;
		    else      idx = key_functions_max;
		}
		else {
		    if (++idx > key_functions_max)  idx = 0;
		}
		if (key_functions[idx])  break;  // we have it
	    }
	    km->function = (u8)(key_functions[idx] - 1);
	}
	else if (id == 2 && menu_key_function_2state(km->function)) {
	    // momentary setting
	    ck.momentary ^= key_bit;
	    km->function_long = 0;  // set to default after switching
	}
	else if (id == 2 || id == 3 && !(ck.momentary & key_bit)) {
	    // function long
	    // select new function, map through key_functions
	    idx = menu_key_function_idx(km->function_long);
	    while (1) {
		if (btn(BTN_ROT_L)) {
		    if (idx)  idx--;
		    else      idx = key_functions_max;
		}
		else {
		    if (++idx > key_functions_max)  idx = 0;
		}
		if (key_functions[idx])  break;  // we have it
	    }
	    km->function_long = (u8)(key_functions[idx] - 1);
	}
	else {
	    // reverse
	    km->function_long = (u8)(km->function_long ? 0 : 1);
	}
    }
    else if (action == 2) {
	// switch to next setting
	if (id == 3)       id = 1;	// 3 is max
	else if (id == 1)  id = 2;	// there is always next setting
	else {
	    if (menu_key_function_2state(km->function))  id = 3;
	    else  id = 1;
	}
    }

    // show value of val_id
    if (id == 1) {
	// function
	lcd_chars(menu_key_function_name(km->function));
	lcd_segment(LS_SYM_PERCENT, LS_OFF);
	lcd_segment(LS_SYM_VOLTS, LS_OFF);
    }
    else if (id == 2 && menu_key_function_2state(km->function)) {
	// momentary setting
	lcd_chars(ck.momentary & key_bit ? "MOM" : "SWI");
	lcd_segment(LS_SYM_PERCENT, LS_ON);
	lcd_segment(LS_SYM_VOLTS, LS_OFF);
    }
    else if (id == 2 || id == 3 && !(ck.momentary & key_bit)) {
	// function long
	lcd_chars(menu_key_function_name(km->function_long));
	lcd_segment(LS_SYM_PERCENT, LS_ON);
	lcd_segment(LS_SYM_VOLTS, LS_ON);
    }
    else {
	// reverse
	lcd_chars(km->function_long ? "REV" : "NOR");
	lcd_segment(LS_SYM_PERCENT, LS_OFF);
	lcd_segment(LS_SYM_VOLTS, LS_ON);
    }
    return id;
}

static u8 km_trim_key(u8 key_id, u8 val_id, u8 action) {
    if (key_id < NUM_TRIMS)  return km_trim(key_id, val_id, action);
    else  return km_key((u8)(key_id - NUM_TRIMS), val_id, action);
}
static void menu_key_mapping(void) {
    u8 key_id = 0;			// trims, keys, trim-keys
    u8 id_val = 0;			// now in key_id
    u8 trim_id;
    static const u8 key_ids[] = {
	1, 2, 3, L7_D, L7_C, L7_B, L7_E
    };

    lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_7seg(key_ids[0]);
    lcd_set_blink(L7SEG, LB_SPC);
    km_trim_key(0, 1, 0);		// show first setting for first trim
    lcd_update();

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END) || btnl(BTN_ENTER))  break;

	if (btn(BTN_ROT_ALL)) {
	    if (id_val) {
		// change selected key setting
		km_trim_key(key_id, id_val, 1);
		lcd_chars_blink(LB_SPC);
		lcd_update();
	    }
	    else {
		// change key-id
		while (1) {
		    if (btn(BTN_ROT_L)) {
			if (key_id)	key_id--;
			else		key_id = 3 * NUM_TRIMS + NUM_KEYS - 1;
		    }
		    else {
			if (++key_id >= 3 * NUM_TRIMS + NUM_KEYS)  key_id = 0;
		    }
		    if (key_id < NUM_TRIMS + NUM_KEYS) {
			// trims and 3keys (CH3/BACK/END) always
			lcd_7seg(key_ids[key_id]);
			lcd_segment(LS_SYM_LEFT, LS_OFF);
			lcd_segment(LS_SYM_RIGHT, LS_OFF);
			break;
		    }
		    // check trim keys and use them only when corresponding
		    //   trim is off
		    trim_id = (u8)((u8)(key_id - NUM_TRIMS - NUM_KEYS) >> 1);
		    if (!(ck.et_off & (u8)(1 <<trim_id)))  continue;

		    lcd_7seg(key_ids[trim_id]);
		    if ((u8)(key_id - NUM_TRIMS - NUM_KEYS) & 1) {
			// right trim key
			lcd_segment(LS_SYM_RIGHT, LS_ON);
			lcd_segment(LS_SYM_LEFT, LS_OFF);
		    }
		    else {
			// left trim key
			lcd_segment(LS_SYM_RIGHT, LS_OFF);
			lcd_segment(LS_SYM_LEFT, LS_ON);
		    }
		    break;
		}
		km_trim_key(key_id, 1, 0);	// show first key setting
		lcd_set_blink(L7SEG, LB_SPC);
		lcd_update();
	    }
	}

	else if (btn(BTN_ENTER)) {
	    // switch key-id/key-setting1/key-setting2/...
	    if (id_val) {
		// what to do depends on what was selected in this item
		id_val = km_trim_key(key_id, id_val, 2);
		if (id_val != 1) {
		    lcd_chars_blink(LB_SPC);
		}
		else {
		    // switch to key selection
		    id_val = 0;
		    lcd_set_blink(L7SEG, LB_SPC);
		    lcd_chars_blink(LB_OFF);
		}
		lcd_update();
	    }
	    else {
		// switch to key settings
		id_val = 1;
		// key setting values is already showed
		lcd_set_blink(L7SEG, LB_OFF);
		lcd_chars_blink(LB_SPC);
	    }
	}
    }

    key_beep();
    lcd_set_blink(LMENU, LB_OFF);
    config_model_save();
    apply_model_config();
}

// prepare sorted functions for mapping keys
static void menu_key_mapping_prepare(void) {
    u8 i;
    s8 n;

    // electronic trims
    for (i = 0; i < TRIM_FUNCTIONS_SIZE; i++) {
	n = menu_et_function_idx(i);
	if (n < 0)  break;  // last
	if (n > trim_functions_max)  trim_functions_max = n;
	trim_functions[n] = (u8)(i + 1);  // + 1 to be able to find non-empty
    }
    // keys
    for (i = 0; i < KEY_FUNCTIONS_SIZE; i++) {
	n = menu_key_function_idx(i);
	if (n < 0)  break;  // last
	if (n > key_functions_max)  key_functions_max = n;
	key_functions[n] = (u8)(i + 1);   // + 1 to be able to find non-empty
    }
}







// choose from menu items
static void select_menu(void) {
    u8 menu = LM_MODEL;
    lcd_menu(menu);
    main_screen(MS_NAME);	// show model number and name

    while (1) {
	btnra();
	menu_stop();

	// Back/End key to end this menu
	if (btn(BTN_BACK | BTN_END))  break;

	// Enter key - goto submenu
	if (btn(BTN_ENTER)) {
	    key_beep();
	    if (menu == LM_MODEL)	menu_model((u8)(btnl(BTN_ENTER) ? 1 : 0));
	    else if (menu == LM_NAME) {
		if (btnl(BTN_ENTER))	menu_reset_model();
		else			menu_name();
	    }
	    else if (menu == LM_REV) {
		if (btnl(BTN_ENTER))	menu_key_mapping();
		else			menu_reverse();
	    }
	    else if (menu == LM_EPO)	menu_endpoint();
	    else if (menu == LM_TRIM) {
		if (btnl(BTN_ENTER))	menu_subtrim();
		else			menu_trim();
	    }
	    else if (menu == LM_DR) {
		if (btnl(BTN_ENTER))	menu_speed();
		else			menu_dualrate();
	    }
	    else if (menu == LM_EXP)	menu_expo();
	    else {
		if (btnl(BTN_ENTER))	break;
		else			menu_abs();
	    }
	    main_screen(MS_NAME);	// show model number and name
	    // exit when BACK
	    if (btn(BTN_BACK))  break;
	}

	// rotate keys
	else if (btn(BTN_ROT_ALL)) {
	    if (btn(BTN_ROT_R)) {
		menu >>= 1;
		if (!menu)  menu = LM_MODEL;
	    }
	    else {
		menu <<= 1;
		if (!menu)  menu = LM_ABS;
	    }
	    lcd_menu(menu);
	    lcd_update();
	}
    }

    key_beep();
    lcd_menu(0);
}








// ****************** MAIN LOOP and init *********************************

// main menu loop, shows main screen and handle keys
static void menu_loop(void) {
    u8 item = MS_NAME;

    lcd_clear();

    while (1) {
	main_screen(item);
	btnra();
	menu_stop();

	// don't wanted in submenus, will be set back in main_screen()
	menu_wants_adc = 0;

    check_keys:
	// Enter long key - global/calibrate/key-test
	if (btnl(BTN_ENTER)) {
	    if (adc_steering_ovs > (CALIB_ST_MID_HIGH << ADC_OVS_SHIFT))
		menu_calibrate();
	    else if (adc_steering_ovs < (CALIB_ST_LOW_MID << ADC_OVS_SHIFT))
		menu_key_test();
	    else menu_global_setup();
	}

	// Enter key - menu
	else if (btn(BTN_ENTER)) {
	    key_beep();
	    select_menu();
	}

	// electronic trims
	else if (menu_electronic_trims())
	    goto check_keys;

	// buttons (CH3, Back, End)
	else if (menu_buttons())
	    goto check_keys;

	// rotate encoder - change model name/battery/...
	else if (btn(BTN_ROT_ALL)) {
	    if (btn(BTN_ROT_L)) {
		if (item)  item--;
		else       item = MS_MAX - 1;
	    }
	    else {
		if (++item >= MS_MAX)  item = 0;
	    }
	}
    }
}



// read config from eeprom, initialize and call menu loop
void menu_init(void) {
    // variables
    menu_key_mapping_prepare();

    // read global config from eeprom, if calibrate values changed,
    //   call calibrate
    if (config_global_read())
	menu_calibrate();
    apply_global_config();

    // read model config from eeprom, but now awake CALC yet
    menu_load_model();
    awake_calc_allowed = 1;

    // and main loop
    menu_loop();
}

