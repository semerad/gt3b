/*
    menu_key - handle key_mapping menu
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






// set mapping of keys
// key_id is: 4 trims, 3 buttons, 8 individual buttons of trims
// individual buttons of trims are available only when corresponding trim
// is off

#define TRIM_FUNCTIONS_SIZE  128
@near static u8 trim_functions[TRIM_FUNCTIONS_SIZE];
@near static u8 trim_functions_max;
static const u8 trim_buttons[][4] = {
    "NOL", "RPT", "MOM", "RES", "END", "SPC"
};
#define TRIM_BUTTONS_SIZE  (sizeof(trim_buttons) / 4)
// 7seg:  1 2 3 d
// chars:
// function
//   OFF			        (NOR/REV)  (NOO/ORS)    (NPV/PRV)   (NOR/ROT)
//   other -> buttons
//              MOM	     -> 	reverse              -> prev_val
//           NOL/RPT/RES/END -> step -> reverse -> opp_reset             -> rotate
// id:        %                  % V     V          V blink      % blink    % V blink
static void km_trim(u8 action) {
    config_et_map_s *etm = &ck.et_map[menu_id];
    u8 idx, btn, new_idx = 0;

    if (action == 1) {
	// change value
	switch (menu_set) {
	    case 0:
		// function
		// select new function, map through trim_functions
		if (!etm->is_trim)  etm->function = 0;
		idx = menu_et_function_idx(etm->function);
		while (1) {
		    if (btn(BTN_ROT_L)) {
			if (idx)  idx--;
			else      idx = trim_functions_max;
		    }
		    else {
			if (++idx > trim_functions_max)  idx = 0;
		    }
		    new_idx = trim_functions[idx];
		    if (!new_idx)  continue;				// empty slot
		    new_idx--;  // was one more
		    if (menu_et_function_is_allowed(new_idx))  break;	// we have it
		}
		// set values to defaults
		((u16 *)etm)[0] = 0;
		((u16 *)etm)[1] = 0;
		etm->function = new_idx;
		if (etm->function)
		    etm->is_trim = etm->is_trim2 = 1;
		break;
	    case 1:
		// buttons
		// show special ("SPC") only when selected function has it
		if (menu_et_function_long_special(etm->function))
			idx = 1;
		else	idx = 2;
		btn = etm->buttons;
		btn = (u8)menu_change_val(btn, 0, TRIM_BUTTONS_SIZE - idx,
					  1, 1);
		// skip MOMentary for list functions
		if (btn == ETB_MOMENTARY &&
		    menu_et_function_is_list(etm->function)) {
		    if (etm->buttons < ETB_MOMENTARY)  btn++;
		    else  btn--;
		}
		etm->buttons = btn;
		break;
	    case 2:
		// step
		etm->step = (u8)menu_change_val(etm->step, 0,
						STEPS_MAP_SIZE - 1, 1, 0);
		break;
	    case 3:
		// reverse
		etm->reverse ^= 1;
		break;
	    case 4:
		// opposite reset
		etm->opposite_reset ^= 1;
		break;
	    case 5:
		// return to previous value
		etm->previous_val ^= 1;
		break;
	    case 6:
		etm->rotate ^= 1;
		break;
	}
    }

    else if (action == 2) {
	// switch to next setting
	if (menu_set || etm->is_trim) {
	    if (etm->buttons == ETB_MOMENTARY) {
		if (++menu_set > 5)       menu_set = 0;
		else if (menu_set == 2)   menu_set = 3;  // skip "step" for momentary
		else if (menu_set == 4)   menu_set = 5;  // skip "opposite_reset"
	    }
	    else {
		if (++menu_set > 4)  menu_set = 0;
		else if (menu_et_function_is_list(etm->function)) {
		    if (menu_set == 2) {
			// skip "step"
			menu_set++;
			etm->step = 0;
		    }
		    else if (menu_set == 4) {
			// skip "opposite reset"
			menu_set = 6;
			etm->opposite_reset = 0;
		    }
		}
	    }
	}
    }

    // show value of menu_set
    switch (menu_set) {
	case 0:
	    if (!etm->is_trim)  lcd_chars("OFF");
	    else  lcd_chars(menu_et_function_name(etm->function));
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 1:
	    lcd_chars(trim_buttons[etm->buttons]);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 2:
	    lcd_char_num3(steps_map[etm->step]);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 3:
	    lcd_chars(etm->reverse ? "REV" : "NOR");
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 4:
	    lcd_chars(etm->opposite_reset ? "ORS" : "NOO");
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    lcd_segment_blink(LS_SYM_VOLTS, LB_SPC);
	    break;
	case 5:
	    lcd_chars(etm->previous_val ? "PRV" : "NPV");
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    lcd_segment_blink(LS_SYM_PERCENT, LB_SPC);
	    break;
	case 6:
	    lcd_chars(etm->rotate ? "ROT" : "NOR");
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    lcd_segment_blink(LS_SYM_PERCENT, LB_SPC);
	    lcd_segment_blink(LS_SYM_VOLTS, LB_SPC);
	    break;
    }
}




#define KEY_FUNCTIONS_SIZE  32
@near static u8 key_functions[KEY_FUNCTIONS_SIZE];
@near static u8 key_functions_max;
// 7seg:  C b E 1l 1r 2l 2r 3l 3r dl dr
// chars:
// function
//   OFF    -> function_long
//   2STATE -> momentary
//               switch    -> reverse -> prev_val -> function_long
//               momentary -> reverse -> prev_val
//   other  -> function_long
//
// function_long (identified by symbol V)
//   OFF
//   2STATE -> reverse -> prev_val
//   other
static void km_key(u8 action) {
    config_key_map_s *km = &ck.key_map[menu_id - NUM_TRIMS];
    u8 idx, new_idx = 0;

    if (action == 1) {
	// change value
	switch (menu_set) {
	    case 0:
		// function
		// select new function, map through key_functions
		idx = menu_key_function_idx(km->function);
		while (1) {
		    if (btn(BTN_ROT_L)) {
			if (idx)  idx--;
			else      idx = key_functions_max;
		    }
		    else {
			if (++idx > key_functions_max)  idx = 0;
		    }
		    new_idx = key_functions[idx];
		    if (!new_idx)  continue;				// empty slot
		    new_idx--;	// was one more
		    if (menu_key_function_is_allowed(new_idx))  break;	// we have it
		}
		// set values to defaults
		if (km->momentary)  *(u16 *)km = 0;  // was momentary, zero all
		else {
		    // zero only no-long function parameters
		    km->reverse = 0;
		    km->previous_val = 0;
		}
		km->function = new_idx;
		break;
	    case 1:
		// momentary setting
		km->momentary ^= 1;
		// after change momentary, reset long setting
		km->function_long = 0;
		km->reverse_long = 0;
		km->previous_val_long = 0;
		break;
	    case 2:
		// reverse
		km->reverse ^= 1;
		break;
	    case 3:
		// previous_val
		km->previous_val ^= 1;
		break;
	    case 4:
		// function long
		// select new function, map through key_functions
		idx = menu_key_function_idx(km->function_long);
		while (1) {
		    if (btn(BTN_ROT_L)) {
			if (idx)  idx--;
			else      idx = key_functions_max;
		    }
		    else {
			if (++idx > key_functions_max)  idx = 0;
		    }
		    new_idx = key_functions[idx];
		    if (!new_idx)  continue;				// empty slot
		    new_idx--;	// was one more
		    if (menu_key_function_is_allowed(new_idx))  break;	// we have it
		}
		// set values to defaults
		km->reverse_long = 0;
		km->previous_val_long = 0;
		km->function_long = new_idx;
		break;
	    case 5:
		// reverse_long
		km->reverse_long ^= 1;
		break;
	    case 6:
		// previous_val_long
		km->previous_val_long ^= 1;
		break;
	}
    }

    else if (action == 2) {
	// switch to next setting
	switch (menu_set) {
	    case 0:
		if (menu_key_function_2state(km->function))  menu_set = 1;
		else  menu_set = 4;
		break;
	    case 1:
		menu_set = 2;
		break;
	    case 2:
		menu_set = 3;
		break;
	    case 3:
		if (km->momentary)  menu_set = 0;
		else  menu_set = 4;
		break;
	    case 4:
		if (menu_key_function_2state(km->function_long))  menu_set = 5;
		else  menu_set = 0;
		break;
	    case 5:
		menu_set = 6;
		break;
	    case 6:
		menu_set = 0;
		break;
	}
    }

    // show value of menu_set
    lcd_segment(LS_SYM_VOLTS, LS_OFF);
    switch (menu_set) {
	case 0:
	    // function
	    lcd_chars(menu_key_function_name(km->function));
	    break;
	case 1:
	    // momentary setting
	    lcd_chars("MO");
	    lcd_char(LCHR3, (u8)(km->momentary + '0'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);
	    break;
	case 2:
	    // reverse
	    lcd_chars("RE");
	    lcd_char(LCHR3, (u8)(km->reverse + '0'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);
	    break;
	case 3:
	    // previous_val
	    lcd_chars("PV");
	    lcd_char(LCHR3, (u8)(km->previous_val + '0'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);
	    break;
	case 4:
	    // function long
	    lcd_chars(menu_key_function_name(km->function_long));
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 5:
	    // reverse_long
	    lcd_chars("RE");
	    lcd_char(LCHR3, (u8)(km->reverse_long + '0'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 6:
	    // previous_val_long
	    lcd_chars("PV");
	    lcd_char(LCHR3, (u8)(km->previous_val_long + '0'));
	    menu_blink &= (u8)~(MCB_CHR1 | MCB_CHR2);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
    }
}







@inline static void km_trim_key(u8 action) {
    if (menu_id < NUM_TRIMS)  km_trim(action);
    else		      km_key(action);
}

static const u8 key_ids[] = {
    1, 2, 3, L7_D, L7_C, L7_B, L7_E
};

void menu_key_mapping_func(u8 action, void *p) {
    if (action == MCA_SET_CHG) {
	km_trim_key(1);
	return;		// value already showed
    }
    else if (action == MCA_SET_NEXT) {
	km_trim_key(2);
	return;		// value already showed
    }
    else if (action == MCA_ID_CHG) {
	while (1) {
	    // select prev/next menu_id
	    if (btn(BTN_ROT_L)) {
		if (menu_id)	menu_id--;
		else		menu_id = 3 * NUM_TRIMS + NUM_KEYS - 1;
	    }
	    else {
		if (++menu_id >= 3 * NUM_TRIMS + NUM_KEYS)  menu_id = 0;
	    }
	    // trims and 3keys (CH3/BACK/END) always
	    if (menu_id < NUM_TRIMS + NUM_KEYS)  break;
	    // check trim keys and use them only when corresponding
	    //   trim is off
	    if (ck.key_map[menu_id - NUM_TRIMS].is_trim)  continue;
	    break;
	}
    }

    // show value
    if (menu_id < NUM_TRIMS + NUM_KEYS)
	// standard trims and buttons
	lcd_7seg(key_ids[0]);
    else {
	// trims as buttons, use arrows to show them
	lcd_7seg(key_ids[(u8)((u8)(menu_id - NUM_TRIMS - NUM_KEYS) >> 1)]);
	if ((u8)(menu_id - NUM_TRIMS - NUM_KEYS) & 1)
	    // right trim key
	    lcd_segment(LS_SYM_RIGHT, LS_ON);
	else
	    // left trim key
	    lcd_segment(LS_SYM_LEFT, LS_ON);
    }
    km_trim_key(0);
}

void menu_key_mapping(void) {
    lcd_set_blink(LMENU, LB_SPC);

    menu_common(menu_key_mapping_func, NULL, MCF_ID_CHG);

    lcd_set_blink(LMENU, LB_OFF);
    config_model_save();
    apply_model_config();
    menu_buttons_initialize();
}





// prepare sorted functions for mapping keys
void menu_key_mapping_prepare(void) {
    u8 i;
    s8 n;

    // electronic trims
    for (i = 0; i < TRIM_FUNCTIONS_SIZE; i++) {
	n = menu_et_function_idx(i);
	if (n < 0)  break;  // last
	if (n > trim_functions_max)  trim_functions_max = n;
	trim_functions[n] = (u8)(i + 1);  // + 1 to be able to find non-empty
    }
    // keys
    for (i = 0; i < KEY_FUNCTIONS_SIZE; i++) {
	n = menu_key_function_idx(i);
	if (n < 0)  break;  // last
	if (n > key_functions_max)  key_functions_max = n;
	key_functions[n] = (u8)(i + 1);   // + 1 to be able to find non-empty
    }
}


