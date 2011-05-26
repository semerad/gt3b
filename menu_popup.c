/*
    menu - handle popup menus
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






// temporary show popup value (trim, subtrim, dualrate, ...)
// if another key pressed, return
static void menu_popup(u8 menu, u8 blink, u16 btn_l, u16 btn_r,
		       u8 channel, s8 *aval,
		       s16 min, s16 max, s16 reset, u8 step,
		       u8 rot_fast, u8 *labels) {
    u16 btn_lr = btn_l | btn_r;
    u16 to_time;
    s16 val;

    if (min >= 0)  val = *(u8 *)aval;	// *aval is unsigned
    else           val = *aval;		// *aval is signed

    // show MENU and CHANNEL
    lcd_segment(menu, LS_ON);
    if (blink) lcd_segment_blink(menu, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_segment(LS_SYM_CHANNEL, LS_ON);
    lcd_7seg(channel);

    while (1) {
	// check value left/right
	if (btnl_all(btn_lr)) {
	    // reset to given reset value
	    key_beep();
	    val = reset;
	    *aval = (s8)val;
	    btnr(btn_lr);
	}
	else if (btn(btn_lr)) {
	    if (!btns_all(btn_lr)) {
		// only when both are not pressed together
		key_beep();
		if (btn(btn_l)) {
		    val -= step;
		    if (val < min)  val = min;
		}
		else {
		    val += step;
		    if (val > max)  val = max;
		}
		*aval = (s8)val;
		btnr(btn_lr);
	    }
	    else btnr_nolong(btn_lr);  // keep long-presses for testing-both
	}
	else if (btn(BTN_ROT_ALL)) {
	    val = menu_change_val(val, min, max, rot_fast, 0);
	    *aval = (s8)val;
	}
	btnr(BTN_ROT_ALL);

	// if another button was pressed, leave this screen
	if (buttons)  break;

	// show current value
	if (labels)		lcd_char_num2_lbl((s8)val, labels);
	else			lcd_char_num3(val);
	lcd_update();

	// sleep 5s, and if no button was pressed during, end this screen
	to_time = time_sec + POPUP_DELAY;
	while (time_sec < to_time && !buttons)
	    delay_menu((to_time - time_sec) * 200);

	if (!buttons)  break;  // timeouted without button press
    }

    btnr(btn_lr);  // reset also long values

    // set selected MENU off
    lcd_segment(menu, LS_OFF);

    // save model config
    config_model_save();
}





// check trims and dualrate keys, invoke popup menu
// return 1 when popup was activated
u8 menu_electronic_trims(void) {
    // trims
    if (btn(BTN_TRIM_LEFT | BTN_TRIM_RIGHT)) {
	menu_popup(LS_MENU_TRIM, 0, BTN_TRIM_LEFT, BTN_TRIM_RIGHT,
		    1, &cm.trim[0],
		    -TRIM_MAX, TRIM_MAX, 0, cg.trim_step,
		    TRIM_FAST, "LNR");
	return 1;
    }
    else if (btn(BTN_TRIM_CH3_L | BTN_TRIM_CH3_R)) {
	menu_popup(LS_MENU_TRIM, 0, BTN_TRIM_CH3_L, BTN_TRIM_CH3_R,
		    1, &cm.trim[0],
		    -TRIM_MAX, TRIM_MAX, 0, cg.trim_step,
		    TRIM_FAST, "LNR");
	return 1;
    }
    else if (btn(BTN_TRIM_FWD | BTN_TRIM_BCK)) {
	menu_popup(LS_MENU_TRIM, 0, BTN_TRIM_FWD, BTN_TRIM_BCK,
		    2, &cm.trim[1],
		    -TRIM_MAX, TRIM_MAX, 0, cg.trim_step,
		    SUBTRIM_FAST, "FNB");
	return 1;
    }
    // dualrate
    else if (btn(BTN_DR_L | BTN_DR_R)) {
	menu_popup(LS_MENU_DR, 0, BTN_DR_L, BTN_DR_R,
		    1, (s8 *)&cm.dr_steering,
		    0, 100, 100, 1,
		    DUALRATE_FAST, NULL);
	return 1;
    }

    return 0;
}





// check buttons CH3, BACK, END, invoke popup to show value
// return 1 when popup was activated
u8 menu_buttons(void) {
    if (!cg.ch3_momentary && btn(BTN_CH3)) {
	key_beep();
	ch3_state = ~ch3_state;
    }
    return 0;
}

