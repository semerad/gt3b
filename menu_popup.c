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





// last state of buttons
static @near u8 menu_buttons_state[NUM_KEYS + 2 * NUM_TRIMS];
#define MBS_INITIALIZE	0xff
// for momentary keys
#define MBS_RELEASED	0x01
#define MBS_PRESSED	0x02
#define MBS_MIDDLE	0x03
// for momentary trims (+MBS_RELEASED)
#define	MBS_LEFT	0x04
#define	MBS_RIGHT	0x05
// for switched keys
#define MBS_ON		0x40
#define MBS_ON_LONG	0x80

// set state of buttons to do initialize
void menu_buttons_initialize(void) {
    memset(menu_buttons_state, MBS_INITIALIZE, sizeof(menu_buttons_state));
    menu_check_keys = 1;
}



// previous values for using buttons that way to return to previous value
//   instead of centre/left
static @near s16 menu_buttons_previous_values[NUM_KEYS + 2 * NUM_TRIMS];


#define BEEP_RESET  if (cg.reset_beep)  buzzer_on(20, 0, 1)






// ********************* TRIMS ****************************


// mapping of keys to trims
const u16 et_buttons[][2] = {
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
    // set function - used instead of directly value assign
    void (*set_func)(u8 *name, s16 *aval, u8 rotate);
    // show function - used to show value instead to show raw value
    void (*show_func)(u8 *name, s16 val);
    // function for special long-press handling
    void (*long_func)(u8 *name, s16 *aval, u16 btn_l, u16 btn_r);
} et_functions_s;
#define EF_NONE		0
#define EF_RIGHT	0b00000001
#define EF_LEFT		0b00000010
#define EF_PERCENT	0b00000100
#define EF_LIST		0b00001000
#define EF_NO2CHANNELS	0b00010000
#define EF_NOCONFIG	0b00100000
#define EF_NOCHANNEL	0b01000000
#define EF_BLINK	0b10000000


// trims with labels
static void show_trim1(u8 *name, s16 val) {
    lcd_char_num2_lbl((s8)val, "LNR");
}
static void show_trim2(u8 *name, s16 val) {
    lcd_char_num2_lbl((s8)val, "FNB");
}

// multi-position show and set value
static void show_MP(u8 *name, s16 val) {
    u8 mp_id = (u8)(name[2] - '1');
    s8 *multi_position;
    u8 channel_MP;
    u8 num_MP = config_get_MP(mp_id, &channel_MP, &multi_position);

    // show also selected channel/DIG
    if (channel_MP == MP_DIG) {
	lcd_7seg(L7_D);
	lcd_menu(LM_EPO);
	lcd_set_blink(LMENU, LB_SPC);
	lcd_segment(LS_SYM_CHANNEL, LS_OFF);
	lcd_segment(LS_SYM_PERCENT, LS_ON);
    }
    else {
	lcd_7seg(channel_MP);
	lcd_segment(LS_SYM_CHANNEL, LS_ON);
    }
    lcd_char_num3(multi_position[menu_MP_index[mp_id]]);
}
static void set_MP(u8 *name, s16 *aval, u8 rotate) {
    u8 mp_id = (u8)(name[2] - '1');
    s8 *multi_position;
    u8 channel_MP;
    u8 num_MP = config_get_MP(mp_id, &channel_MP, &multi_position);

    // if END value selected, return it back to previous
    if (multi_position[*aval] == MULTI_POSITION_END) {
	if (rotate) {
	    if (!menu_MP_index[mp_id]) {
		// rotated left through 0, find right value
		while (multi_position[*aval] == MULTI_POSITION_END)
		    (*aval)--;
	    }
	    else  *aval = 0;  // rotated right, return to 0
	}
	else  (*aval)--;  // no rotate, return back to previous index
    }
    // set value of channel
    if (channel_MP) {
	if (channel_MP == MP_DIG)  menu_DIG_mix = multi_position[*aval];
	else  menu_channel3_8[channel_MP - 3] = multi_position[*aval];
    }
}


