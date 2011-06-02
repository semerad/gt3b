/*
    menu_global - handle global settings menus
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
#include "version.h"








// show firmware version
static void gs_firmware(u8 change) {
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    lcd_7seg(L7_F);
    lcd_chars(VERSION);
}


// steps in seconds and minutes
static const u16 bl_steps[] = {
    5, 10, 15, 20, 30, 45,
    60, 2*60, 5*60, 10*60,
    BACKLIGHT_MAX
};
#define BL_STEPS_SIZE  (sizeof(bl_steps) / sizeof(u16))
static void bl_num2(u8 val) {
    if (val < 10)  lcd_char(LCHR1, ' ');
    else           lcd_char(LCHR1, (u8)((u8)(val / 10) + '0'));
    lcd_char(LCHR2, (u8)((u8)(val % 10) + '0'));

}
static void gs_backlight_time(u8 change) {
    s8 i;
    u16 *addr = &cg.backlight_time;

    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change) {
	if (btn(BTN_ROT_L)) {
	    // find lower value
	    for (i = BL_STEPS_SIZE - 1; i >= 0; i--) {
		if (bl_steps[i] >= *addr)  continue;
		*addr = bl_steps[i];
		break;
	    }
	    if (i < 0)
		*addr = bl_steps[BL_STEPS_SIZE - 1];
	}
	else {
	    // find upper value
	    for (i = 0; i < BL_STEPS_SIZE; i++) {
		if (bl_steps[i] <= *addr)  continue;
		*addr = bl_steps[i];
		break;
	    }
	    if (i == BL_STEPS_SIZE)
		*addr = bl_steps[0];
	}
    }
    lcd_7seg(L7_L);
    if (*addr < 60) {
	// seconds
	bl_num2((u8)*addr);
	lcd_char(LCHR3, 'S');
    }
    else if (*addr < 3600) {
	// minutes
	bl_num2((u8)(*addr / 60));
	lcd_char(LCHR3, 'M');
    }
    else if (*addr != BACKLIGHT_MAX) {
	// hours
	bl_num2((u8)(*addr / 3600));
	lcd_char(LCHR3, 'H');
    }
    else {
	// max
	lcd_chars("MAX");
    }
}


static void gs_battery_low(u8 change) {
    u8 *addr = &cg.battery_low;
    if (change == 0xff) {
	lcd_segment(LS_SYM_LOWPWR, LS_OFF);
	lcd_segment(LS_SYM_DOT, LS_OFF);
	lcd_segment(LS_SYM_VOLTS, LS_OFF);
	return;
    }
    if (change)  *addr = (u8)menu_change_val(*addr, 75, 105, 2, 0);
    lcd_segment(LS_SYM_LOWPWR, LS_ON);
    lcd_segment(LS_SYM_DOT, LS_ON);
    lcd_segment(LS_SYM_VOLTS, LS_ON);
    lcd_char_num3(cg.battery_low);
}


static void gs_endpoint_max(u8 change) {
    u8 *addr = &cg.endpoint_max;
    if (change == 0xff) {
	lcd_segment(LS_MENU_EPO, LS_OFF);
	lcd_segment(LS_SYM_PERCENT, LS_OFF);
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  *addr = (u8)menu_change_val(*addr, 100, 150, 5, 0);
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    lcd_segment(LS_MENU_EPO, LS_ON);
    lcd_char_num3(*addr);
    if (*addr > 120) {
	lcd_7seg(L7_D);
	lcd_set_blink(L7SEG, LB_SPC);
    }
    else lcd_set(L7SEG, LB_EMPTY);
}


static void gs_key_beep(u8 change) {
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  cg.key_beep ^= 1;
    lcd_7seg(L7_B);
    if (cg.key_beep)  lcd_chars("ON ");
    else              lcd_chars("OFF");
}


static void gs_steering_dead(u8 change) {
    u8 *addr = &cg.steering_dead_zone;
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  *addr = (u8)menu_change_val(*addr, 0, 50, 2, 0);
    lcd_7seg(1);
    lcd_char_num3(*addr);
}


static void gs_throttle_dead(u8 change) {
    u8 *addr = &cg.throttle_dead_zone;
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  *addr = (u8)menu_change_val(*addr, 0, 50, 2, 0);
    lcd_7seg(2);
    lcd_char_num3(*addr);
}


static _Bool gs_reset_flag;
static void gs_reset_all(u8 change) {
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	if (gs_reset_flag) {
	    gs_reset_flag = 0;
	    config_global_set_default();
	    config_global_save();
	    eeprom_empty_models();
	    menu_load_model();
	}
	return;
    }
    if (change)  gs_reset_flag ^= 1;
    lcd_7seg(L7_R);
    if (gs_reset_flag)	lcd_chars("YES");
    else		lcd_chars("ALL");
}

static void gs_reset_model_all(u8 change) {
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	if (gs_reset_flag) {
	    gs_reset_flag = 0;
	    cg.model = 0;
	    config_global_save();
	    eeprom_empty_models();
	    menu_load_model();
	}
	return;
    }
    if (change)  gs_reset_flag ^= 1;
    lcd_7seg(L7_R);
    if (gs_reset_flag)	lcd_chars("YES");
    else		lcd_chars("MOD");
}




// array of functions for global setup items
typedef void (*global_setup_t)(u8 change);
static const global_setup_t gs_config[] = {
    gs_firmware,
    gs_backlight_time,
    gs_battery_low,
    gs_endpoint_max,
    gs_steering_dead,
    gs_throttle_dead,
    gs_key_beep,
    gs_reset_all,
    gs_reset_model_all,
};
#define GS_CONFIG_SIZE  (sizeof(gs_config) / sizeof(u8 *))






void menu_global_setup(void) {
    u8 item = 0;
    u8 item_val = 0;		// now selecting item
    global_setup_t func = gs_config[item];

    // cleanup screen and disable possible low bat warning
    buzzer_off();
    key_beep();
    menu_battery_low = 0;	// it will be set automatically again
    backlight_set_default(BACKLIGHT_MAX);
    backlight_on();
    lcd_clear();

    lcd_segment(LS_MENU_MODEL, LS_ON);
    lcd_segment_blink(LS_MENU_MODEL, LB_INV);
    lcd_segment_blink(LS_MENU_NAME, LB_INV);
    lcd_update();
    func(0);			// show current value

    while (1) {
	btnra();
	stop();

	if (btnl(BTN_BACK | BTN_ENTER))  break;

	if (btn(BTN_ENTER)) {
	    if (item > 0) {		// not for firmware version
		key_beep();
		item_val = (u8)(1 - item_val);
		if (item_val) {
		    // changing value
		    lcd_chars_blink(LB_SPC);
		}
		else {
		    // selecting item
		    lcd_chars_blink(LB_OFF);
		}
	    }
	}

	else if (btn(BTN_ROT_ALL)) {
	    if (item_val) {
		// change item value
		func(1);
		lcd_chars_blink(LB_SPC);
	    }
	    else {
		// select another item
		func(0xff);		// un-show labels
		if (btn(BTN_ROT_L)) {
		    if (item)  item--;
		    else       item = GS_CONFIG_SIZE - 1;
		}
		else {
		    if (++item >= GS_CONFIG_SIZE)  item = 0;
		}
		func = gs_config[item];
		func(0);		// show current value
	    }
	    lcd_update();
	}
    }

    func(0xff);		// un-show labels, apply resets
    beep(60);
    lcd_clear();
    config_global_save();
    apply_global_config();
}

