/*
    menu_mix - handle menus for mix settings
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
#include "menu.h"
#include "config.h"
#include "calc.h"
#include "timer.h"
#include "ppm.h"
#include "lcd.h"
#include "buzzer.h"
#include "input.h"






static u8 mix_4WS(u8 val_id, u8 action) {
    u8 id = val_id;
    u8 val;

    if (action == 1) {
	// change value
	switch (id) {
	    case 1:
		// channel number/off
		val = cm.channel_4WS;
		if (!val)  val = 2;
		val = (u8)menu_change_val(val, 2, MAX_CHANNELS, 1, 1);
		if (val == 2)   cm.channel_4WS = 0;
		else		cm.channel_4WS = val;
		break;
	    case 2:
		// mix value
		menu_4WS_mix = (s8)menu_change_val(menu_4WS_mix, -100, 100,
						   MIX_FAST, 0);
		break;
	    case 3:
		// crab/no-crab
		menu_4WS_crab ^= 1;
	}
    }
    else if (action == 2) {
	// select next value
	if (++id > 3)  id = 1;
    }

    // show value
    switch (id) {
	case 1:
	    // channel number/OFF
	    if (!cm.channel_4WS)  lcd_chars("OFF");
	    else		  lcd_char_num3(cm.channel_4WS);
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 2:
	    // mix value
	    lcd_char_num3(menu_4WS_mix);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 3:
	    // crab/no-crab
	    lcd_chars(menu_4WS_crab ? "CRB" : "NOC");
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
    }

    return id;

}


static u8 mix_DIG(u8 val_id, u8 action) {
    u8 id = val_id;
    u8 val;

    if (action == 1) {
	// change value
	switch (id) {
	    case 1:
		// channel number/off
		val = cm.channel_DIG;
		if (!val)  val = 2;
		val = (u8)menu_change_val(val, 2, MAX_CHANNELS, 1, 1);
		if (val == 2)   cm.channel_DIG = 0;
		else		cm.channel_DIG = val;
		break;
	    case 2:
		// mix value
		menu_DIG_mix = (s8)menu_change_val(menu_DIG_mix, -100, 100,
						   MIX_FAST, 0);
		break;
	}
    }
    else if (action == 2) {
	// select next value
	if (++id > 2)  id = 1;
    }

    // show value
    switch (id) {
	case 1:
	    // channel number/OFF
	    if (!cm.channel_DIG)  lcd_chars("OFF");
	    else		  lcd_char_num3(cm.channel_DIG);
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    break;
	case 2:
	    // mix value
	    lcd_char_num3(menu_DIG_mix);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    break;
    }

    return id;
}





typedef u8 (*mix_func_t)(u8 val_id, u8 action);

void menu_mix(void) {
    u8 id_val = 0;			// now in key_id
    u8 menu_id = 0;
    static const u8 menu_ids[] = {
	4, L7_D,
    };
    static const mix_func_t menu_funcs[] = {
	mix_4WS, mix_DIG
    };
    mix_func_t func = menu_funcs[0];
    #define MAX_MENU_IDS  sizeof(menu_ids)


    lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_7seg(menu_ids[0]);
    lcd_set_blink(L7SEG, LB_SPC);
    func(1, 0);		// show first setting for first menu id
    lcd_update();

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END) || btnl(BTN_ENTER))  break;

	if (btn(BTN_ROT_ALL)) {
	    if (id_val) {
		// change selected setting
		func(id_val, 1);
		lcd_chars_blink(LB_SPC);
		lcd_update();
	    }
	    else {
		// change menu-id
		if (btn(BTN_ROT_L)) {
		    if (menu_id)  menu_id--;
		    else	  menu_id = MAX_MENU_IDS - 1;
		}
		else {
		    if (++menu_id >= MAX_MENU_IDS)  menu_id = 0;
		}
		func = menu_funcs[menu_id];
		lcd_7seg(menu_ids[menu_id]);
		// remove possible showed symbols
		lcd_segment(LS_SYM_PERCENT, LS_OFF);
		lcd_segment(LS_SYM_VOLTS, LS_OFF);
		func(1, 0);	// show first setting
		lcd_set_blink(L7SEG, LB_SPC);
		lcd_update();
	    }
	}

	else if (btn(BTN_ENTER)) {
	    // switch menu_id/menu-setting1/menu-setting2/...
	    if (id_val) {
		// what to do depends on what was selected in this item
		id_val = func(id_val, 2);
		if (id_val != 1) {
		    lcd_chars_blink(LB_SPC);
		}
		else {
		    // switch to menu selection
		    id_val = 0;
		    lcd_set_blink(L7SEG, LB_SPC);
		    lcd_chars_blink(LB_OFF);
		}
		lcd_update();
	    }
	    else {
		// switch to key settings
		id_val = 1;
		// key setting values is already showed
		lcd_set_blink(L7SEG, LB_OFF);
		lcd_chars_blink(LB_SPC);
	    }
	}
    }

    key_beep();
    lcd_set_blink(LMENU, LB_OFF);
    config_model_save();
    apply_model_config();

}


