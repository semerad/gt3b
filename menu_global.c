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
static u8 gs_firmware(u8 val_id, u8 action, u8 *chars_blink) {
    // only show, not possible to change
    lcd_7seg(L7_F);
    lcd_chars(VERSION);
    return 1;	// only one value
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

// set backlight time
static u8 gs_backlight_time(u8 val_id, u8 action, u8 *chars_blink) {
    s8 i;
    u16 *addr = &cg.backlight_time;

    // change value
    if (action == 1) {
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

    // show value
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

    return 1;	// only one value
}


// set inactivity alarm
static u8 gs_inactivity_alarm(u8 val_id, u8 action, u8 *chars_blink) {

    // change value
    if (action == 1) {
	cg.inactivity_alarm = (u8)menu_change_val(cg.inactivity_alarm, 0, 10,
						  1, 1);
	reset_inactivity_timer();
    }

    // show value
    lcd_7seg(1);
    if (!cg.inactivity_alarm)  lcd_chars("OFF");
    else {
	bl_num2(cg.inactivity_alarm);
	lcd_char(LCHR3, 'M');
    }

    return 1;	// only one value
}


// set battery low voltage
static u8 gs_battery_low(u8 val_id, u8 action, u8 *chars_blink) {
    u8 *addr = &cg.battery_low;

    // change value
    if (action == 1)
	*addr = (u8)menu_change_val(*addr, 20, 105, 2, 0);

    // show value, it isn't at 7SEG, so do more things
    lcd_set(L7SEG, LB_EMPTY);
    lcd_segment(LS_SYM_LOWPWR, LS_ON);
    lcd_segment_blink(LS_SYM_LOWPWR, LS_ON);
    lcd_segment(LS_SYM_DOT, LS_ON);
    lcd_segment(LS_SYM_VOLTS, LS_ON);
    lcd_char_num3(cg.battery_low);

    return 1;	// only one value
}


// set max endpoint value
static u8 gs_endpoint_max(u8 val_id, u8 action, u8 *chars_blink) {
    u8 *addr = &cg.endpoint_max;

    // change value
    if (action == 1)
	*addr = (u8)menu_change_val(*addr, 100, 150, 5, 0);

    // show value
    lcd_7seg(L7_E);
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    lcd_char_num3(*addr);
    if (*addr > 120)  lcd_segment_blink(LS_SYM_PERCENT, LS_ON);

    return 1;	// only one value
}


// set dead zones and ADC oversampled/last
static u8 gs_adc(u8 val_id, u8 action, u8 *chars_blink) {
    u8 id = val_id;
    u8 *addr;

    // change value
    if (action == 1) {
	switch (id) {
	    case 1:
		addr = &cg.steering_dead_zone;
		*addr = (u8)menu_change_val(*addr, 0, 50, 2, 0);
		break;
	    case 2:
		addr = &cg.throttle_dead_zone;
		*addr = (u8)menu_change_val(*addr, 0, 50, 2, 0);
		break;
	    case 3:
		cg.adc_ovs_last ^= 1;
		break;
	}
    }

    // select next value
    else if (action == 2) {
	if (++id > 3)  id = 1;
    }

    // show values
    lcd_7seg(L7_A);
    switch (id) {
	case 1:
	    lcd_char_num2_lbl(cg.steering_dead_zone, "SSS");
	    *chars_blink = 0b110;	// last 2 chars will blink
	    break;
	case 2:
	    lcd_char_num2_lbl(cg.throttle_dead_zone, "TTT");
	    *chars_blink = 0b110;	// last 2 chars will blink
	    break;
	case 3:
	    lcd_char(LCHR1, 'A');
	    lcd_char(LCHR2, ' ');
	    lcd_char(LCHR3, (u8)(cg.adc_ovs_last ? '1' : '4'));
	    *chars_blink = 0b100;	// only last char will blink
	    break;
    }

    return id;
}


// set beeps for key, center/reset, poweron
static u8 gs_beep(u8 val_id, u8 action, u8 *chars_blink) {
    u8 id = val_id;

    // change value
    if (action == 1) {
	switch (id) {
	    case 1:
		cg.key_beep ^= 1;
		break;
	    case 2:
		cg.reset_beep ^= 1;
		break;
	    case 3:
		cg.poweron_beep ^= 1;
		break;
	}
    }

    // select next value
    else if (action == 2) {
	if (++id > 3)  id = 1;
    }

    // show values
    lcd_7seg(L7_B);
    lcd_char(LCHR2, ' ');
    *chars_blink = 0b100;	// only last char will blink
    switch (id) {
	case 1:
	    lcd_char(LCHR1, 'K');
	    lcd_char(LCHR3, (u8)(cg.key_beep ? 'Y' : 'N'));
	    break;
	case 2:
	    lcd_char(LCHR1, 'C');
	    lcd_char(LCHR3, (u8)(cg.reset_beep ? 'Y' : 'N'));
	    break;
	case 3:
	    lcd_char(LCHR1, 'P');
	    lcd_char(LCHR3, (u8)(cg.poweron_beep ? 'Y' : 'N'));
	    break;
    }

    return id;
}

// set long press delay
static u8 gs_long_press_delay(u8 val_id, u8 action, u8 *chars_blink) {
    u8 *addr = &cg.long_press_delay;

    // change value
    if (action == 1)
	*addr = (u8)menu_change_val(*addr, 20, 200, 5, 0);

    // show value
    lcd_7seg(L7_D);
    lcd_char_num3(*addr * 5);

    return 1;	// only one value
}


// reset global or all models
static u8 gs_config_reset(u8 val_id, u8 action, u8 *chars_blink) {
    u8 id = val_id;

    // change value
    if (action == 1)
	menu_tmp_flag ^= 1;

    // select next value, reset when flag is set
    else if (action == 2) {
	if (menu_tmp_flag) {
	    menu_tmp_flag = 0;
	    buzzer_on(60, 0, 1);
	    if (id == 1)  config_global_set_default();
	    else          cg.model = 0;
	    config_global_save();
	    eeprom_empty_models();
	    menu_load_model();
	}
	if (++id > 2)  id = 1;
    }

    // show values
    lcd_7seg(L7_R);
    lcd_char(LCHR2, ' ');
    lcd_char(LCHR3, (u8)(menu_tmp_flag ? 'Y' : 'N'));
    *chars_blink = 0b100;	// only last char will blink
    lcd_char(LCHR1, (u8)(id == 1 ? 'G' : 'M'));

    return id;
}






// array of functions for global setup items
static const menu_func_t gs_config[] = {
    gs_firmware,
    gs_backlight_time,
    gs_inactivity_alarm,
    gs_battery_low,
    gs_endpoint_max,
    gs_adc,
    gs_beep,
    gs_long_press_delay,
    gs_config_reset,
};
#define GS_CONFIG_SIZE  (sizeof(gs_config) / sizeof(u8 *))






void menu_global_setup(void) {
    // cleanup screen and disable possible low bat warning
    buzzer_off();
    key_beep();
    menu_battery_low = 0;	// it will be set automatically again
    battery_low_shutup = 0;
    backlight_set_default(BACKLIGHT_MAX);
    backlight_on();
    lcd_clear();

    // show blinking MODEL + NAME
    lcd_segment(LS_MENU_MODEL, LS_ON);
    lcd_segment_blink(LS_MENU_MODEL, LB_INV);
    lcd_segment_blink(LS_MENU_NAME, LB_INV);

    menu_common(gs_config, sizeof(gs_config) / sizeof(void *), 1);

    buzzer_on(60, 0, 1);	// not beep() because of key_beep() at menu_common()
    lcd_clear();
    config_global_save();
    apply_global_config();
    btnra();
}

