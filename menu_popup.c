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






// XXX temporary, will be in model config
static struct {
    config_et_map_s  et_map[4];
//    config_key_map_s key_map[3];
//    config_key_map_s key_map_long[3];
} cml = {
    {
	{ 1, 0, 1, 0 },
	{ 2, 0, 1, 0 },
	{ 1, 0, 1, 0 },
	{ 3, 1, 1, 0 }
    },
};




// mapping of keys to trims
static const u16 et_buttons[][2] = {
    { BTN_TRIM_LEFT,  BTN_TRIM_RIGHT },
    { BTN_TRIM_FWD,   BTN_TRIM_BCK },
    { BTN_TRIM_CH3_L, BTN_TRIM_CH3_R },
    { BTN_DR_L,       BTN_DR_R },
};
#define ET_BUTTONS_SIZE  (sizeof(et_buttons) / sizeof(u16) / 2)
#define ETB_L(id)  et_buttons[id][0]
#define ETB_R(id)  et_buttons[id][1]




// functions assignable to trims
typedef struct {
    u8 idx;		// will be showed sorted by this
    u8 *name;		// showed identification
    u8 menu;		// which menu item(s) show
    u8 flags;		// bits: arrow_l, arrow_r, percent, ....., blink
    u8 channel;		// show this channel
    void *aval;		// address of variable
    s16 min, max, reset; // limits of variable
    u8 rot_fast_step;	// step for fast encoder rotate
    u8 *labels;		// labels for trims
} et_functions_s;
static const et_functions_s et_functions[] = {
    { 0, "OFF", 0, 0,  0, NULL, 0, 0, 0, 0, NULL },
    { 1, "TR1", LM_TRIM, 0, 1, &cm.trim[0], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, "LNR" },
    { 2, "TR2", LM_TRIM, 0, 2, &cm.trim[1], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, "FNB" },
    { 3, "DRS", LM_DR, 0b00000100, 1, &cm.dr_steering, 0, 100, 100,
      DUALRATE_FAST, NULL },
    { 4, "DRF", LM_DR, 0b00000110, 2, &cm.dr_forward, 0, 100, 100,
      DUALRATE_FAST, NULL },
    { 5, "DRF", LM_DR, 0b00000101, 2, &cm.dr_back, 0, 100, 100,
      DUALRATE_FAST, NULL },
};
#define ET_FUNCTIONS_SIZE  (sizeof(et_cunctions) / sizeof(et_cunctions_s))





// temporary show popup value (trim, subtrim, dualrate, ...)
// if another key pressed, return
static u8 menu_popup(u8 trim_id) {
    config_et_map_s *etm;
    et_functions_s *etf;
    u16 to_time;
    s16 val;
    u16 btn_l = ETB_L(trim_id);
    u16 btn_r = ETB_R(trim_id);
    u16 btn_lr = btn_l | btn_r;

    if (!btn(btn_lr))	 return 0;	// no key pressed
    etm = &cml.et_map[trim_id];		// XXX change to model config
    if (!etm->function)  return 0;	// do nothing for OFF

    etm->step = cg.trim_step;		// XXX delete when in model config
    etf = &et_functions[etm->function];

    // read value
    if (etf->min >= 0)  val = *(u8 *)etf->aval;	// *aval is unsigned
    else                val = *(s8 *)etf->aval;	// *aval is signed

    // show MENU and CHANNEL
    lcd_menu(etf->menu);
    if (etf->flags & 0x80) lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, (u8)((etf->flags >> 2) & 1));
    lcd_segment(LS_SYM_LEFT, (u8)((etf->flags >> 1) & 1));
    lcd_segment(LS_SYM_RIGHT, (u8)(etf->flags & 1));
    lcd_segment(LS_SYM_CHANNEL, LS_ON);
    lcd_7seg(etf->channel);

    while (1) {
	// check value left/right
	if (btnl_all(btn_lr)) {
	    // reset to given reset value
	    key_beep();
	    val = etf->reset;
	    *(s8 *)etf->aval = (s8)val;
	    btnr(btn_lr);
	}
	else if (btn(btn_lr)) {
	    if (!btns_all(btn_lr)) {
		// only when both are not pressed together
		// XXX add code for long key press
		key_beep();
		if ((u8)(btn(btn_l) ? 1 : 0) ^ etm->reverse) {
		    val -= etm->step;
		    if (val < etf->min)  val = etf->min;
		}
		else {
		    val += etm->step;
		    if (val > etf->max)  val = etf->max;
		}
		*(s8 *)etf->aval = (s8)val;
		btnr(btn_lr);
	    }
	    else btnr_nolong(btn_lr);  // keep long-presses for testing-both
	}
	else if (btn(BTN_ROT_ALL)) {
	    val = menu_change_val(val, etf->min, etf->max, etf->rot_fast_step,
	                          0);
	    *(s8 *)etf->aval = (s8)val;
	}
	btnr(BTN_ROT_ALL);

	// if another button was pressed, leave this screen
	if (buttons)  break;

	// show current value
	if (etf->labels)	lcd_char_num2_lbl((s8)val, etf->labels);
	else			lcd_char_num3(val);
	lcd_update();

	// sleep 5s, and if no button was pressed during, end this screen
	to_time = time_sec + POPUP_DELAY;
	while (time_sec < to_time && !buttons)
	    delay_menu((to_time - time_sec) * 200);

	if (!buttons)  break;  // timeouted without button press
    }

    btnr(btn_lr);  // reset also long values

    // set MENU off
    lcd_menu(0);

    // save model config
    config_model_save();
    return 1;
}





// check trims and dualrate keys, invoke popup menu
// return 1 when popup was activated
u8 menu_electronic_trims(void) {
    u8 i;

    // for each trim, call function
    for (i = 0; i < ET_BUTTONS_SIZE; i++)
	if (menu_popup(i))  return 1;

    return 0;
}





// check buttons CH3, BACK, END, invoke popup to show value
// return 1 when popup was activated
u8 menu_buttons(void) {
    // CH3 button
    if (cg.ch3_momentary)
	menu_channel3 = (s8)(btns(BTN_CH3) ? 100 : -100);
    else {
	if (btn(BTN_CH3)) {
	    key_beep();
	    menu_channel3 = (s8)(-menu_channel3);
	}
    }

    return 0;
}

