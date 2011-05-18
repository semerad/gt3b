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








// steps: 15s 30s 1m 2m 5m 10m 20m 30m 1h 2h 5h MAX
static const u16 bl_steps[] = {
    15, 30, 45,
    60, 2*60, 5*60, 10*60, 20*60, 30*60,
    3600, 2*3600, 5*3600,
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
    lcd_7seg(L7_B);
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


static void gs_trim_step(u8 change) {
    u8 *addr = &cg.trim_step;
    if (change == 0xff) {
	lcd_segment(LS_MENU_TRIM, LS_OFF);
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  *addr = (u8)menu_change_val(*addr, 1, 20, 2, 0);
    lcd_segment(LS_MENU_TRIM, LS_ON);
    lcd_7seg(5);
    lcd_char_num3(*addr);
}


static void gs_endpoint_max(u8 change) {
    u8 *addr = &cg.endpoint_max;
    if (change == 0xff) {
	lcd_segment(LS_MENU_EPO, LS_OFF);
	lcd_segment(LS_SYM_PERCENT, LS_OFF);
	return;
    }
    if (change)  *addr = (u8)menu_change_val(*addr, 100, 150, 5, 0);
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    lcd_segment(LS_MENU_EPO, LS_ON);
    lcd_char_num3(*addr);
}


static void gs_key_beep(u8 change) {
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  cg.key_beep ^= 1;
    lcd_7seg(L7_K);
    if (cg.key_beep)  lcd_chars("ON ");
    else              lcd_chars("OFF");
}


static void gs_ch3_momentary(u8 change) {
    if (change == 0xff) {
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change) {
	cg.ch3_momentary ^= 1;
	ch3_state = 0;
    }
    lcd_7seg(3);
    if (cg.ch3_momentary)  lcd_chars("ON ");
    else                   lcd_chars("OFF");
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


static void gs_trim_autorepeat(u8 change) {
    if (change == 0xff) {
	lcd_segment(LS_MENU_TRIM, LS_OFF);
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  cg.autorepeat ^= BTN_TRIM_LEFT | BTN_TRIM_RIGHT | BTN_TRIM_FWD | BTN_TRIM_BCK;
    lcd_segment(LS_MENU_TRIM, LS_ON);
    lcd_7seg(L7_A);
    if (cg.autorepeat & BTN_TRIM_LEFT)  lcd_chars("ON ");
    else				lcd_chars("OFF");
}


static void gs_dr_autorepeat(u8 change) {
    if (change == 0xff) {
	lcd_segment(LS_MENU_DR, LS_OFF);
	lcd_set(L7SEG, LB_EMPTY);
	return;
    }
    if (change)  cg.autorepeat ^= BTN_DR_L | BTN_DR_R;
    lcd_segment(LS_MENU_DR, LS_ON);
    lcd_7seg(L7_A);
    if (cg.autorepeat & BTN_DR_L)  lcd_chars("ON ");
    else			   lcd_chars("OFF");
}





// array of functions for global setup items
typedef void (*global_setup_t)(u8 change);
static const global_setup_t gs_config[] = {
    gs_backlight_time,
    gs_battery_low,
    gs_endpoint_max,
    gs_trim_step,
    gs_steering_dead,
    gs_throttle_dead,
    gs_ch3_momentary,
    gs_key_beep,
    gs_trim_autorepeat,
    gs_dr_autorepeat
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

	if (btnl(BTN_BACK))  break;

	if (btn(BTN_ENTER)) {
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

    beep(60);
    lcd_clear();
    config_global_save();
    apply_global_config();
}

