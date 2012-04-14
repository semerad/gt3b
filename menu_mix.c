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






static u8 mix_4WS(u8 val_id, u8 action, u8 *chars_blink) {
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
	if (!cm.channel_4WS)  id = 1;
    }

    // show value
    lcd_7seg(4);
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


static u8 mix_DIG(u8 val_id, u8 action, u8 *chars_blink) {
    u8 id = val_id;
    u8 val;

    if (action == 1) {
	// change value
	switch (id) {
	    case 1:
		// channel number/off
		val = cm.channel_DIG;
		if (!val)  val = 2;
		val = (u8)menu_change_val(val, 1, MAX_CHANNELS, 1, 1);
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
	if (!cm.channel_DIG)  id = 1;
    }

    // show value
    lcd_7seg(L7_D);
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


static u8 mix_MultiPosition(u8 val_id, u8 action, u8 *chars_blink) {
    u8 id = val_id;
    s8 val;

    if (action == 1) {
	// change value
	if (id == 1) {
	    // channel number/off
	    val = cm.channel_MP;
	    if (!val)  val = 2;
	    else if (val == MP_DIG)  val = MAX_CHANNELS + 1;
	    val = (u8)menu_change_val(val, 2, MAX_CHANNELS + 1, 1, 1);
	    if (val == 2)   			cm.channel_MP = 0;
	    else if (val == MAX_CHANNELS + 1)	cm.channel_MP = MP_DIG;
	    else	    			cm.channel_MP = val;
	}
	else {
	    // position value + END state (END not for first position)
	    val = cm.multi_position[id - 2];
	    if (val == MULTI_POSITION_END)  val = -101;
	    val = (s8)menu_change_val(val, id == 2 ? -100 : -101, 100,
				      CHANNEL_FAST, 0);
	    if (val == -101) {
		// set all from this to END value
		memset(&cm.multi_position[id - 2], (u8)MULTI_POSITION_END,
		       NUM_MULTI_POSITION + 2 - id);
	    }
	    else cm.multi_position[id - 2] = val;
	}
    }
    else if (action == 2) {
	// select next value
	if (cm.channel_MP) {
	    if (id == 1)  id = 2;
	    else if (cm.multi_position[id - 2] == MULTI_POSITION_END
		    || ++id > (NUM_MULTI_POSITION + 1))  id = 1;
	}
	// allow forcing channel value
	if (id > 1 && cm.channel_MP && cm.channel_MP <= MAX_CHANNELS) {
	    menu_force_value_channel = cm.channel_MP;
	}
	else menu_force_value_channel = 0;
    }

    // show value
    lcd_7seg(L7_P);
    if (id == 1) {
	// channel number/OFF
	if (!cm.channel_MP)	lcd_chars("OFF");
	else if (cm.channel_MP == MP_DIG)
				lcd_chars("DIG");
	else			lcd_char_num3(cm.channel_MP);
    }
    else {
	// position value
	val = cm.multi_position[id - 2];
	if (val == MULTI_POSITION_END) {
	    lcd_chars("END");
	    val = -100;
	}
	else  lcd_char_num3(val);
	if (cm.channel_MP == MP_DIG)	menu_DIG_mix = val;
	else				menu_force_value = val * PPM(5);
    }

    return id;
}


static u8 mix_brake_off(u8 val_id, u8 action, u8 *chars_blink) {

    if (action == 1) {
	// change value
	cm.brake_off ^= 1;
    }

    // show value
    lcd_7seg(L7_B);
    lcd_chars(cm.brake_off ? "CUT" : "OFF");

    return 1;
}






static const menu_func_t mix_funcs[] = {
    mix_4WS,
    mix_DIG,
    mix_MultiPosition,
    mix_brake_off,
};


void menu_mix(void) {
    lcd_set_blink(LMENU, LB_SPC);
    menu_common(mix_funcs, sizeof(mix_funcs) / sizeof(void *));
    lcd_set_blink(LMENU, LB_OFF);
    config_model_save();
    apply_model_config();
}

