/*
    menu_mix - handle menus for mix settings
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
#include "ppm.h"
#include "input.h"
#include "lcd.h"
#include "buzzer.h"





// variables to be used in CALC task
s8  menu_channel3_8[MAX_CHANNELS - 2];	// values -100..100 for channels >=3
u8  menu_channels_mixed;	// channel with 1 here will not be set from
				//   menu_channel3_8
s8  menu_4WS_mix;		// mix -100..100
_Bool menu_4WS_crab;		// when 1, crab steering
s8  menu_DIG_mix;		// mix -100..100
u8  menu_MP_index[MP_COUNT];	// index of MultiPosition channels
_Bool menu_brake;		// when 1, full brake is applied



// battery low flag
_Bool menu_battery_low;
// raw battery ADC value for check to battery low
u16 battery_low_raw;




// set menu_channels_mixed to ignore them from menu_channel3_8
void set_menu_channels_mixed(void) {
    menu_channels_mixed = 0;
    if (cm.channel_4WS)
	menu_channels_mixed |= (u8)(1 << (u8)(cm.channel_4WS - 1));
    if (cm.channel_DIG)
	menu_channels_mixed |= (u8)(1 << (u8)(cm.channel_DIG - 1));
    if (cm.channel_brake)
	menu_channels_mixed |= (u8)(1 << (u8)(cm.channel_brake - 1));
}


// apply model settings to variables
void apply_model_config(void) {
    u8 i, autorepeat = 0;

    // set number of channels for this model
    ppm_set_channels((u8)(cm.channels + 1));

    set_menu_channels_mixed();

    // set autorepeat
    for (i = 0; i < 4; i++) {
	if (!ck.et_map[i].is_trim)  continue;  // trim is off, skip
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

    // set values of channels >= 3 to default left state,
    //   for channels mapped to some trims/keys, it will next be set
    //   to corresponding centre/reset value
    for (i = 0; i < MAX_CHANNELS - 2; i++)
	menu_channel3_8[i] = -100;

    // set 4WS, DIG, MP to defaults
    menu_4WS_mix = 0;
    menu_4WS_crab = 0;
    menu_DIG_mix = 0;

    memset(menu_MP_index, 0, sizeof(menu_MP_index));
    if (cm.channel_MP0 && cm.channel_MP0 != MP_DIG)
	menu_channel3_8[cm.channel_MP0 - 3] = cm.multi_position0[0];
    if (cm.channel_MP1 && cm.channel_MP1 != MP_DIG)
	menu_channel3_8[cm.channel_MP1 - 3] = cm.multi_position1[0];
    if (cm.channel_MP2 && cm.channel_MP2 != MP_DIG)
	menu_channel3_8[cm.channel_MP2 - 3] = cm.multi_position2[0];
    if (cm.channel_MP3 && cm.channel_MP3 != MP_DIG)
	menu_channel3_8[cm.channel_MP3 - 3] = cm.multi_position3[0];

    menu_brake = 0;

    // set state of buttons to do initialize
    menu_buttons_initialize();

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


// menu stop - checks low battery
_Bool battery_low_shutup;
void menu_stop(void) {
    static _Bool battery_low_on;
    stop();
    // low_bat is disabled in calibrate, key-test and global menus,
    //   check it by buzzer_running
    if (menu_battery_low && !buzzer_running && !battery_low_shutup)
	battery_low_on = 0;
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


// clear all symbols
void menu_clear_symbols(void) {
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_segment(LS_SYM_CHANNEL, LS_OFF);
}


// common menu, select item at 7SEG and then set params at CHR3
u8 menu_set;		// menu is in: 0 = menu_id, 1..X = menu setting 1..X
u8 menu_id;		// id of selected menu
_Bool menu_id_set;	// 0 = in menu-id, 1 = in menu-setting
u8 menu_blink;		// what of chars should blink

static void func_init(u8 flags) {
    // blink all
    menu_blink = 0xff;

    // clear display symbols
    menu_clear_symbols();
    if (flags & MCF_LOWPWR)  lcd_segment(LS_SYM_LOWPWR, LS_OFF);
}

void menu_common(menu_common_t func, void *params, u8 flags) {
    menu_id_set = flags & MCF_SET_ONLY ? 1 : 0;
    menu_set = 0;		// now in menu_id
    menu_id = 0;		// first menu item

    // init and show setting
    func_init(flags);
    func(MCA_INIT, params);
    if (menu_id_set) {
	lcd_chars_blink_mask(LB_SPC, menu_blink);
    }
    else {
	if (menu_blink & MCB_7SEG)  lcd_set_blink(L7SEG, LB_SPC);
    }
    lcd_update();

    while (1) {

	// remove button flags and wait for wakeup
	btnra();
	if (flags & MCF_STOP)	stop();
	else			menu_stop();

	// end this menu with defined buttons
	if (btn(BTN_BACK | BTN_END) || btnl(BTN_ENTER))  break;
	if ((flags & MCF_ENTER) && btn(BTN_ENTER))	 break;

	// if menu ADC was activated, call func to read for example left-right pos
	if (menu_adc_wakeup)  func(MCA_ADC_PRE, params);

	// rotate encoder changed, change menu-id or value
	if (btn(BTN_ROT_ALL)) {
	    if (menu_id_set) {
		func_init(flags);
		func(MCA_SET_CHG, params);
		lcd_chars_blink_mask(LB_SPC, menu_blink);
	    }
	    else {
		// change menu-id

		// reset some variables
		menu_adc_wakeup = 0;
		menu_force_value_channel = 0;

		// select new menu id and show it
		func_init(flags);
		func(MCA_ID_CHG, params);	// do own change based on BTN_ROT

		if (menu_blink & MCB_7SEG)  lcd_set_blink(L7SEG, LB_SPC);
	    }
	    lcd_update();
	}

	// ENTER pressed, switch between menu settings
	else if (btn(BTN_ENTER)) {
	    // switch menu_id/menu-setting0/menu-setting1/...
	    key_beep();
	    if (flags & MCF_SWITCH) {
		// switch will be done by function
		func_init(flags);
		func(MCA_SWITCH, params);
		if (menu_set == 255)  break;	// exit menu when requested
		// blinking
		if (menu_id_set) {
		    lcd_chars_blink_mask(LB_SPC, menu_blink);
		    lcd_set_blink(L7SEG, LB_OFF);
		}
		else {
		    if (menu_blink & MCB_7SEG)  lcd_set_blink(L7SEG, LB_SPC);
		    lcd_chars_blink(LB_OFF);
		}
		lcd_update();
	    }
	    else if (menu_id_set) {
		// select next menu setting
		func_init(flags);
		func(MCA_SET_NEXT, params);
		if (menu_set == 255)  break;	// exit menu when requested
		if (menu_set || (flags & MCF_SET_ONLY)) {
		    // some > 0 menu setting
		    lcd_chars_blink_mask(LB_SPC, menu_blink);
		}
		else {
		    // rotated back to setting 0, switch to menu selection
		    menu_id_set = 0;
		    if (menu_blink & MCB_7SEG)  lcd_set_blink(L7SEG, LB_SPC);
		    lcd_chars_blink(LB_OFF);
		}
		lcd_update();
	    }
	    else {
		// switch to first menu setting
		menu_id_set = 1;
		// menu setting values is already showed
		lcd_set_blink(L7SEG, LB_OFF);
		lcd_chars_blink_mask(LB_SPC, menu_blink);
	    }
	}

	// if menu ADC was activated, call func to check if to show for example
	//   other value when left-right position changed
	if (menu_adc_wakeup) {
	    u8 mis = menu_id_set;	// save value
	    menu_id_set = 0;		// func will set it to 1 when show call needed
	    func(MCA_ADC_POST, params);
	    if (menu_id_set) {
		menu_id_set = mis;
		// show changed value
		func_init(flags);
		func(MCA_SHOW, params);
		if (menu_id_set) {
		    lcd_chars_blink_mask(LB_SPC, menu_blink);
		}
		else {
		    if (menu_blink & MCB_7SEG)  lcd_set_blink(L7SEG, LB_SPC);
		}
		lcd_update();
	    }
	    else menu_id_set = mis;
	}
    }

    // call to select next value which can do some action (such as reset)
    if (menu_id_set && menu_set != 255)  func(MCA_SET_NEXT, params);

    // cleanup display
    func_init(flags);

    // reset variables
    menu_adc_wakeup = 0;
    menu_force_value_channel = 0;
    key_beep();
}


// common list menu, given by list of functions, one for each menu item
typedef struct {
    menu_list_t *funcs;
    u8 nitems;
} menu_list_params_t;

static void menu_list_func(u8 action, menu_list_params_t *p) {
    menu_list_t func = p->funcs[menu_id];
    switch (action) {
	case MCA_SET_CHG:
	    func(MLA_CHG);
	    return;	// value already showed
	case MCA_SET_NEXT:
	    func(MLA_NEXT);
	    return;	// value already showed
	case MCA_ID_CHG:
	    menu_id = (u8)menu_change_val(menu_id, 0, p->nitems - 1, 1, 1);
	    func = p->funcs[menu_id];
	    break;
    }
    // show value
    func(MLA_SHOW);
}

void menu_list(menu_list_t *menu_funcs, u8 menu_nitems, u8 flags) {
    menu_list_params_t params;
    params.funcs = menu_funcs;
    params.nitems = menu_nitems;
    menu_common(menu_list_func, &params, (u8)(flags & (MCF_STOP | MCF_LOWPWR)));
}

