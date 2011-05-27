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






// XXX temporary, will be in model config, default button mappings
static struct {
    config_et_map_s   et_map[4];
    config_key_map_s  key_map[3];
    config_key_map2_s key_map_flags;
} cml = {
    {
	{ 1, 0, 1, 0 },
	{ 2, 0, 1, 0 },
	{ 1, 0, 1, 0 },
	{ 3, 0, 1, 1 }
    },
    {
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
    },
    { 0, 0 }
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
    u8 flags;		// bits below
    u8 channel;		// show this channel number
    void *aval;		// address of variable
    s16 min, max, reset; // limits of variable
    u8 rot_fast_step;	// step for fast encoder rotate
    u8 *labels;		// labels for trims
} et_functions_s;
#define EF_NONE		0
#define EF_RIGHT	0b00000001
#define EF_LEFT		0b00000010
#define EF_PERCENT	0b00000100
#define EF_BLINK	0b10000000

static const et_functions_s et_functions[] = {
    { 0, "OFF", 0, EF_NONE, 0, NULL, 0, 0, 0, 0, NULL },
    { 1, "TR1", LM_TRIM, EF_NONE, 1, &cm.trim[0], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, "LNR" },
    { 2, "TR2", LM_TRIM, EF_NONE, 2, &cm.trim[1], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, "FNB" },
    { 7, "DRS", LM_DR, EF_PERCENT, 1, &cm.dr_steering, 0, 100, 100,
      DUALRATE_FAST, NULL },
    { 8, "DRF", LM_DR, EF_PERCENT | EF_LEFT, 2, &cm.dr_forward, 0, 100, 100,
      DUALRATE_FAST, NULL },
    { 9, "DRB", LM_DR, EF_PERCENT | EF_RIGHT, 2, &cm.dr_back, 0, 100, 100,
      DUALRATE_FAST, NULL },
    { 10, "EXS", LM_EXP, EF_PERCENT, 1, &cm.expo_steering, -EXPO_MAX, EXPO_MAX,
      0, EXPO_FAST, NULL },
    { 11, "EXF", LM_EXP, EF_PERCENT | EF_LEFT, 2, &cm.expo_forward,
      -EXPO_MAX, EXPO_MAX, 0, EXPO_FAST, NULL },
    { 12, "EXB", LM_EXP, EF_PERCENT | EF_RIGHT, 2, &cm.expo_back,
      -EXPO_MAX, EXPO_MAX, 0, EXPO_FAST, NULL },
    { 13, "CH3", 0, EF_NONE, 3, &menu_channel3, -100, 100, 0, CHANNEL_FAST,
      NULL },
    { 17, "ST1", LM_TRIM, EF_BLINK, 1, &cm.subtrim[0], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL },
    { 18, "ST2", LM_TRIM, EF_BLINK, 2, &cm.subtrim[1], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL },
    { 19, "ST3", LM_TRIM, EF_BLINK, 3, &cm.subtrim[2], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL },
#if MAX_CHANNELS >= 4
    { 14, "CH4", 0, EF_NONE, 4, &menu_channel4, -100, 100, 0, CHANNEL_FAST,
      NULL },
    { 20, "ST4", LM_TRIM, EF_BLINK, 4, &cm.subtrim[3], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL },
#if MAX_CHANNELS >= 5
    { 15, "CH5", 0, EF_NONE, 5, &menu_channel5, -100, 100, 0, CHANNEL_FAST,
      NULL },
    { 21, "ST5", LM_TRIM, EF_BLINK, 5, &cm.subtrim[4], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL },
#if MAX_CHANNELS >= 6
    { 16, "CH6", 0, EF_NONE, 6, &menu_channel6, -100, 100, 0, CHANNEL_FAST,
      NULL },
    { 22, "ST6", LM_TRIM, EF_BLINK, 6, &cm.subtrim[5], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL },
#endif
#endif
#endif
};
#define ET_FUNCTIONS_SIZE  (sizeof(et_functions) / sizeof(et_functions_s))


// return name of given line
u8 *menu_et_function_name(u8 n) {
    return et_functions[n].name;
}

// return sort idx of given line
s8 menu_et_function_idx(u8 n) {
    if (n >= ET_FUNCTIONS_SIZE)  return -1;
    return et_functions[n].idx;
}



