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
#include "gt3b.h"
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
#define G config_global
u8 config_global_set_default(void) {
    u8 cc = 0;	// calibrate changed

    G.magic_global	= CONFIG_GLOBAL_MAGIC;
    G.magic_model	= CONFIG_MODEL_MAGIC;
    G.backlight_time	= 30;
    G.model		= 0;
    // set calibrate values only when they are out of limits
    cc |= check_val(&G.calib_steering[0], 0, CALIB_ST_LOW_MID, 0);
    cc |= check_val(&G.calib_steering[1], CALIB_ST_LOW_MID, CALIB_ST_MID_HIGH, 512);
    cc |= check_val(&G.calib_steering[2], CALIB_ST_MID_HIGH, 1023, 1023);
    cc |= check_val(&G.calib_throttle[0], 0, CALIB_TH_LOW_MID, 0);
    cc |= check_val(&G.calib_throttle[1], CALIB_TH_LOW_MID, CALIB_TH_MID_HIGH, 600);
    cc |= check_val(&G.calib_throttle[2], CALIB_TH_MID_HIGH, 1023, 1023);
    return cc;
}




// set model configuration to default
#define M config_model
void config_model_set_default(void) {
    M.channels		= 3;
    M.name[0]		= 'M';
    M.name[1]		= (u8)(G.model / 10 + '0');
    M.name[2]		= (u8)(G.model % 10 + '0');
    M.reverse		= 0;
    memset(&M.trim, 0, MAX_CHANNELS);
    memset(&M.endpoint, 100, MAX_CHANNELS * 2);
    M.expo_steering	= 0;
    M.expo_throttle	= 0;
    M.expo_brake	= 0;
    M.dualrate[0]	= 100;
    M.dualrate[1]	= 100;
    M.abs_type		= 0;
}




// read model config from eeprom, if empty, set to defaults
void config_model_read(u8 model) {
    eeprom_read_model(model);
    if (config_model.name[0] != CONFIG_MODEL_EMPTY)  return;
    config_model_set_default();
    // XXX write to eeprom
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
    config_model_read(config_global.model);

    return calib_changed;
}

