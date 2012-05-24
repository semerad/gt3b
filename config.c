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
@near config_model_s config_model;




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
    cg.channels_default	= 2;		// 3 channels as model channels default

    cg.steering_dead_zone = 2;
    cg.throttle_dead_zone = 2;
    cg.adc_ovs_last	= 0;		// use oversampled value in CALC

    cg.backlight_time	= 30;
    cg.battery_calib	= 672;
    cg.battery_low	= 92;		// 9.2 V
    cg.endpoint_max	= 120;
    cg.long_press_delay	= 1000 / 5;	// 1sec
    cg.inactivity_alarm	= 0;
    cg.key_beep		= 1;
    cg.reset_beep	= 1;
    cg.poweron_beep	= 1;
    cg.poweron_warn	= 0;

    cg.timer1_type	= 0;		// OFF
    cg.timer2_type	= 0;
    cg.timer1_alarm	= 0;		// OFF
    cg.timer2_alarm	= 0;

    cg.ppm_sync_frame	= 0;		// to constant SYNC length
    cg.ppm_length	= 1;		// 4ms constant SYNC length
    cg.rotate_reverse	= 0;		// not-reversed
    cg.ch3_pot		= 0;		// CH3 is button
    cg.encoder_2detents	= 0;		// use 2 detents to change value

    // set calibrate values only when they are out of limits
    cc |= check_val(&cg.calib_steering_left, 0, CALIB_ST_LOW_MID, 0);
    cc |= check_val(&cg.calib_steering_mid, CALIB_ST_LOW_MID, CALIB_ST_MID_HIGH, 512);
    cc |= check_val(&cg.calib_steering_right, CALIB_ST_MID_HIGH, 1023, 1023);
    cc |= check_val(&cg.calib_throttle_fwd, 0, CALIB_TH_LOW_MID, 0);
    cc |= check_val(&cg.calib_throttle_mid, CALIB_TH_LOW_MID, CALIB_TH_MID_HIGH, 600);
    cc |= check_val(&cg.calib_throttle_bck, CALIB_TH_MID_HIGH, 1023, 1023);
    check_val(&cg.calib_ch3_left, 0, 512, 0);
    check_val(&cg.calib_ch3_right, 512, 1023, 1023);

    cg.unused		= 0;
    memset(cg.reserve, 0, sizeof(cg.reserve));

    return cc;
}




// default model key mapping
static const config_key_mapping_s default_key_mapping = {
    // keys: function, function_long
    {
	{ 0, 0, 0, 0, 0, 0, 0, 0 },	// CH3 to nothing
	{ 0, 0, 0, 0, 0, 0, 0, 0 },	// BACK to nothing
	{ 0, 0, 0, 0, 1, 0, 0, 0 }	// END-long to battery low shutup
    },
    // trims: function, reverse, step, buttons
    {
	{ 1, 1, 0, 0, 0, 0, ETB_LONG_OFF, 0, 0, 1, 0 },	// trim1 to steer trim
	{ 2, 1, 0, 0, 0, 0, ETB_LONG_OFF, 0, 0, 1, 0 },	// trim2 to throt trim
	{ 1, 1, 0, 0, 0, 0, ETB_LONG_OFF, 0, 0, 1, 0 },	// trim3 to steer trim
	{ 3, 1, 0, 0, 0, 0, ETB_AUTORPT, 0, 0, 1, 0 }	// trim4(DR) to steer dualrate, autorepeat
    },
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
    cm.channels		= cg.channels_default;

    cm.reverse		= 0;
    memset(cm.subtrim, 0, sizeof(cm.subtrim));
    memset(cm.endpoint, 100, sizeof(cm.endpoint));
    cm.trim_steering	= 0;
    cm.trim_throttle	= 0;

    cm.dr_steering	= 100;
    cm.dr_forward	= 100;
    cm.dr_back		= 100;

    cm.expo_steering	= 0;
    cm.expo_forward	= 0;
    cm.expo_back	= 0;

    memset(cm.speed, 100, sizeof(cm.speed));
    cm.stspd_return	= 100;

    cm.abs_type		= 0;
    cm.brake_off	= 0;
    cm.thspd_onlyfwd	= 0;

    cm.channel_4WS	= 0;
    cm.channel_DIG	= 0;
    cm.channel_brake	= 0;

    cm.channel_MP0	= 0;
    memset(cm.multi_position0, (u8)MULTI_POSITION_END, sizeof(cm.multi_position0));
    cm.multi_position0[0]= -100;
    cm.channel_MP1	= 0;
    memset(cm.multi_position1, (u8)MULTI_POSITION_END, sizeof(cm.multi_position1));
    cm.multi_position1[0]= -100;
    cm.channel_MP2	= 0;
    memset(cm.multi_position2, (u8)MULTI_POSITION_END, sizeof(cm.multi_position2));
    cm.multi_position2[0]= -100;
    cm.channel_MP3	= 0;
    memset(cm.multi_position3, (u8)MULTI_POSITION_END, sizeof(cm.multi_position3));
    cm.multi_position3[0]= -100;

    memcpy(&cm.key_mapping, &default_key_mapping, sizeof(config_key_mapping_s));
    if (cg.ch3_pot) {
	*ck_ch3_pot_func = 0;
	*ck_ch3_pot_rev  = 0;
    }

    cm.unused		= 0;
    cm.unused2		= 0;
    cm.unused3		= 0;
    memset(cm.reserve, 0, sizeof(cm.reserve));
}