static const u8 steps_map[] = {
    1, 2, 5, 10, 20, 30, 40, 50, 67, 100, 200,
};
#define STEPS_MAP_SIZE  sizeof(steps_map)





// temporary show popup value (trim, subtrim, dualrate, ...)
// if another key pressed, return
#define AVAL(x)  *(s8 *)etf->aval = (s8)(x)
static u8 menu_popup_et(u8 trim_id) {
    u16 to_time;
    s16 val;
    u8  step;
    u16 buttons_state_last;
    u16 btn_l = ETB_L(trim_id);
    u16 btn_r = ETB_R(trim_id);
    u16 btn_lr = btn_l | btn_r;
    config_et_map_s *etm = &cml.et_map[trim_id];  // XXX change to model config
    et_functions_s *etf = &et_functions[etm->function];

    // do nothing when set to OFF
    if (!etm->function)  return 0;

    // if keys are momentary, show nothing, but set value
    if (etm->buttons == ETB_MOMENTARY) {
	if (btns(btn_l)) {
	    // left
	    AVAL(etm->reverse ? etf->max : etf->min);
	}
	else if (btns(btn_r)) {
	    // right
	    AVAL(etm->reverse ? etf->min : etf->max);
	}
	else {
	    // center
	    AVAL(etf->reset);
	}
	return 0;
    }

    // return when key was not pressed
    if (!btn(btn_lr))	 return 0;

    // remember buttons state
    buttons_state_last = buttons_state & ~btn_lr;

    // convert steps
    step = steps_map[etm->step];
    step = cg.trim_step;		// XXX delete when in model config

    // read value
    if (etf->min >= 0)  val = *(u8 *)etf->aval;	// *aval is unsigned
    else                val = *(s8 *)etf->aval;	// *aval is signed

    // show MENU and CHANNEL
    lcd_menu(etf->menu);
    if (etf->flags & EF_BLINK) lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, (u8)(etf->flags & EF_PERCENT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_LEFT, (u8)(etf->flags & EF_LEFT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_RIGHT, (u8)(etf->flags & EF_RIGHT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_CHANNEL, LS_ON);
    lcd_7seg(etf->channel);

    while (1) {
	// check value left/right
	if (btnl_all(btn_lr)) {
	    // reset to given reset value
	    key_beep();
	    val = etf->reset;
	    AVAL(val);
	    btnr(btn_lr);
	}
	else if (btn(btn_lr)) {
	    if (!btns_all(btn_lr)) {
		// only when both are not pressed together
		key_beep();
		if ((etm->buttons == ETB_LONG_RESET ||
		     etm->buttons == ETB_LONG_ENDVAL) && btnl(btn_lr)) {
		    // handle long key press
		    if (etm->buttons == ETB_LONG_RESET)
			val = etf->reset;
		    else {
			// set side value
			if ((u8)(btn(btn_l) ? 1 : 0) ^ etm->reverse)
			    val = etf->min;
			else
			    val = etf->max;
		    }
		}
		else {
		    // handle short key press
		    if ((u8)(btn(btn_l) ? 1 : 0) ^ etm->reverse) {
			val -= step;
			if (val < etf->min)  val = etf->min;
		    }
		    else {
			val += step;
			if (val > etf->max)  val = etf->max;
		    }
		}
		AVAL(val);
		btnr(btn_lr);
	    }
	    else btnr_nolong(btn_lr);  // keep long-presses for testing-both
	}
	else if (btn(BTN_ROT_ALL)) {
	    val = menu_change_val(val, etf->min, etf->max, etf->rot_fast_step,
	                          0);
	    AVAL(val);
	}
	btnr(BTN_ROT_ALL);

	// if another button was pressed, leave this screen
	if (buttons)  break;
	if ((buttons_state & ~btn_lr) != buttons_state_last)  break;

	// show current value
	if (etf->labels)	lcd_char_num2_lbl((s8)val, etf->labels);
	else			lcd_char_num3(val);
	lcd_update();

	// sleep 5s, and if no button was changed during, end this screen
	to_time = time_sec + POPUP_DELAY;
	while (time_sec < to_time && !buttons &&
	       ((buttons_state & ~btn_lr) == buttons_state_last))
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
	if (menu_popup_et(i))  return 1;

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

