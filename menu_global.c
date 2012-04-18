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
static void gs_firmware(u8 action) {
    // only show, not possible to change
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


// set backlight time
static void gs_backlight_time(u8 action) {
    s8 i;
    u16 *addr = &cg.backlight_time;

    // change value
    if (action == MLA_CHG) {
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
}


// set inactivity alarm
static void gs_inactivity_alarm(u8 action) {

    // change value
    if (action == MLA_CHG) {
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
}


// set battery low voltage
static void gs_battery_low(u8 action) {
    u8 *addr = &cg.battery_low;

    // change value
    if (action == MLA_CHG)
	*addr = (u8)menu_change_val(*addr, 20, 105, 2, 0);

    // show value, it isn't at 7SEG, so do more things
    lcd_set(L7SEG, LB_EMPTY);
    lcd_segment(LS_SYM_LOWPWR, LS_ON);
    lcd_segment_blink(LS_SYM_LOWPWR, LS_ON);
    lcd_segment(LS_SYM_DOT, LS_ON);
    lcd_segment(LS_SYM_VOLTS, LS_ON);
    lcd_char_num3(cg.battery_low);
}


// set max endpoint value
static void gs_endpoint_max(u8 action) {
    u8 *addr = &cg.endpoint_max;

    // change value
    if (action == MLA_CHG)
	*addr = (u8)menu_change_val(*addr, 100, 150, 5, 0);

    // show value
    lcd_7seg(L7_E);
    lcd_segment(LS_SYM_PERCENT, LS_ON);
    lcd_char_num3(*addr);
    if (*addr > 120)  lcd_segment_blink(LS_SYM_PERCENT, LS_ON);
}


// set dead zones and ADC oversampled/last
static void gs_adc(u8 action) {
    u8 *addr;

    // change value
    if (action == MLA_CHG) {
	switch (menu_set) {
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
    else if (action == MLA_NEXT) {
	if (++menu_set > 3)  menu_set = 1;
    }

    // show values
    lcd_7seg(L7_A);
    switch (menu_set) {
	case 1:
	    lcd_char_num2_lbl(cg.steering_dead_zone, "SSS");
	    menu_blink &= (u8)~MCB_CHR1;	// last 2 chars will blink
	    break;
	case 2:
	    lcd_char_num2_lbl(cg.throttle_dead_zone, "TTT");
	    menu_blink &= (u8)~MCB_CHR1;	// last 2 chars will blink
	    break;
	case 3:
	    lcd_char(LCHR1, 'A');
	    lcd_char(LCHR2, ' ');
	    lcd_char(LCHR3, (u8)(cg.adc_ovs_last ? '1' : '4'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);	// only last char will blink
	    break;
    }
}


// set beeps for key, center/reset, poweron
static void gs_beep(u8 action) {
    // change value
    if (action == MLA_CHG) {
	switch (menu_set) {
	    case 1:
		cg.key_beep ^= 1;
		break;
	    case 2:
		cg.reset_beep ^= 1;
		break;
	    case 3:
		cg.poweron_beep ^= 1;
		break;
	    case 4:
		cg.poweron_warn ^= 1;
		break;
	}
    }

    // select next value
    else if (action == MLA_NEXT) {
	if (++menu_set > 4)  menu_set = 1;
    }

    // show values
    lcd_7seg(L7_B);
    lcd_char(LCHR2, ' ');
    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);	// only last char will blink
    switch (menu_set) {
	case 1:
	    lcd_char(LCHR1, 'K');
	    lcd_char(LCHR3, (u8)(cg.key_beep ? 'Y' : 'N'));
	    break;
	case 2:
	    lcd_char(LCHR1, 'V');
	    lcd_char(LCHR3, (u8)(cg.reset_beep ? 'Y' : 'N'));
	    break;
	case 3:
	    lcd_char(LCHR1, 'P');
	    lcd_char(LCHR3, (u8)(cg.poweron_beep ? 'Y' : 'N'));
	    break;
	case 4:
	    lcd_char(LCHR1, 'C');
	    lcd_char(LCHR3, (u8)(cg.poweron_warn ? 'Y' : 'N'));
	    break;
    }
}


// set long press delay
static void gs_long_press_delay(u8 action) {
    u8 *addr = &cg.long_press_delay;

    // change value
    if (action == MLA_CHG)
	*addr = (u8)menu_change_val(*addr, 20, 200, 5, 0);

    // show value
    lcd_7seg(L7_D);
    lcd_char_num3(*addr * 5);
}


// reset global or all models
static void gs_config_reset(u8 action) {
    // change value
    if (action == MLA_CHG)
	menu_tmp_flag ^= 1;

    // select next value, reset when flag is set
    else if (action == MLA_NEXT) {
	if (menu_tmp_flag) {
	    menu_tmp_flag = 0;
	    buzzer_on(60, 0, 1);
	    if (menu_set == 1)  config_global_set_default();
	    else          	cg.model = 0;
	    config_global_save();
	    eeprom_empty_models();
	    menu_load_model();
	}
	if (++menu_set > 2)  menu_set = 1;
    }

    // show values
    lcd_7seg(L7_R);
    lcd_char(LCHR2, ' ');
    lcd_char(LCHR3, (u8)(menu_tmp_flag ? 'Y' : 'N'));
    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);	// only last char will blink
    lcd_char(LCHR1, (u8)(menu_set == 1 ? 'G' : 'M'));
}






// array of functions for global setup items
static const menu_list_t gs_config[] = {
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

    menu_list(gs_config, sizeof(gs_config) / sizeof(void *), (u8)(MCF_STOP | MCF_LOWPWR));

    buzzer_on(60, 0, 1);	// not beep() because of key_beep() at menu_list()
    lcd_clear();
    config_global_save();
    apply_global_config();
    btnra();
}

