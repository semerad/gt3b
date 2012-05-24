/*
    config include file
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


#ifndef _CONFIG_INCLUDED
#define _CONFIG_INCLUDED


#include "gt3b.h"




// global config

// change MAGIC number when changing global config
// also add code to setting default values at config.c
// length must by multiple of 4 because of EEPROM Word programming
// 52 bytes (16 reserved)
#define CONFIG_GLOBAL_MAGIC  0xf807
typedef struct {
    u8  steering_dead_zone;
    u8  throttle_dead_zone;
    u16 calib_steering_left;
    u16 calib_steering_mid;
    u16 calib_steering_right;
    u16 calib_throttle_fwd;
    u16 calib_throttle_mid;
    u16 calib_throttle_bck;
    u16 backlight_time;
    u16 magic_global;		// it is here because of problems when flashing
    u16 magic_model;		//  original fw back (1.byte was not overwrited)
    u16 calib_ch3_left;
    u16 calib_ch3_right;
    u16 battery_calib;		// raw ADC value for 10 Volts
    u8  model;			// selected model
    u8  battery_low;		// low battery threshold in .1 Volts
    u8  endpoint_max;		// max allowed endpoint value (def 120%)
    u8  long_press_delay;	// long press delay in 5ms steps
    u8	timer1_alarm;		// alarm of timer
    u8	timer2_alarm;

    u8  inactivity_alarm:4;	// time (min) of inactivity warning
    u8	key_beep:1;		// beep on key press
    u8	reset_beep:1;		// beep on center/reset value
    u8	poweron_beep:1;		// beep on power on
    u8	adc_ovs_last:1;		// use oversampled (0) or last (1) value in CALC
    u8  timer1_type:3;		// type of timer
    u8	timer2_type:3;
    u8	poweron_warn:1;		// beep 3 times when not-centered poweron
    u8	rotate_reverse:1;	// reverse rotate encoder sense
    u8	ppm_length:4;		// length of PPM sync signal (3..) or frame length (9..)
    u8	ppm_sync_frame:1;	// 0 = constant SYNC length, 1 = constant frame length
    u8	channels_default:3;	// default number of channels for model
    u8  ch3_pot:1;		// potentiometer connected instead of CH3 button
    u8	encoder_2detents:1;	// use 2 encoder detents to change value (weak GT3C encoder)

    u8  unused:6;		// reserve
    u8	reserve[16];
} config_global_s;

extern config_global_s config_global;
#define cg	config_global


// threshold limits for steering/throttle
#define CALIB_ST_LOW_MID   300
#define CALIB_ST_MID_HIGH  750
#define CALIB_TH_LOW_MID   350
#define CALIB_TH_MID_HIGH  800




// model config

typedef struct {	// unused parts are to match with config_key_map_s
    u8 function:7;
    u8 is_trim:1;
    u8 step:5;
    u8 previous_val:1;
    u8 reverse:1;
    u8 opposite_reset:1;
    u8 buttons:3;
    u8 rotate:1;
    u8 unused2:3;
    u8 is_trim2:1;
    u8 unused3;
} config_et_map_s;
#define ETB_LONG_OFF	0
#define ETB_AUTORPT	1
#define ETB_MOMENTARY	2
#define ETB_LONG_RESET	3
#define ETB_LONG_ENDVAL	4
#define ETB_SPECIAL	5

typedef struct {
    u8 function:5;
    u8 previous_val:1;
    u8 reverse:1;
    u8 is_trim:1;
    u8 function_long:5;
    u8 previous_val_long:1;
    u8 reverse_long:1;
    u8 momentary:1;
} config_key_map_s;

#define NUM_TRIMS  4
#define NUM_KEYS   3
// 22 bytes
typedef struct {
    config_key_map_s	key_map[NUM_KEYS];  // will expand to following et_map
    config_et_map_s	et_map[NUM_TRIMS];
} config_key_mapping_s;

#define MP_COUNT	     4
#define NUM_MULTI_POSITION0  8
#define NUM_MULTI_POSITION1  6
#define NUM_MULTI_POSITION2  4
#define NUM_MULTI_POSITION3  4
#define MULTI_POSITION_END   -128
#define MP_DIG               0x0f




// change MAGIC number when changing model config
// also add code to setting default values
// length must by multiple of 4 because of EEPROM/FLASH Word programming
// 54(30 reserved) + 22(keys) + channels * 4 bytes = 108 for 8-channel fw
#define CONFIG_MODEL_MAGIC  (0xd820 | (MAX_CHANNELS - 1))
typedef struct {
    u8	name[3];
    u8	reverse;			// bit for each channel
    s8	subtrim[MAX_CHANNELS];
    u8	endpoint[MAX_CHANNELS][2];
    u8	speed[MAX_CHANNELS];		// reduce speed of servo
#define stspd_turn	speed[0]
#define thspd		speed[1]
    u8	stspd_return;			// steering speed return
    s8	trim[2];			// for steering and throttle
#define trim_steering	trim[0]
#define trim_throttle	trim[1]
    u8	dualrate[3];			// for steering and throttle
#define dr_steering	dualrate[0]
#define dr_forward	dualrate[1]
#define dr_back		dualrate[2]
    s8	expo[3];			// steering/forward/back
#define expo_steering	expo[0]
#define expo_forward	expo[1]
#define expo_back	expo[2]
    s8	multi_position0[NUM_MULTI_POSITION0];  // values for MultiPosition 0
    s8	multi_position1[NUM_MULTI_POSITION1];  // values for MultiPosition 1
    s8	multi_position2[NUM_MULTI_POSITION2];  // values for MultiPosition 2
    s8	multi_position3[NUM_MULTI_POSITION3];  // values for MultiPosition 3

    u8	channels:3;			// number of channels for this model - 1
    u8	brake_off:1;			// don't use brake side of throttle
    u8	channel_4WS:4;			// channel for 4WS mix or 0 when off
    u8	channel_DIG:4;			// channel for DIG mix or 0 when off
    u8	channel_MP0:4;			// channel for MultiPosition0 or 0 when off
    u8	channel_MP1:4;			// channel for MultiPosition1 or 0 when off
    u8	channel_MP2:4;			// channel for MultiPosition2 or 0 when off
    u8	channel_MP3:4;			// channel for MultiPosition3 or 0 when off
    u8	thspd_onlyfwd:1;		// throttle speed only at forward side
    u8	abs_type:2;

    u8	unused:1;
    u8	channel_brake:4;		// channel for brake side of throttle

    u8  unused2:4;
    u8	unused3;

    config_key_mapping_s key_mapping;

    u8	reserve[13];
} config_model_s;

extern @near config_model_s config_model;
#define cm		config_model
#define ck		cm.key_mapping
#define ck_ch3_pot_func	((u8 *)&ck.key_map[0])
#define ck_ch3_pot_rev	((u8 *)&ck.key_map[0] + 1)

// get multi_position channel and array of values
// return number of multi_position items
extern u8 config_get_MP(u8 index, u8 *pchannel_MP, s8 **pmulti_position);






#include "eeprom.h"


// number of model memories (eeprom + flash)

#define CONFIG_MODEL_MAX_EEPROM	((EEPROM_SIZE - sizeof(config_global_s)) / \
				 sizeof(config_model_s))
// last eeprom model config will be used to temporarily store
//   flash model config
#define CONFIG_MODEL_TMP	(CONFIG_MODEL_MAX_EEPROM - 1)

// DefaultInterrupt is last used place in FLASH, skip 1 byte ret instruction
//   and compute how many models will fit to flash
extern @interrupt void DefaultInterrupt (void);
#define CONFIG_MODEL_MAX_FLASH	((u8)(((u16)0xffff - (u16)DefaultInterrupt) / \
				 sizeof(config_model_s)))

#define CONFIG_MODEL_MAX	(CONFIG_MODEL_MAX_EEPROM - 1 + CONFIG_MODEL_MAX_FLASH)


// when name[0] is 0x00, that model memory was empty
#define CONFIG_MODEL_EMPTY  0x00






// set default values
extern u8 config_global_set_default(void);
extern void config_model_set_default(void);


// read values from eeprom and set defaults when needed
extern void config_model_read(void);
extern u8 config_global_read(void);
extern u8 *config_model_name(u8 model);

// write values to eeprom
extern void config_set_model(u8 model);
extern void config_model_save(void);
#define config_global_save()  eeprom_write_global()
#define config_empty_models() eeprom_empty_models()


#endif

