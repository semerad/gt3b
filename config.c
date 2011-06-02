/*
    config - global and model config
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
#include "config.h"
#include "eeprom.h"
#include "input.h"


// actual configuration
config_global_s config_global;
config_model_s config_model;




static u8 check_val(u16 *pv, u16 lo, u16 hi, u16 def) {
    if (*pv >= lo && *pv <= hi)  return 0;
    *pv = def;
    return 1;
}

// set global configuration to defaults
u8 config_global_set_default(void) {
    u8 cc = 0;	// calibrate changed

    cg.magic_global	= CONFIG_GLOBAL_MAGIC;
    cg.magic_model	= CONFIG_MODEL_MAGIC;
    cg.model		= 0;
    cg.steering_dead_zone = 2;
    cg.throttle_dead_zone = 2;
    cg.backlight_time	= 30;
    cg.battery_calib	= 672;
    cg.battery_low	= 92;		// 9.2 V
    cg.endpoint_max	= 120;
    cg.inactivity_alarm	= 0;
    cg.key_beep		= 1;

    // set calibrate values only when they are out of limits
    cc |= check_val(&cg.calib_steering_left, 0, CALIB_ST_LOW_MID, 0);
    cc |= check_val(&cg.calib_steering_mid, CALIB_ST_LOW_MID, CALIB_ST_MID_HIGH, 512);
    cc |= check_val(&cg.calib_steering_right, CALIB_ST_MID_HIGH, 1023, 1023);
    cc |= check_val(&cg.calib_throttle_fwd, 0, CALIB_TH_LOW_MID, 0);
    cc |= check_val(&cg.calib_throttle_mid, CALIB_TH_LOW_MID, CALIB_TH_MID_HIGH, 600);
    cc |= check_val(&cg.calib_throttle_bck, CALIB_TH_MID_HIGH, 1023, 1023);
    return cc;
}




// default model key mapping
static const config_key_mapping_s default_key_mapping = {
    // keys: function, function_long
    {
	{ 1, 0 },		// CH3 to channel 3
	{ 0, 0 },
	{ 0, 0 }
    },
    // trims: function, reverse, step, buttons, opposite_reset
    {
	{ 1, 0, 0, 0, 0 },	// trim1 to steering trim
	{ 2, 0, 0, 0, 0 },	// trim2 to throttle trim
	{ 1, 0, 0, 0, 0 },	// trim3 to steering trim
	{ 3, 0, 0, 1, 0 }	// trim4(DR) to steering dualrate, autorepeat
    },
    // momentary
    0,
    // et_off
    0
};

// set default name to given pointer
static void default_model_name(u8 model, u8 *name) {
    *name++ = 'M';
    *name++ = (u8)(model / 10 + '0');
    *name   = (u8)(model % 10 + '0');
}


// set model configuration to default
void config_model_set_default(void) {
    default_model_name(cg.model, cm.name);
    cm.reverse		= 0;
    memset(cm.subtrim, 0, MAX_CHANNELS);
    memset(cm.endpoint, 100, MAX_CHANNELS * 2);
    cm.trim[0]		= 0;
    cm.trim[1]		= 0;
    cm.dr_steering	= 100;
    cm.dr_forward	= 100;
    cm.dr_back		= 100;
    cm.expo_steering	= 0;
    cm.expo_forward	= 0;
    cm.expo_back	= 0;
    cm.abs_type		= 0;
    memcpy(&cm.key_mapping, &default_key_mapping, sizeof(config_key_mapping_s));
}




// read model config from eeprom, if empty, set to defaults
void config_model_read(void) {
    eeprom_read_model(config_global.model);
    if (config_model.name[0] != CONFIG_MODEL_EMPTY)  return;
    config_model_set_default();
}


// return model name for given model number
u8 *config_model_name(u8 model) {
    @near static u8 fake_name[3];
    u8 *addr = EEPROM_CONFIG_MODEL + sizeof(config_model_s) * model;
    if (*addr == CONFIG_MODEL_EMPTY) {
	default_model_name(model, fake_name);
	addr = fake_name;
    }
    return addr;
}



// read global config from eeprom, if MAGICs changed, set to defaults
// returns 1 if calibration values was set to defaults
u8 config_global_read(void) {
    u8 calib_changed = 0;

    eeprom_read_global();
    if (config_global.magic_global != CONFIG_GLOBAL_MAGIC) {
	// global config changed, initialize whole eeprom
	eeprom_empty_models();
	calib_changed = config_global_set_default();
	// do not write magic_global yet to eliminate interrupted initialization
	//   (for example flash-verify after flash-write in STVP)
	config_global.magic_global = 0;
	config_global_save();
	// and now as last set magic_global
	config_global.magic_global = CONFIG_GLOBAL_MAGIC;
	config_global_save();
    }
    else if (config_global.magic_model != CONFIG_MODEL_MAGIC) {
	// model config changed, empty all models
	eeprom_empty_models();
	// set model number to 0
	config_global.model = 0;
	config_global_save();
	// set new model magic
	config_global.magic_model = CONFIG_MODEL_MAGIC;
	config_global_save();
    }

    return calib_changed;
}

