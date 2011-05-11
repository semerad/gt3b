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



#include "menu.h"
#include "config.h"
#include "calc.h"
#include "timer.h"
#include "ppm.h"
#include "lcd.h"
#include "buzzer.h"
#include "input.h"




_Bool menu_takes_adc;
_Bool menu_wants_battery;
_Bool menu_battery_low;




// calibrate menu
static void calibrate(void) {
    u8 channel = 1;
    u16 last_val = 0xffff;
    u16 val;
    u8 seg;

    menu_takes_adc = 1;

    // cleanup screen and disabble possible low bat warning
    buzzer_off();
    menu_battery_low = 0;	// it will bet automatically again
    lcd_clear();

    button_reset(BTN_ALL);

    // show intro text
    lcd_chars("CAL");
    lcd_update();
    delay_menu(200);

    // show channel number and not-yet calibrated values
    lcd_7seg(channel);
    lcd_menu(LM_MODEL | LM_NAME | LM_REV | LM_TRIM | LM_DR | LM_EXP);
    lcd_set_blink(LMENU, LB_SPC);

    while (1) {
	// check keys
	if (buttons & BTN_BACK)  break;
	if (buttons & (BTN_END | BTN_ROT_ALL)) {
	    // change channel number
	    if (buttons & BTN_ROT_L) {
		// down
		if (!--channel)  channel = 4;
	    }
	    else {
		// up
		if (++channel > 4)  channel = 1;
	    }
	    lcd_7seg(channel);
	    key_beep();
	}
	else if (buttons & BTN_ENTER) {
	    // save calibrate value for channels 1 and 2
	    if (channel == 1) {
		val = adc_steering_ovs >> ADC_OVS_SHIFT;
		if (val < CALIB_ST_LOW_MID) {
		    cg.calib_steering[0] = val;
		    seg = LS_MENU_MODEL;
		}
		else if (val <= CALIB_ST_MID_HIGH) {
		    cg.calib_steering[1] = val;
		    seg = LS_MENU_NAME;
		}
		else {
		    cg.calib_steering[2] = val;
		    seg = LS_MENU_REV;
		}
		lcd_segment(seg, LS_OFF);
		key_beep();
	    }
	    else if (channel == 2) {
		val = adc_throttle_ovs >> ADC_OVS_SHIFT;
		if (val < CALIB_TH_LOW_MID) {
		    cg.calib_throttle[0] = val;
		    seg = LS_MENU_TRIM;
		}
		else if (val <= CALIB_TH_MID_HIGH) {
		    cg.calib_throttle[1] = val;
		    seg = LS_MENU_DR;
		}
		else {
		    cg.calib_throttle[2] = val;
		    seg = LS_MENU_EXP;
		}
		lcd_segment(seg, LS_OFF);  // set corresponding LCD off
		key_beep();
	    }
	}

	// show ADC value if other than last val
	if (channel == 4)  val = adc_battery_filt;
	else  val = adc_all_ovs[channel] >> ADC_OVS_SHIFT;
	if (val != last_val) {
	    last_val = val;
	    lcd_char_num3(val);
	    lcd_update();
	}

	button_reset(BTN_ALL);
	stop();
    }

    menu_takes_adc = 0;
    beep(60);
    config_global_save();
}


// key test menu
static void key_test(void) {
    
}


// show model number, extra function to handle more than 10 models
static void show_model_number(u8 model) {
    lcd_7seg(cg.model);
}


// show main screen (model number and name)
static void main_screen(void) {
    lcd_clear();
    show_model_number(cg.model);
    lcd_chars(cm.name);
}


// menu stop - checks low battery
static void menu_stop(void) {
    static _Bool battery_low_on;
    stop();
    if (battery_low_on == menu_battery_low)  return;
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
}


// main menu loop, shows main screen and menus
static void menu_loop(void) {
    main_screen();
    while (1) {

	menu_stop();
    }
}



// initialize variables
void menu_init(void) {
    // variables
    
    // read global config from eeprom
    if (config_global_read())
	calibrate();

    // read model config from eeprom
    config_model_read();
    ppm_set_channels(cm.channels);

    // and main loop
    menu_loop();
}