static const et_functions_s et_functions[] = {
    { 0, "OFF", 0, EF_NONE, 0, NULL, 0, 0, 0, 0, NULL, NULL },
    { 1, "TR1", LM_TRIM, EF_NONE, 1, &cm.trim[0], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, NULL, show_trim1, NULL },
    { 2, "TR2", LM_TRIM, EF_NONE, 2, &cm.trim[1], -TRIM_MAX, TRIM_MAX, 0,
      TRIM_FAST, NULL, show_trim2, NULL },
    { 7, "DRS", LM_DR, EF_PERCENT, 1, &cm.dr_steering, 0, 100, 100,
      DUALRATE_FAST, NULL, NULL, NULL },
    { 8, "DRF", LM_DR, EF_PERCENT | EF_LEFT, 2, &cm.dr_forward, 0, 100, 100,
      DUALRATE_FAST, NULL, NULL, NULL },
    { 9, "DRB", LM_DR, EF_PERCENT | EF_RIGHT, 2, &cm.dr_back, 0, 100, 100,
      DUALRATE_FAST, NULL, NULL, NULL },
    { 10, "EXS", LM_EXP, EF_PERCENT, 1, &cm.expo_steering, -EXPO_MAX, EXPO_MAX,
      0, EXPO_FAST, NULL, NULL, NULL },
    { 11, "EXF", LM_EXP, EF_PERCENT | EF_LEFT, 2, &cm.expo_forward,
      -EXPO_MAX, EXPO_MAX, 0, EXPO_FAST, NULL, NULL, NULL },
    { 12, "EXB", LM_EXP, EF_PERCENT | EF_RIGHT, 2, &cm.expo_back,
      -EXPO_MAX, EXPO_MAX, 0, EXPO_FAST, NULL, NULL, NULL },
    { 13, "CH3", 0, EF_NOCONFIG, 3, &menu_channel3, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL, NULL },
    { 19, "ST1", LM_TRIM, EF_BLINK, 1, &cm.subtrim[0], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 20, "ST2", LM_TRIM, EF_BLINK, 2, &cm.subtrim[1], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 21, "ST3", LM_TRIM, EF_BLINK, 3, &cm.subtrim[2], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 27, "SST", LM_DR, EF_BLINK | EF_LEFT | EF_PERCENT, 1, &cm.stspd_turn,
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
    { 28, "SSR", LM_DR, EF_BLINK | EF_RIGHT | EF_PERCENT, 1, &cm.stspd_return,
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
    { 29, "CS2", LM_DR, EF_BLINK | EF_PERCENT, 2, &cm.thspd,
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
    { 30, "CS3", LM_DR, EF_BLINK | EF_PERCENT, 3, &cm.speed[2],
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
#if MAX_CHANNELS >= 4
    { 14, "CH4", 0, EF_NOCONFIG, 4, &menu_channel4, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL, NULL },
    { 22, "ST4", LM_TRIM, EF_BLINK, 4, &cm.subtrim[3], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 31, "CS4", LM_DR, EF_BLINK | EF_PERCENT, 4, &cm.speed[3],
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
#if MAX_CHANNELS >= 5
    { 15, "CH5", 0, EF_NOCONFIG, 5, &menu_channel5, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL, NULL },
    { 23, "ST5", LM_TRIM, EF_BLINK, 5, &cm.subtrim[4], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 32, "CS5", LM_DR, EF_BLINK | EF_PERCENT, 5, &cm.speed[4],
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
#if MAX_CHANNELS >= 6
    { 16, "CH6", 0, EF_NOCONFIG, 6, &menu_channel6, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL, NULL },
    { 24, "ST6", LM_TRIM, EF_BLINK, 6, &cm.subtrim[5], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 33, "CS6", LM_DR, EF_BLINK | EF_PERCENT, 6, &cm.speed[5],
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
#if MAX_CHANNELS >= 7
    { 17, "CH7", 0, EF_NOCONFIG, 7, &menu_channel7, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL, NULL },
    { 25, "ST7", LM_TRIM, EF_BLINK, 7, &cm.subtrim[6], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 34, "CS7", LM_DR, EF_BLINK | EF_PERCENT, 7, &cm.speed[6],
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
#if MAX_CHANNELS >= 8
    { 18, "CH8", 0, EF_NOCONFIG, 8, &menu_channel8, -100, 100, 0, CHANNEL_FAST,
      NULL, NULL, NULL },
    { 26, "ST8", LM_TRIM, EF_BLINK, 8, &cm.subtrim[7], -SUBTRIM_MAX,
      SUBTRIM_MAX, 0, SUBTRIM_FAST, NULL, NULL, NULL },
    { 35, "CS8", LM_DR, EF_BLINK | EF_PERCENT, 8, &cm.speed[7],
      1, 100, 100, SPEED_FAST, NULL, NULL, NULL },
#endif
#endif
#endif
#endif
#endif
    { 36, "4WS", LM_EPO, EF_BLINK | EF_PERCENT | EF_NOCHANNEL | EF_NOCONFIG | EF_NO2CHANNELS,
      4, &menu_4WS_mix, -100, 100, 0, MIX_FAST, NULL, NULL, NULL },
    { 37, "DIG", LM_EPO, EF_BLINK | EF_PERCENT | EF_NOCHANNEL | EF_NOCONFIG | EF_NO2CHANNELS,
      L7_D, &menu_DIG_mix, -100, 100, 0, MIX_FAST, NULL, NULL, NULL },
    { 38, "MP1", 0, EF_LIST | EF_NOCONFIG, 3, &menu_MP_index[0],
      0, NUM_MULTI_POSITION0 - 1, 0, 1, set_MP, show_MP, NULL },
    { 39, "MP2", 0, EF_LIST | EF_NOCONFIG, 4, &menu_MP_index[1],
      0, NUM_MULTI_POSITION1 - 1, 0, 1, set_MP, show_MP, NULL },
    { 40, "MP3", 0, EF_LIST | EF_NOCONFIG, 5, &menu_MP_index[2],
      0, NUM_MULTI_POSITION2 - 1, 0, 1, set_MP, show_MP, NULL },
    { 41, "MP4", 0, EF_LIST | EF_NOCONFIG, 6, &menu_MP_index[3],
      0, NUM_MULTI_POSITION3 - 1, 0, 1, set_MP, show_MP, NULL },
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

// return 1 if given function has special long-press handling
u8 menu_et_function_long_special(u8 n) {
    return (u8)(et_functions[n].long_func ? 1 : 0);
}

// return 1 if given function switches list of items
u8 menu_et_function_is_list(u8 n) {
    return (u8)(et_functions[n].flags & EF_LIST);
}

// return 1 if given function is allowed
u8 menu_et_function_is_allowed(u8 n) {
    et_functions_s *etf = &et_functions[n];
    if (n == 0)			    return 1;	// OFF is always allowed
    // check number of channels
    if ((etf->flags & EF_NO2CHANNELS) && channels == 2)
				    return 0;	// not for 2 channels
    if (etf->flags & EF_NOCHANNEL)  return 1;	// channel not used
    if (etf->channel <= channels)   return 1;	// channel is OK
    return 0;					// channel too big
}

// set function value from linear channel value
// lin_val in range -5000..5000
#define AVAL(x) \
    if (etf->set_func)  etf->set_func(etf->name, &x, SF_ROTATE); \
    *(s8 *)etf->aval = (s8)(x)
#define SF_ROTATE 0
void menu_et_function_set_from_linear(u8 n, s16 lin_val) {
    et_functions_s *etf = &et_functions[n];
    s16 val;
    if (n == 0 || n >= ET_FUNCTIONS_SIZE)  return;  // OFF or bad
    // map lin_val between min and max
    val = (s16)((s32)(lin_val + PPM(500)) * (etf->max - etf->min + 1)
		/ PPM(1000)) + etf->min;
    if (val > etf->max)  val = etf->max;	// lin_val was full right
    AVAL(val);
}
#undef SF_ROTATE

// find function by name
static et_functions_s *menu_et_function_find_name(u8 *name) {
    u8 i, *n;
    for (i = 0; i < ET_FUNCTIONS_SIZE; i++) {
	n = et_functions[i].name;
	if (n[0] == name[0] && n[1] == name[1] && n[2] == name[2])
	    return &et_functions[i];
    }
    return NULL;
}

// show functions ID
static void menu_et_function_show_id(et_functions_s *etf) {
    lcd_menu(etf->menu);
    if (etf->flags & EF_BLINK) lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_DOT, LS_OFF);
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    lcd_segment(LS_SYM_PERCENT, (u8)(etf->flags & EF_PERCENT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_LEFT, (u8)(etf->flags & EF_LEFT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_RIGHT, (u8)(etf->flags & EF_RIGHT ? LS_ON : LS_OFF));
    lcd_segment(LS_SYM_CHANNEL, (u8)(etf->flags & EF_NOCHANNEL ? LS_OFF : LS_ON));
    lcd_7seg(etf->channel);
}



// mapping of trim steps
const u8 steps_map[STEPS_MAP_SIZE] = {
    1, 2, 5, 10, 20, 30, 40, 50, 67, 100, 200,
};





// temporary show popup value (trim, subtrim, dualrate, ...)
// if another key pressed, return
#define RVAL(x) \
    if (etf->min >= 0)  x = *(u8 *)etf->aval; \
    else		x = *(s8 *)etf->aval;
static u8 menu_popup_et(u8 trim_id) {
    u16 delay_time;
    s16 val;
    u8  step;
    u16 buttons_state_last;
    u16 btn_l = ETB_L(trim_id);
    u16 btn_r = ETB_R(trim_id);
    u16 btn_lr = btn_l | btn_r;
    config_et_map_s *etm = &ck.et_map[trim_id];
#define SF_ROTATE  etm->rotate
    et_functions_s *etf = &et_functions[etm->function];
    u8 *mbs = &menu_buttons_state[NUM_KEYS + 2 * trim_id];

    // read value
    RVAL(val);

    // remember buttons state
    buttons_state_last = buttons_state & ~btn_lr;

    // handle momentary keys
    //   when something changed, show value for 5s while checking buttons
    //   during initialize show nothing
    if (etm->buttons == ETB_MOMENTARY) {
	s16 *pv = &menu_buttons_previous_values[NUM_KEYS + 2 * trim_id];
	u8 value_showed = 0;
	u8 state;

	while (1) {
	    // set actual state of btn_lr to buttons_state_last
	    buttons_state_last &= ~btn_lr;
	    buttons_state_last |= buttons_state & btn_lr;

	    // check buttons
	    if (btns(btn_l)) {
		// left
		if (*mbs == MBS_LEFT)  break;	// already was left
		state = MBS_LEFT;
		*pv = val;			// save previous value
		val = etm->reverse ? etf->max : etf->min;
	    }
	    else if (btns(btn_r)) {
		// right
		if (*mbs == MBS_RIGHT)  break;	// already was right
		state = MBS_RIGHT;
		*pv = val;			// save previous value
		val = etm->reverse ? etf->min : etf->max;
	    }
	    else {
		// center
		if (*mbs == MBS_RELEASED)  break;  // already was center
		state = MBS_RELEASED;
		if (etm->previous_val) {
		    if (*mbs != MBS_INITIALIZE)  val = *pv;
		}
		else  val = etf->reset;
	    }
	    AVAL(val);
	    if (*mbs == MBS_INITIALIZE) {
		// show nothing when doing initialize
		*mbs = state;
		break;
	    }
	    *mbs = state;
	    btnr(btn_lr);
	    key_beep();

	    // if another button was pressed, leave this screen
	    if (buttons)  break;
	    if (buttons_state != buttons_state_last) break;

	    // show function id first time
	    if (!value_showed) {
		value_showed = 1;
		menu_et_function_show_id(etf);
	    }

	    // show current value
	    if (etf->show_func)  etf->show_func(etf->name, val);
	    else		 lcd_char_num3(val);
	    lcd_update();

	    // sleep 5s, and if no button was changed during, end this screen
	    delay_time = POPUP_DELAY * 200;
	    while (delay_time && !(buttons & ~btn_lr) &&
		   (buttons_state == buttons_state_last))
		delay_time = delay_menu(delay_time);

	    if ((buttons_state & btn_lr) == (buttons_state_last & btn_lr))
		break;
	}

	btnr(btn_lr);
	if (value_showed)  lcd_menu(0);		// set MENU off
	return value_showed;
    }

    // if button is not initialized, do it
    if (*mbs == MBS_INITIALIZE) {
	// set value to default only for non-config values (mixes, channels...)
	if (etf->flags & EF_NOCONFIG) {
	    val = etf->reset;
	    AVAL(val);
	}
	*mbs = MBS_RELEASED;
    }

    // return when key was not pressed
    if (!btn(btn_lr))	 return 0;

    // convert steps
    step = steps_map[etm->step];

    // show MENU and CHANNEL
    menu_et_function_show_id(etf);

    while (1) {
	u8  val_set_to_reset = 0;

	// check value left/right
	if (btnl_all(btn_lr)) {
	    // both long keys together
	    key_beep();
	    if (etf->long_func && etm->buttons == ETB_SPECIAL)
		// special handling
		etf->long_func(etf->name, &val, btn_l, btn_r);
	    else {
		// reset to given reset value
		val = etf->reset;
	    }
	    AVAL(val);
	    if (val == etf->reset)  val_set_to_reset = 1;
	    btnr(btn_lr);
	}
	else if (btn(btn_lr)) {
	    if (!btns_all(btn_lr)) {
		// only one key is currently pressed
		key_beep();
		if (etf->long_func && etm->buttons == ETB_SPECIAL &&
		    btnl(btn_lr))
		    // special handling
		    etf->long_func(etf->name, &val, btn_l, btn_r);
		else if ((etm->buttons == ETB_LONG_RESET ||
			  etm->buttons == ETB_LONG_ENDVAL) && btnl(btn_lr)) {
		    // handle long key press
		    if (etm->buttons == ETB_LONG_RESET) {
			val = etf->reset;
		    }
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
			if (val < etf->min) {
			    if (etm->rotate)  val = etf->max;
			    else	      val = etf->min;
			}
			if (etm->opposite_reset &&
			    val > etf->reset)  val = etf->reset;
		    }
		    else {
			val += step;
			if (val > etf->max) {
			    if (etm->rotate)  val = etf->min;
			    else	      val = etf->max;
			}
			if (etm->opposite_reset &&
			    val < etf->reset)  val = etf->reset;
		    }
		}
		AVAL(val);
		if (val == etf->reset)  val_set_to_reset = 1;
		btnr(btn_lr);
	    }
	    else btnr_nolong(btn_lr);  // keep long-presses for testing-both
	}
	else if (btn(BTN_ROT_ALL)) {
	    s16 val2 = val;
	    val = menu_change_val(val, etf->min, etf->max, etf->rot_fast_step,
	                          etm->rotate);
	    // if encoder skipped reset value, set it to reset value
	    if (val2 < etf->reset && val > etf->reset ||
	        val2 > etf->reset && val < etf->reset)  val = etf->reset;
	    AVAL(val);
	    if (val == etf->reset)  val_set_to_reset = 1;
	}
	btnr(BTN_ROT_ALL);

	// longer beep at value reset value
	if (val_set_to_reset)  BEEP_RESET;

	// if another button was pressed, leave this screen
	if (buttons)  break;
	if ((buttons_state & ~btn_lr) != buttons_state_last)  break;

	// show current value
	if (etf->show_func)  etf->show_func(etf->name, val);
	else		 lcd_char_num3(val);
	lcd_update();

	// if reset value was reached, ignore rotate/btn_lr for some time
	delay_time = POPUP_DELAY * 200;
	if (val_set_to_reset) {
	    u8 delay = RESET_VALUE_DELAY;
	    while (delay && !(buttons & ~(btn_lr | BTN_ROT_ALL)) &&
		   ((buttons_state & ~btn_lr) == buttons_state_last))
		delay = (u8)delay_menu(delay);
	    btnr(BTN_ROT_ALL | btn_lr);
	    delay_time -= RESET_VALUE_DELAY;
	}

	// sleep 5s, and if no button was changed during, end this screen
	while (delay_time && !buttons &&
	       ((buttons_state & ~btn_lr) == buttons_state_last))
	    delay_time = delay_menu(delay_time);

	if (!buttons)  break;  // timeouted without button press
    }

    btnr(btn_lr);  // reset also long values

    // set MENU off
    lcd_menu(0);

    // save model config
    config_model_save();
    return 1;
}
#undef SF_ROTATE





// check trims and dualrate keys, invoke popup menu
// return 1 when popup was activated
u8 menu_electronic_trims(void) {
    u8 i;

    // for each trim, call function
    for (i = 0; i < ET_BUTTONS_SIZE; i++) {
	if (!ck.et_map[i].is_trim)  continue;  // trim is off
	if (menu_popup_et(i))  return 1;  // check other keys in main loop
    }

    return 0;
}






// ************************* KEYS *************************


// mapping of keys to no-trims
static const u16 key_buttons[] = {
    BTN_CH3,
    BTN_BACK,
    BTN_END,
    BTN_TRIM_LEFT,  BTN_TRIM_RIGHT,
    BTN_TRIM_FWD,   BTN_TRIM_BCK,
    BTN_TRIM_CH3_L, BTN_TRIM_CH3_R,
    BTN_DR_L,       BTN_DR_R,
};
#define KEY_BUTTONS_SIZE  (sizeof(key_buttons) / sizeof(u16))




// functions assignable to keys
typedef struct {
    u8 idx;		// will be showed sorted by this
    u8 *name;		// showed identification
    u8 flags;		// bits below
    void (*func)(u8 *id, u8 *param, u8 flags, s16 *prev_val);	// function to process key, flags below
    void *param;	// param given to function
    u8 channel;		// which channel it operates, only to eliminate excess channels
} key_functions_s;
// flags bits
#define KF_NONE		0
#define KF_2STATE	0b00000001
#define KF_NOSHOW	0b00000010
// func flags bits
#define FF_NONE		0
#define FF_ON		0b00000001
#define FF_REVERSE	0b00000010
#define FF_MID		0b00000100
#define FF_HAS_MID	0b00001000
#define FF_PREVIOUS	0b00010000
#define FF_SHOW		0b10000000






#define SF_ROTATE 1

// set channel value to one endpoint (also to middle with 3-pos CH3)
static void kf_set_switch(u8 *id, u8 *param, u8 flags, s16 *prev_val) {
    u8 *name = param ? param : id;
    et_functions_s *etf = menu_et_function_find_name(name);
    s16 val;

    if (!etf)  return;
    RVAL(val);

    if (flags & FF_MID) {
	// middle
	if (flags & FF_PREVIOUS)  val = *prev_val;
	else			  val = etf->reset;
    }
    else if (flags & FF_ON) {
	// ON
	*prev_val = val;  // always save previous val
	val = (s8)(flags & FF_REVERSE ? etf->min : etf->max);
    }
    else {
	// OFF
	if ((flags & FF_PREVIOUS) && !(flags & FF_HAS_MID))
	    // use previous only when there is no middle state
	    val = *prev_val;
	else {
	    // save previous only when there is middle state
	    if (flags & FF_HAS_MID)  *prev_val = val;
	    val = (s8)(flags & FF_REVERSE ? etf->max : etf->min);
	}
    }
    AVAL(val);

    if (flags & FF_SHOW) {
	menu_et_function_show_id(etf);
	lcd_char_num3(val);
    }
}

// reset value to reset_value
static void kf_reset(u8 *id, u8 *param, u8 flags, s16 *prev_val) {
    u8 *name = param ? param : id;
    et_functions_s *etf = menu_et_function_find_name(name);
    s16 val;

    if (!etf)  return;
    val = etf->reset;
    AVAL(val);

    if (flags & FF_SHOW) {
	BEEP_RESET;
	menu_et_function_show_id(etf);
	lcd_char_num3(val);
    }
}

// change 4WS crab/no-crab
static void kf_4ws(u8 *id, u8 *param, u8 flags, s16 *prev_val) {
    if (flags & FF_ON)
	menu_4WS_crab = (u8)(flags & FF_REVERSE ? 0 : 1);
    else
	menu_4WS_crab = (u8)(flags & FF_REVERSE ? 1 : 0);

    if (flags & FF_SHOW) {
	lcd_7seg(4);
	lcd_chars(menu_4WS_crab ? "CRB" : "NOC");
    }
}

// switch multi-position to next index
static void kf_multi_position(u8 *id, u8 *param, u8 flags, s16 *prev_val) {
    u8 mp_id = (u8)(u16)param;
    s8 *multi_position;
    u8 channel_MP;
    u8 num_MP = config_get_MP(mp_id, &channel_MP, &multi_position);
    u8 *mp_index = &menu_MP_index[mp_id];

    if (++*mp_index >= num_MP ||
        multi_position[*mp_index] == MULTI_POSITION_END)
	    *mp_index = 0;
    if (channel_MP) {
	if (channel_MP == MP_DIG)
	    menu_DIG_mix = multi_position[*mp_index];
	else
	    menu_channel3_8[channel_MP - 3] = multi_position[*mp_index];
    }

    if (flags & FF_SHOW) {
	if (!*mp_index)  BEEP_RESET;
	show_MP(id, 0);
    }
}
static void kf_multi_position_reset(u8 *id, u8 *param, u8 flags, s16 *pv) {
    u8 mp_id = (u8)(u16)param;
    s8 *multi_position;
    u8 channel_MP;
    u8 num_MP = config_get_MP(mp_id, &channel_MP, &multi_position);

    menu_MP_index[mp_id] = 0;
    if (channel_MP) {
	if (channel_MP == MP_DIG)
	    menu_DIG_mix = multi_position[0];
	else
	    menu_channel3_8[channel_MP - 3] = multi_position[0];
    }

    if (flags & FF_SHOW) {
	BEEP_RESET;
	show_MP(id, 0);
    }
}

// shut up battery low beeper
static void kf_battery_low_shutup(u8 *id, u8 *param, u8 flags, s16 *pv) {
    battery_low_shutup = 1;
    buzzer_off();
}

// change menu_brake bit
static void kf_brake(u8 *id, u8 *param, u8 flags, s16 *prev_val) {
    if (flags & FF_ON)
	menu_brake = (u8)(flags & FF_REVERSE ? 0 : 1);
    else
	menu_brake = (u8)(flags & FF_REVERSE ? 1 : 0);

    if (flags & FF_SHOW) {
	lcd_7seg(L7_B);
	lcd_chars(menu_brake ? "BRK" : "OFF");
    }
}




// table of key functions
static const key_functions_s key_functions[] = {
    { 0, "OFF", KF_NONE, NULL, NULL, 0 },
    { 29, "BLS", KF_NOSHOW, kf_battery_low_shutup, NULL, 0 },  // default END-long
    { 1, "CH3", KF_2STATE, kf_set_switch, NULL, 3 },
    { 7, "C3R", KF_NONE, kf_reset, "CH3", 3 },
#if MAX_CHANNELS >= 4
    { 2, "CH4", KF_2STATE, kf_set_switch, NULL, 4 },
    { 8, "C4R", KF_NONE, kf_reset, "CH4", 4 },
#if MAX_CHANNELS >= 5
    { 3, "CH5", KF_2STATE, kf_set_switch, NULL, 5 },
    { 9, "C5R", KF_NONE, kf_reset, "CH5", 5 },
#if MAX_CHANNELS >= 6
    { 4, "CH6", KF_2STATE, kf_set_switch, NULL, 6 },
    { 10, "C6R", KF_NONE, kf_reset, "CH6", 6 },
#if MAX_CHANNELS >= 7
    { 5, "CH7", KF_2STATE, kf_set_switch, NULL, 7 },
    { 11, "C7R", KF_NONE, kf_reset, "CH7", 7 },
#if MAX_CHANNELS >= 8
    { 6, "CH8", KF_2STATE, kf_set_switch, NULL, 8 },
    { 12, "C8R", KF_NONE, kf_reset, "CH8", 8 },
#endif
#endif
#endif
#endif
#endif
    { 13, "4WS", KF_2STATE, kf_4ws, NULL, 3 },
    { 14, "DIG", KF_2STATE, kf_set_switch, NULL, 3 },
    { 15, "DGR", KF_NONE, kf_reset, "DIG", 3 },
    { 16, "MP1", KF_NONE, kf_multi_position, (u8 *)0, 3 },
    { 17, "MR1", KF_NONE, kf_multi_position_reset, (u8 *)0, 3 },
    { 18, "MP2", KF_NONE, kf_multi_position, (u8 *)1, 4 },
    { 19, "MR2", KF_NONE, kf_multi_position_reset, (u8 *)1, 4 },
    { 20, "MP3", KF_NONE, kf_multi_position, (u8 *)2, 5 },
    { 21, "MR3", KF_NONE, kf_multi_position_reset, (u8 *)2, 5 },
    { 22, "MP4", KF_NONE, kf_multi_position, (u8 *)3, 6 },
    { 23, "MR4", KF_NONE, kf_multi_position_reset, (u8 *)3, 6 },
    { 24, "T1S", KF_NOSHOW, kf_menu_timer_start, (u8 *)0, 0 },
    { 25, "T1R", KF_NOSHOW, kf_menu_timer_reset, (u8 *)0, 0 },
    { 26, "T2S", KF_NOSHOW, kf_menu_timer_start, (u8 *)1, 0 },
    { 27, "T2R", KF_NOSHOW, kf_menu_timer_reset, (u8 *)1, 0 },
    { 28, "BRK", KF_2STATE, kf_brake, NULL, 0 },
    // beware of BLS with last id
};
#define KEY_FUNCTIONS_SIZE  (sizeof(key_functions) / sizeof(key_functions_s))




// return name of given line
u8 *menu_key_function_name(u8 n) {
    return key_functions[n].name;
}

// return sort idx of given line
s8 menu_key_function_idx(u8 n) {
    if (n >= KEY_FUNCTIONS_SIZE)  return -1;
    return key_functions[n].idx;
}

// return if it is 2-state function
u8 menu_key_function_2state(u8 n) {
    return (u8)(key_functions[n].flags & KF_2STATE);
}

// return 1 if given function is allowed
u8 menu_key_function_is_allowed(u8 n) {
    key_functions_s *kf = &key_functions[n];
    if (n == 0)			   return 1;	// OFF is always allowed
    // check number of channels
    if (kf->channel <= channels)   return 1;	// channel is OK
    return 0;					// channel too big
}

// delete all symbols and clean 7seg
static void menu_key_empty_id(void) {
    menu_clear_symbols();
    lcd_set(L7SEG, LB_EMPTY);
}




// change val, temporary show new value (not always)
// end when another key pressed
static u8 menu_popup_key(u8 key_id) {
    u16 delay_time;
    u16 buttons_state_last;
    u16 btnx;
    config_key_map_s *km = &ck.key_map[key_id];
    key_functions_s *kf;
    key_functions_s *kfl;
    u8 flags;
    u8 is_long = 0;
    u8 *mbs = &menu_buttons_state[key_id];
    s16 *pv = &menu_buttons_previous_values[key_id];

    // do nothing when both short and long set to OFF
    if (!km->function && !km->function_long)  return 0;

    // prepare more variables
    kf = &key_functions[km->function];
    btnx = key_buttons[key_id];

    // remember buttons state
    buttons_state_last = buttons_state & ~btnx;


    // check momentary setting
    //   when something changed, show value for 5s while checking buttons
    //   during initialize show nothing
    if (km->function && (kf->flags & KF_2STATE) && km->momentary) {
	static @near u8 ch3_has_middle; // set to 1 if ch3 has middle state
	u8 state;			// new button state
	u8 value_showed = 0;
	u16 btnx_orig = btnx;

	// for CH3 button add MIDDLE state also
	if (key_id == 0)  btnx |= BTN_CH3_MID;

	while (1) {
	    // set actual state of btnx to buttons_state_last
	    buttons_state_last &= ~btnx;
	    buttons_state_last |= buttons_state & btnx;

	    // check button
	    flags = FF_NONE;
	    state = MBS_RELEASED;
	    if (key_id == 0 && btns(BTN_CH3_MID)) {
		// special check for CH3 button middle
		flags |= FF_MID;
		state = MBS_MIDDLE;
		ch3_has_middle = 1;
	    }
	    else if (btns(btnx_orig)) {
		flags |= FF_ON;
		state = MBS_PRESSED;
	    }
	    if (km->reverse)	    flags |= FF_REVERSE;
	    if (key_id == 0 && ch3_has_middle)
				    flags |= FF_HAS_MID;
	    if (km->previous_val)   flags |= FF_PREVIOUS;

	    // end if button state didn't changed
	    if (state == *mbs)  break;

	    // end when initialize
	    if (*mbs == MBS_INITIALIZE) {
		// call func when not previous_val
		if (!km->previous_val || state == MBS_PRESSED)
		    kf->func(kf->name, kf->param, flags, pv);
		*mbs = state;
		break;
	    }
	    *mbs = state;
	    btnr(btnx);
	    key_beep();

	    // if value showing disabled, exit
	    if (kf->flags & KF_NOSHOW)  break;

	    // if another button was pressed, leave this screen
	    if (buttons)  break;
	    if (buttons_state != buttons_state_last) break;

	    // show function id first time
	    if (!value_showed) {
		value_showed = 1;
		menu_key_empty_id();
	    }

	    // call function to set value and show it
	    flags |= FF_SHOW;
	    kf->func(kf->name, kf->param, flags, pv);
	    lcd_update();

	    // sleep 5s, and if no button was changed during, end this screen
	    delay_time = POPUP_DELAY * 200;
	    while (delay_time && !(buttons & ~btnx) &&
		   (buttons_state == buttons_state_last))
		delay_time = delay_menu(delay_time);

	    if ((buttons_state & btnx) == (buttons_state_last & btnx))
		break;
	}

	btnr(btnx);
	if (value_showed)  lcd_menu(0);		// set MENU off
	return value_showed;
    }


    // non-momentary key
    kfl = &key_functions[km->function_long];

    // if button is not initialized, do it
    if (*mbs == MBS_INITIALIZE) {
	if (km->function && (kf->flags & KF_2STATE) && !km->previous_val)
	    kf->func(kf->name, kf->param, (u8)(km->reverse ? FF_REVERSE : 0),
	             pv);
	if (km->function_long && (kfl->flags & KF_2STATE)
	    && !km->previous_val_long)
	    kfl->func(kfl->name, kfl->param,
		      (u8)(km->reverse_long ? FF_REVERSE : 0), pv);
	*mbs = 0;  // both are OFF
    }

    // return when key was not pressed
    if (!btn(btnx))	 return 0;

    // clear some lcd segments
    menu_key_empty_id();

    while (1) {
	if (km->function_long && btnl(btnx)) {
	    // long key press
	    key_beep();
	    flags = 0;
	    if (!(kfl->flags & KF_NOSHOW))  flags = FF_SHOW;
	    if (kfl->flags & KF_2STATE) {	// ON/OFF is only for 2-state
		if (*mbs & MBS_ON_LONG) {
		    // is ON, switch to OFF
		    *mbs &= (u8)~MBS_ON_LONG;
		}
		else {
		    // is OFF, switch to ON
		    *mbs |= MBS_ON_LONG;
		    flags |= FF_ON;
		}
		if (km->reverse_long)       flags |= FF_REVERSE;
		if (km->previous_val_long)  flags |= FF_PREVIOUS;
	    }
	    kfl->func(kfl->name, kfl->param, flags, pv);  // switch value
	    if (kfl->flags & KF_NOSHOW) {
		btnr(btnx);
		return 0;
	    }
	    lcd_update();
	    is_long = 1;
	}
	else if (km->function && btn(btnx)) {
	    // short key press
	    key_beep();
	    flags = 0;
	    if (!(kf->flags & KF_NOSHOW))  flags = FF_SHOW;
	    if (kf->flags & KF_2STATE) {	// ON/OFF is only for 2-state
		if (*mbs & MBS_ON) {
		    // is ON, switch to OFF
		    *mbs &= (u8)~MBS_ON;
		}
		else {
		    // is OFF, switch to ON
		    *mbs |= MBS_ON;
		    flags |= FF_ON;
		}
		if (km->reverse)       flags |= FF_REVERSE;
		if (km->previous_val)  flags |= FF_PREVIOUS;
	    }
	    kf->func(kf->name, kf->param, flags, pv);  // switch value
	    if (kf->flags & KF_NOSHOW) {
		btnr(btnx);
		return 0;
	    }
	    lcd_update();
	}
	else {
	    // nothing to do
	    btnr(btnx);
	    return 0;
	}
	btnr(btnx);

	// if another button was pressed, leave this screen
	if (buttons)  break;
	if ((buttons_state & ~btnx) != buttons_state_last)  break;

	// sleep 5s, and if no button was changed during, end this screen
	delay_time = POPUP_DELAY * 200;
	while (delay_time && !buttons &&
	       ((buttons_state & ~btnx) == buttons_state_last))
	    delay_time = delay_menu(delay_time);

	if (!buttons)  break;  // timeouted without button press
	if (is_long) {
	    // if long required but short pressed, end
	    if (!btnl(btnx))  break;
	}
	else {
	    // if short required, but long pressed with function_long
	    //   specified, end
	    if (km->function_long && btnl(btnx))  break;
	}
    }

    // set MENU off
    lcd_menu(0);

    return 1;  // popup was showed
}




// check buttons CH3, BACK, END, invoke popup to show value
// return 1 when popup was activated
u8 menu_buttons(void) {
    u8 i;

    // for each key, call function
    for (i = 0; i < KEY_BUTTONS_SIZE; i++) {
	if (i >= NUM_KEYS && ck.key_map[i].is_trim)
	    continue;	// trim is enabled for this key
	if (menu_popup_key(i))  return 1;  // check other keys in main loop
    }

    return 0;
}

