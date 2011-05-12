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




// flags for wakeup after each ADC measure
_Bool menu_takes_adc;
_Bool menu_wants_battery;
// battery low flag
_Bool menu_battery_low;




// calibrate menu
static void calibrate(void) {
    u8 channel = 1;
    u16 last_val = 0xffff;
    u16 val;
    u8 seg;

    menu_takes_adc = 1;

    // cleanup screen and disable possible low bat warning
    buzzer_off();
    menu_battery_low = 0;	// it will be set automatically again
    lcd_clear();

    btnra();

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
	if (btn(BTN_BACK))  break;
	if (btn(BTN_END | BTN_ROT_ALL)) {
	    key_beep();
	    // change channel number
	    if (btn(BTN_ROT_L)) {
		// down
		if (!--channel)  channel = 4;
	    }
	    else {
		// up
		if (++channel > 4)  channel = 1;
	    }
	    lcd_7seg(channel);
	    lcd_update();
	}
	else if (btn(BTN_ENTER)) {
	    // save calibrate value for channels 1 and 2
	    if (channel == 1) {
		key_beep();
		val = adc_steering_ovs >> ADC_OVS_SHIFT;
		if (val < CALIB_ST_LOW_MID) {
		    cg.calib_steering_left = val;
		    seg = LS_MENU_MODEL;
		}
		else if (val <= CALIB_ST_MID_HIGH) {
		    cg.calib_steering_mid = val;
		    seg = LS_MENU_NAME;
		}
		else {
		    cg.calib_steering_right = val;
		    seg = LS_MENU_REV;
		}
		lcd_segment(seg, LS_OFF);
		lcd_update();
	    }
	    else if (channel == 2) {
		key_beep();
		val = adc_throttle_ovs >> ADC_OVS_SHIFT;
		if (val < CALIB_TH_LOW_MID) {
		    cg.calib_throttle_fwd = val;
		    seg = LS_MENU_TRIM;
		}
		else if (val <= CALIB_TH_MID_HIGH) {
		    cg.calib_throttle_mid = val;
		    seg = LS_MENU_DR;
		}
		else {
		    cg.calib_throttle_bck = val;
		    seg = LS_MENU_EXP;
		}
		lcd_segment(seg, LS_OFF);  // set corresponding LCD off
		lcd_update();
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

	btnra();
	stop();
    }

    menu_takes_adc = 0;
    beep(60);
    lcd_menu(0);
    lcd_update();
    config_global_save();
}


// key test menu
static u8 *key_ids[] = {
    "T1L", "T1R",
    "T2F", "T2B",
    "T3-", "T3+",
    "DR-", "DR+",
    "ENT",
    "BCK",
    "END",
    "CH3",
    "ROL", "ROR"
};
static void key_test(void) {
    u8 i;
    u16 bit;
    
    // cleanup screen and disable possible low bat warning
    buzzer_off();
    menu_battery_low = 0;	// it will be set automatically again

    // do full screen blink
    lcd_set_full_on();
    delay_menu(200);
    lcd_clear();

    btnra();

    // show intro text
    lcd_chars("KEY");
    lcd_update_stop();		// wait for key

    while (1) {
	if (btnl(BTN_BACK))  break;

	for (i = 0, bit = 1; i < 14; i++, bit <<= 1) {
	    if (btn(bit)) {
		key_beep();
		lcd_chars(key_ids[i]);
		if (btnl(bit))  lcd_7seg(1);
		else lcd_set(L7SEG, LB_EMPTY);
		lcd_update();
		break;
	    }
	}
	btnra();
	stop();
    }
    key_beep();
}


// show model number, extra function to handle more than 10 models
static void show_model_number(u8 model) {
    lcd_7seg((u8)(cg.model % 10));
    if (model >= 40) {
	// too much, set blinking arrows
	lcd_segment(LS_SYM_RIGHT, LS_ON);
	lcd_segment(LS_SYM_LEFT,  LS_ON);
	lcd_segment_blink(LS_SYM_RIGHT, LB_SPC);
	lcd_segment_blink(LS_SYM_LEFT,  LB_SPC);
    }
    else {
	lcd_segment(LS_SYM_RIGHT, (u8)((u8)(model / 10) & 1));
	lcd_segment(LS_SYM_LEFT, (u8)((u8)(model / 20) & 1));
    }
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
static void main_screen(u8 item) {
    // model number, use arrows for > 10
    lcd_segment(LS_SYM_MODELNO, LS_ON);
    show_model_number(cg.model);

    // chars is item dependent
    if (item == 0) {
	// model name
	lcd_segment(LS_SYM_DOT, LS_OFF);
	lcd_segment(LS_SYM_VOLTS, LS_OFF);
	lcd_chars(cm.name);
    }
    else if (item == 1) {
	// battery voltage
	lcd_segment(LS_SYM_DOT, LS_OFF);
	lcd_segment(LS_SYM_VOLTS, LS_OFF);
	lcd_chars("XXX");
    }
    lcd_update();
}


// choose from menu items
static void select_menu(void) {
    u8 menu = LM_MODEL;
    lcd_menu(menu);

    while (1) {
	// Back key
	if (btn(BTN_BACK))  break;

	// Enter key
	if (btn(BTN_ENTER)) {
	    key_beep();
	    // XXX enter submenu
	}

	// rotate keys
	if (btn(BTN_ROT_R)) {
	    key_beep();
	    menu >>= 1;
	    if (!menu)  menu = LM_MODEL;
	    lcd_menu(menu);
	}
	if (btn(BTN_ROT_L)) {
	    key_beep();
	    menu <<= 1;
	    if (!menu)  menu = LM_ABS;
	    lcd_menu(menu);
	}

	btnra();
	lcd_update();
	menu_stop();
    }

    key_beep();
    lcd_menu(0);
}


// main menu loop, shows main screen and handle keys
static void menu_loop(void) {
    u8 item = 0;

    lcd_clear();
    main_screen(item);
    btnra();

    while (1) {
	// Enter key
	if (btn(BTN_ENTER)) {
	    key_beep();
	    if (adc_steering_ovs > (CALIB_ST_MID_HIGH << ADC_OVS_SHIFT))
		calibrate();
	    else if (adc_steering_ovs < (CALIB_ST_LOW_MID << ADC_OVS_SHIFT))
		key_test();
	    else  select_menu();
	    main_screen(item);
	}

	// trims

	// dualrate

	// channel 3 button

	// rotate encoder - change model name/battery/...
	if (btn(BTN_ROT_ALL)) {
	    key_beep();
	    item = 1 - item;
	    main_screen(item);
	}

	btnra();
	menu_stop();
    }
}



// read config from eeprom, initialize and call menu loop
void menu_init(void) {
    // variables

    // read global config from eeprom, if calibrate values changed,
    //   call calibrate
    if (config_global_read())
	calibrate();
    // apply global settings
    button_autorepeat(cg.autorepeat);

    // read model config from eeprom
    config_model_read();
    // apply model settings
    ppm_set_channels(cm.channels);

    // and main loop
    menu_loop();
}

