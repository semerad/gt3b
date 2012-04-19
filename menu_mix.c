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






static void mix_4WS(u8 action) {
    u8 val;

    if (action == MLA_CHG) {
	// change value
	switch (menu_set) {
	    case 0:
		// channel number/off
		val = cm.channel_4WS;
		if (!val)  val = 2;
		val = (u8)menu_change_val(val, 2, channels, 1, 1);
		if (val == 2)   cm.channel_4WS = 0;
		else		cm.channel_4WS = val;
		break;
	    case 1:
		// mix value
		menu_4WS_mix = (s8)menu_change_val(menu_4WS_mix, -100, 100,
						   MIX_FAST, 0);
		break;
	    case 2:
		// crab/no-crab
		menu_4WS_crab ^= 1;
	}
    }
    else if (action == MLA_NEXT) {
	// select next value
	if (++menu_set > 2)   menu_set = 0;
	if (!cm.channel_4WS)  menu_set = 0;
    }

    // show value
    lcd_7seg(4);
    switch (menu_set) {
	case 0:
	    // channel number/OFF
	    if (!cm.channel_4WS)  lcd_chars("OFF");
	    else		  lcd_char_num3(cm.channel_4WS);
	    lcd_segment(LS_SYM_CHANNEL, LS_ON);
	    break;
	case 1:
	    // mix value
	    lcd_char_num3(menu_4WS_mix);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    break;
	case 2:
	    // crab/no-crab
	    lcd_chars("CR");
	    lcd_char(LCHR3, (u8)(menu_4WS_crab + '0'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);
    }
}


static void mix_DIG(u8 action) {
    u8 val;

    if (action == MLA_CHG) {
	// change value
	switch (menu_set) {
	    case 0:
		// channel number/off
		val = cm.channel_DIG;
		if (!val)  val = 2;
		val = (u8)menu_change_val(val, 1, channels, 1, 1);
		if (val == 2)   cm.channel_DIG = 0;
		else		cm.channel_DIG = val;
		break;
	    case 1:
		// mix value
		menu_DIG_mix = (s8)menu_change_val(menu_DIG_mix, -100, 100,
						   MIX_FAST, 0);
		break;
	}
    }
    else if (action == MLA_NEXT) {
	// select next value
	if (++menu_set > 1)   menu_set = 0;
	if (!cm.channel_DIG)  menu_set = 0;
    }

    // show value
    lcd_7seg(L7_D);
    switch (menu_set) {
	case 0:
	    // channel number/OFF
	    if (!cm.channel_DIG)  lcd_chars("OFF");
	    else		  lcd_char_num3(cm.channel_DIG);
	    lcd_segment(LS_SYM_CHANNEL, LS_ON);
	    break;
	case 1:
	    // mix value
	    lcd_char_num3(menu_DIG_mix);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    break;
    }
}


static void mix_MultiPosition(u8 action) {
    s8 val;

    if (action == MLA_CHG) {
	// change value
	if (menu_set == 0) {
	    // channel number/off
	    val = cm.channel_MP;
	    if (!val)  val = 2;
	    else if (val == MP_DIG)  val = (s8)(channels + 1);
	    val = (u8)menu_change_val(val, 2, channels + 1, 1, 1);
	    if (val == 2)   			cm.channel_MP = 0;
	    else if (val == (s8)(channels + 1))	cm.channel_MP = MP_DIG;
	    else	    			cm.channel_MP = val;
	}
	else {
	    // position value + END state (END not for first position)
	    val = cm.multi_position[menu_set - 1];
	    if (val == MULTI_POSITION_END)  val = -101;
	    val = (s8)menu_change_val(val, menu_set == 1 ? -100 : -101, 100,
				      CHANNEL_FAST, 0);
	    if (val == -101) {
		// set all from this to END value
		memset(&cm.multi_position[menu_set - 1], (u8)MULTI_POSITION_END,
		       NUM_MULTI_POSITION + 1 - menu_set);
	    }
	    else cm.multi_position[menu_set - 1] = val;
	}
    }
    else if (action == MLA_NEXT) {
	// select next value
	if (cm.channel_MP) {
	    if (menu_set == 0)  menu_set = 1;
	    else if (cm.multi_position[menu_set - 1] == MULTI_POSITION_END
		    || ++menu_set > NUM_MULTI_POSITION)  menu_set = 0;
	}
	// allow forcing channel value
	if (menu_set && cm.channel_MP && cm.channel_MP <= channels) {
	    menu_force_value_channel = cm.channel_MP;
	}
	else menu_force_value_channel = 0;
    }

    // show value
    lcd_7seg(L7_P);
    if (menu_set == 0) {
	// channel number/OFF
	if (!cm.channel_MP)	lcd_chars("OFF");
	else if (cm.channel_MP == MP_DIG)
				lcd_chars("DIG");
	else			lcd_char_num3(cm.channel_MP);
	lcd_segment(LS_SYM_CHANNEL, LS_ON);
    }
    else {
	// position value
	val = cm.multi_position[menu_set - 1];
	if (val == MULTI_POSITION_END) {
	    lcd_chars("END");
	    val = -100;
	}
	else  lcd_char_num3(val);
	if (cm.channel_MP == MP_DIG)	menu_DIG_mix = val;
	else				menu_force_value = val * PPM(5);
    }
}


static void mix_brake_off(u8 action) {
    // change value
    if (action == MLA_CHG)  cm.brake_off ^= 1;

    // show value
    lcd_7seg(L7_B);
    lcd_chars(cm.brake_off ? "CUT" : "OFF");
}






static const menu_list_t mix_funcs[] = {
    mix_4WS,
    mix_DIG,
    mix_MultiPosition,
    mix_brake_off,
};


void menu_mix(void) {
    lcd_set_blink(LMENU, LB_SPC);
    menu_list(mix_funcs, sizeof(mix_funcs) / sizeof(void *), MCF_NONE);
    lcd_set_blink(LMENU, LB_OFF);
    config_model_save();
    apply_model_config();
}

