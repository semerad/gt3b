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
    cg.backlight_time	= 30;
    cg.battery_low	= 616;
    // set calibrate values only when they are out of limits
    cc |= check_val(&cg.calib_steering_left, 0, CALIB_ST_LOW_MID, 0);
    cc |= check_val(&cg.calib_steering_mid, CALIB_ST_LOW_MID, CALIB_ST_MID_HIGH, 512);
    cc |= check_val(&cg.calib_steering_right, CALIB_ST_MID_HIGH, 1023, 1023);
    cc |= check_val(&cg.calib_throttle_fwd, 0, CALIB_TH_LOW_MID, 0);
    cc |= check_val(&cg.calib_throttle_mid, CALIB_TH_LOW_MID, CALIB_TH_MID_HIGH, 600);
    cc |= check_val(&cg.calib_throttle_bck, CALIB_TH_MID_HIGH, 1023, 1023);
    return cc;
}




// set default name to given pointer
static void default_model_name(u8 model, u8 *name) {
    *name++ = 'M';
    *name++ = (u8)(model / 10 + '0');
    *name   = (u8)(model % 10 + '0');
}


// set model configuration to default
void config_model_set_default(void) {
    cm.channels		= 3;
    default_model_name(cg.model, cm.name);
    cm.reverse		= 0;
    memset(&cm.trim, 0, MAX_CHANNELS);
    memset(&cm.endpoint, 100, MAX_CHANNELS * 2);
    cm.expo_steering	= 0;
    cm.expo_forward	= 0;
    cm.expo_back	= 0;
    cm.dualrate[0]	= 100;
    cm.dualrate[1]	= 100;
    cm.abs_type		= 0;
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
    u8 *addr = EEPROM_CONFIG_MODEL + 1 + sizeof(config_model_s) * model;
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
	calib_changed = config_global_set_default();
	eeprom_write_global();
	eeprom_empty_models();
    }
    else if (config_global.magic_model != CONFIG_MODEL_MAGIC) {
	// model config changed, empty it
	eeprom_empty_models();
    }

    return calib_changed;
}

