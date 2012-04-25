/*
    menu_service - handle calibrate and key_test menus
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
#include "timer.h"
#include "lcd.h"
#include "buzzer.h"
#include "input.h"






// calibrate menu
void menu_calibrate(u8 at_poweron) {
    u8 channel = 1;
    u16 last_val = 0xffff;
    u16 val;
    u8 seg;
    u8 bat_volts;
    u16 update_time = 0;
    u16 update_val = 0;

    menu_adc_wakeup = 1;

    // cleanup screen and disable possible low bat warning
    buzzer_off();
    if (at_poweron)	buzzer_on(30, 30, 2);
    else		key_beep();
    menu_battery_low = 0;	// it will be set automatically again
    battery_low_shutup = 0;
    backlight_set_default(BACKLIGHT_MAX);
    backlight_on();
    lcd_clear();

    btnra();

    // show intro text
    lcd_chars("CAL");
    lcd_update();
    delay_menu_always(2);

    // show channel number and not-yet calibrated values
    lcd_segment(LS_SYM_CHANNEL, LS_ON);
    lcd_7seg(channel);
    lcd_menu(LM_MODEL | LM_NAME | LM_REV | LM_TRIM | LM_DR | LM_EXP);
    lcd_set_blink(LMENU, LB_SPC);
    // blink arrows for ch3 potentiometer also
    if (cg.ch3_pot) {
	lcd_segment(LS_SYM_LEFT, LS_ON);
	lcd_segment(LS_SYM_RIGHT, LS_ON);
	lcd_segment_blink(LS_SYM_LEFT, LB_SPC);
	lcd_segment_blink(LS_SYM_RIGHT, LB_SPC);
    }

    while (1) {
	// check keys
	if (btnl(BTN_BACK | BTN_ENTER))  break;

	if (btn(BTN_END | BTN_ROT_ALL)) {
	    if (btn(BTN_END))  key_beep();
	    // change channel number
	    channel = (u8)menu_change_val(channel, 1, 4, 1, 1);
	    lcd_7seg(channel);
	    lcd_update();
	    update_time = 0;
	}

	else if (btn(BTN_ENTER)) {
	    // save calibrate value for channels 1 and 2 (and 3 if ch3_pot)
	    // select actual voltage for channel 4
	    if (channel == 1) {
		key_beep();
		val = ADC_OVS(steering);
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
		val = ADC_OVS(throttle);
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
	    else if (channel == 3 && cg.ch3_pot) {
		// only if ch3 is potentiometer
		key_beep();
		val = ADC_OVS(ch3);
		if (val < 512) {
		    cg.calib_ch3_left = val;
		    seg = LS_SYM_LEFT;
		}
		else {
		    cg.calib_ch3_right = val;
		    seg = LS_SYM_RIGHT;
		}
		lcd_segment(seg, LS_OFF);  // set corresponding LCD off
		lcd_update();
	    }
	    else if (channel == 4) {
		key_beep();
		// allow to set actual battery voltage
		lcd_segment(LS_SYM_DOT, LS_ON);
		lcd_segment(LS_SYM_VOLTS, LS_ON);
		bat_volts = (u8)(((u32)adc_battery * 100 + 300) / cg.battery_calib);
		lcd_char_num3(bat_volts);
		lcd_update();

		while (1) {
		    btnra();
		    stop();

		    if (btnl(BTN_BACK) || btn(BTN_ENTER | BTN_END))  break;
		    if (btn(BTN_ROT_ALL)) {
			if (btn(BTN_ROT_L))  bat_volts--;
			else                 bat_volts++;
			lcd_char_num3(bat_volts);
			lcd_update();
		    }
		}

		key_beep();
		lcd_segment(LS_SYM_DOT, LS_OFF);
		lcd_segment(LS_SYM_VOLTS, LS_OFF);
		last_val = 0xffff;	// show ADC value
		if (!btn(BTN_END)) {
		    // recalculate calibrate value for 10V
		    cg.battery_calib = (u16)(((u32)adc_battery * 100 + 40) / bat_volts);
		    if (btnl(BTN_BACK | BTN_ENTER))  break;
		}
	    }
	}

	// show ADC value if other than last val
	switch (channel) {
	    case 1:
		val = ADC_OVS(steering);
		break;
	    case 2:
		val = ADC_OVS(throttle);
		break;
	    case 3:
		val = ADC_OVS(ch3);
		break;
	    case 4:
		val = adc_battery;
		break;
	    default:		// to eliminate compiler warning
		val = 0;
	}
	// only update display every 1s
	if (time_sec >= update_time) {
	    update_time = time_sec + 1;
	    update_val = val;
	}
	if (update_val != last_val) {
	    last_val = update_val;
	    lcd_char_num3(val);
	    lcd_update();
	}

	btnra();
	stop();
    }

    menu_adc_wakeup = 0;
    beep(60);
    lcd_menu(0);
    lcd_update();
    config_global_save();
    apply_global_config();
}






// key test menu
static const u8 key_ids[][4] = {
    "T1L", "T1R",
    "T2F", "T2B",
    "T3-", "T3+",
    "DR-", "DR+",
    "ENT",
    "BCK",
    "END",
    "CH3",
    "NO1", "NO2",
    "ROL", "ROR"
};
void menu_key_test(void) {
    u8 i;
    u16 bit;
    
    // cleanup screen and disable possible low bat warning
    buzzer_off();
    key_beep();
    menu_battery_low = 0;	// it will be set automatically again
    battery_low_shutup = 0;

    // do full screen blink
    lcd_set_full_on();
    delay_menu_always(2);
    while (btns(BTN_ENTER))  stop();  // wait for release of ENTER
    lcd_clear();

    button_autorepeat(0);	// disable autorepeats
    btnra();

    // show intro text
    lcd_chars("KEY");
    lcd_update_stop();		// wait for key

    while (1) {
	if (btnl(BTN_BACK | BTN_ENTER))  break;

	for (i = 0, bit = 1; i < 16; i++, bit <<= 1) {
	    if (btn(bit)) {
		key_beep();
		lcd_chars(key_ids[i]);
		if (btnl(bit))  lcd_7seg(L7_L);
		else lcd_set(L7SEG, LB_EMPTY);
		lcd_update();
		break;
	    }
	}
	btnra();
	stop();
    }
    key_beep();
    apply_model_config();
}

