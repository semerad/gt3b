/*
    menu include file
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


#ifndef _MENU_INCLUDED
#define _MENU_INCLUDED


#include "gt3b.h"



// trims/subtrims limits
#define TRIM_MAX	99
#define SUBTRIM_MAX	99
#define EXPO_MAX	100

// amount of step when fast encoder rotate
#define MODEL_FAST	2
#define ENDPOINT_FAST	5
#define TRIM_FAST	5
#define SUBTRIM_FAST	5
#define DUALRATE_FAST	5
#define EXPO_FAST	5
#define CHANNEL_FAST	5
#define MIX_FAST	5
#define SPEED_FAST	5

// delay in seconds of popup menu (trim, dualrate, ...)
#define POPUP_DELAY	5

// pause in 5ms when reset value is reached during trim popup
#define RESET_VALUE_DELAY	(500 / 5)




// variables to be used in CALC task
extern u8  menu_force_value_channel;	// set PPM value for this channel
extern s16 menu_force_value;		//   to this value (-500..500)
extern s8  menu_channel3_8[MAX_CHANNELS - 2];	// values -100..100 for channels >=3
#define menu_channel3  menu_channel3_8[0]
#define menu_channel4  menu_channel3_8[1]
#define menu_channel5  menu_channel3_8[2]
#define menu_channel6  menu_channel3_8[3]
#define menu_channel7  menu_channel3_8[4]
#define menu_channel8  menu_channel3_8[5]
extern u8  menu_channels_mixed;	    // channel with 1 here will not be set from
					//   menu_channel3_8
extern s8  menu_4WS_mix;		// mix -100..100
extern _Bool menu_4WS_crab;		// when 1, crab steering
extern s8  menu_DIG_mix;		// mix -100..100

extern @near u8 menu_buttons_state[];	// last state of button
#define MBS_INITIALIZE	0xff
// for momentary keys
#define MBS_RELEASED	0x01
#define MBS_PRESSED	0x02
#define MBS_MIDDLE	0x03
// for momentary trims
#define	MBS_LEFT	0x04
#define	MBS_RIGHT	0x05
// for switched keys
#define MBS_ON		0x40
#define MBS_ON_LONG	0x80






// task MENU will be alias for task OPER
#define MENU OPER


// menu tasks will be handling ADC now (for calibrate)
extern _Bool menu_wants_adc;
// flag low battery voltage
extern _Bool menu_battery_low;
// raw battery ADC value for check to battery low
extern u16 battery_low_raw;



// internal functions, used in split menu files
extern void menu_stop(void);
extern void menu_calibrate(void);
extern void menu_key_test(void);
extern void menu_global_setup(void);
extern s16  menu_change_val(s16 val, s16 min, s16 max, u8 amount_fast, u8 rotate);
extern void apply_global_config(void);
extern void menu_load_model(void);
extern void apply_model_config(void);
extern u8 menu_electronic_trims(void);
extern u8 menu_buttons(void);
extern u8 *menu_et_function_name(u8 n);
extern s8 menu_et_function_idx(u8 n);
extern u8 menu_et_function_long_special(u8 n);
extern u8 *menu_key_function_name(u8 n);
extern s8 menu_key_function_idx(u8 n);
extern u8 menu_key_function_2state(u8 n);
extern const u8 steps_map[];
#define STEPS_MAP_SIZE  11
extern const u16 et_buttons[][2];
extern void menu_mix(void);
extern void menu_key_mapping(void);
extern void menu_key_mapping_prepare(void);
extern void menu_buttons_initialize(void);


#endif