// read model config from eeprom/flash, if empty, set to defaults
// first read after poweron should read FLASH model from
//   eeprom CONFIG_MODEL_TMP, because FLASH memory is updated
//   only after model change (so when powered off, current memory
//   is only at tmp eeprom and not in FLASH)
void config_model_read(void) {
    static _Bool not_first;
    u8 model = cg.model;

    if (!not_first && cg.model > CONFIG_MODEL_TMP)
	model = CONFIG_MODEL_TMP;

    if (model < CONFIG_MODEL_TMP || !not_first) {
	// it is in eeprom
	not_first = 1;
	eeprom_read_model(model);
	// if it was empty place, set it to defaults
	if (config_model.name[0] != CONFIG_MODEL_EMPTY)  return;
	config_model_set_default();
	return;
    }
    // it is in flash, read it and save to tmp eeprom
    flash_read_model((u8)(cg.model - CONFIG_MODEL_TMP));
    // if not configured, set to defaults before saving
    //   to tmp eeprom
    if (config_model.name[0] == CONFIG_MODEL_EMPTY)
	config_model_set_default();
    // save it to tmp eeprom
    eeprom_write_model(CONFIG_MODEL_TMP);
}


// return model name for given model number
u8 *config_model_name(u8 model) {
    @near static u8 fake_name[3];
    u8 *addr;

    if (model == cg.model)  return cm.name;	// actual model

    if (model < CONFIG_MODEL_TMP)
	// eeprom
	addr = EEPROM_CONFIG_MODEL + sizeof(config_model_s) * model;
    else
	// flash
	addr = (u8 *)(0 - sizeof(config_model_s) * (model - CONFIG_MODEL_TMP + 1));

    // if not configured, return default name
    if (*addr == CONFIG_MODEL_EMPTY) {
	default_model_name(model, fake_name);
	addr = fake_name;
    }
    return addr;
}




// save model config to eeprom (to temporary location if model is in FLASH)
void config_model_save(void) {
    u8 model = cg.model;
    if (model > CONFIG_MODEL_TMP)  model = CONFIG_MODEL_TMP;  // temporary place
    eeprom_write_model(model);
}


// set new global model, if previous model was from flash, save it
//   there firstly
void config_set_model(u8 model) {
    if (cg.model >= CONFIG_MODEL_TMP)
	flash_write_model((u8)(cg.model - CONFIG_MODEL_TMP));
    cg.model = model;
    config_global_save();
}




// read global config from eeprom, if MAGICs changed, set to defaults
// returns 1 if calibration values was set to defaults
u8 config_global_read(void) {
    u8 calib_changed = 0;

    eeprom_read_global();
    if (cg.magic_global != CONFIG_GLOBAL_MAGIC) {
	// global config changed, initialize whole eeprom
	eeprom_empty_models();
	calib_changed = config_global_set_default();
	// do not write magic_global yet to eliminate interrupted initialization
	//   (for example flash-verify after flash-write in STVP)
	cg.magic_global = 0;
	config_global_save();
	// and now as last set magic_global
	cg.magic_global = CONFIG_GLOBAL_MAGIC;
	config_global_save();
    }
    else if (cg.magic_model != CONFIG_MODEL_MAGIC) {
	// model config changed, empty all models
	eeprom_empty_models();
	// set model number to 0
	cg.model = 0;
	config_global_save();
	// set new model magic
	cg.magic_model = CONFIG_MODEL_MAGIC;
	config_global_save();
    }

    return calib_changed;
}





// get multi_position channel and array of values
// return number of multi_position items
// change function set_channel_MP() at menu_mix.c also
u8 config_get_MP(u8 index, u8 *pchannel_MP, s8 **pmulti_position) {
    switch (index) {
	case 0:
	    // multi position 0
	    *pchannel_MP = cm.channel_MP0;
	    *pmulti_position = cm.multi_position0;
	    return NUM_MULTI_POSITION0;
	case 1:
	    // multi position 1
	    *pchannel_MP = cm.channel_MP1;
	    *pmulti_position = cm.multi_position1;
	    return NUM_MULTI_POSITION1;
	case 2:
	    // multi position 2
	    *pchannel_MP = cm.channel_MP2;
	    *pmulti_position = cm.multi_position2;
	    return NUM_MULTI_POSITION2;
	case 3:
	    // multi position 3
	    *pchannel_MP = cm.channel_MP3;
	    *pmulti_position = cm.multi_position3;
	    return NUM_MULTI_POSITION3;
    }
    // shouldn't come here
    return 0;
}

