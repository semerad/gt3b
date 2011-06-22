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
static const u8 *trim_buttons[] = {
    "NOL", "RPT", "MOM", "RES", "END", "SPC"
};
#define TRIM_BUTTONS_SIZE  (sizeof(trim_buttons) / sizeof(u8 *))
// 7seg:  1 2 3 d
// chars:
// function
//   OFF			        (NOR/REV)  (NOO/RES)    (NPV/PRV)
//   other -> buttons
//              MOM	     -> 	reverse              -> prev_val
//           NOL/RPT/RES/END -> step -> reverse -> opp_reset
// id:        %                  % V     V          V blink      % blink
static u8 km_trim(u8 trim_id, u8 val_id, u8 action) {
    config_et_map_s *etm = &ck.et_map[trim_id];
    u8 id = val_id;
    u8 idx;

    if (action == 1) {
	// change value
	switch (id) {
	    case 1:
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
		    if (trim_functions[idx])  break;  // we have it
		}
		// set values to defaults
		((u16 *)etm)[0] = 0;
		((u16 *)etm)[1] = 0;
		etm->function = (u8)(trim_functions[idx] - 1);
		if (etm->function)
		    etm->is_trim = etm->is_trim2 = 1;
		break;
	    case 2:
		// buttons
		// show special ("SPC") only when selected function has it
		if (menu_et_function_long_special(etm->function))
			idx = 1;
		else	idx = 2;
		etm->buttons = (u8)menu_change_val(etm->buttons, 0,
						 TRIM_BUTTONS_SIZE - idx, 1, 1);
		break;
	    case 3:
		// step
		etm->step = (u8)menu_change_val(etm->step, 0,
						STEPS_MAP_SIZE - 1, 1, 0);
		break;
	    case 4:
		// reverse
		etm->reverse ^= 1;
		break;
	    case 5:
		// opposite reset
		etm->opposite_reset ^= 1;
		break;
	    case 6:
		// return to previous value
		etm->previous_val ^= 1;
	}
    }

    else if (action == 2) {
	// switch to next setting
	if (id != 1 || etm->is_trim) {
	    if (etm->buttons == ETB_MOMENTARY) {
		if (++id > 6)       id = 1;
		else if (id == 3)   id = 4;  // skip "step" for momentary
		else if (id == 5)   id = 6;  // skip "opposite_reset"
	    }
	    else {
		if (++id > 5)  id = 1;
	    }
	}
    }

    // show value of val_id
    switch (id) {
	case 1:
	    if (!etm->is_trim)  lcd_chars("OFF");
	    else  lcd_chars(menu_et_function_name(etm->function));
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 2:
	    lcd_chars(trim_buttons[etm->buttons]);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 3:
	    lcd_char_num3(steps_map[etm->step]);
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 4:
	    lcd_chars(etm->reverse ? "REV" : "NOR");
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 5:
	    lcd_chars(etm->opposite_reset ? "RES" : "NOO");
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    lcd_segment_blink(LS_SYM_VOLTS, LB_SPC);
	    break;
	case 6:
	    lcd_chars(etm->previous_val ? "NPV" : "PRV");
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    lcd_segment_blink(LS_SYM_PERCENT, LB_SPC);
	    break;
    }
    return id;
}




#define KEY_FUNCTIONS_SIZE  32
@near static u8 key_functions[KEY_FUNCTIONS_SIZE];
@near static u8 key_functions_max;
// 7seg:  C b E 1l 1r 2l 2r 3l 3r dl dr
// chars: function
//          OFF    -> function_long (% V)
//          2STATE -> momentary (%)
//                      SWI -> function_long (% V)
//                      MOM -> reverse(NOR/REV) (V)
//          other  -> function_long (% V)
static u8 km_key(u8 key_id, u8 val_id, u8 action) {
    config_key_map_s *km = &ck.key_map[key_id];
    u8 id = val_id;
    u8 idx;

    if (action == 1) {
	// change value
	switch (id) {
	    case 1:
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
		    if (key_functions[idx])  break;  // we have it
		}
		*(u16 *)km = 0;  // set values to defaults
		km->function = (u8)(key_functions[idx] - 1);
		break;
	    case 2:
		// momentary setting
		km->momentary ^= 1;
		break;
	    case 3:
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
		    if (key_functions[idx])  break;  // we have it
		}
		km->function_long = (u8)(key_functions[idx] - 1);
		break;
	    case 4:
		// reverse
		km->reverse ^= 1;
		break;
	}
    }

    else if (action == 2) {
	// switch to next setting
	switch (id) {
	    case 1:
		if (menu_key_function_2state(km->function))  id = 2;
		else  id = 3;
		break;
	    case 2:
		if (km->momentary)  id = 4;
		else  id = 3;
		break;
	    case 3:
		id = 1;
		break;
	    case 4:
		id = 1;
		break;
	}
    }

    // show value of val_id
    switch (id) {
	case 1:
	    // function
	    lcd_chars(menu_key_function_name(km->function));
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 2:
	    // momentary setting
	    lcd_chars(km->momentary ? "MOM" : "SWI");
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_OFF);
	    break;
	case 3:
	    // function long
	    lcd_chars(menu_key_function_name(km->function_long));
	    lcd_segment(LS_SYM_PERCENT, LS_ON);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
	case 4:
	    // reverse
	    lcd_chars(km->reverse ? "REV" : "NOR");
	    lcd_segment(LS_SYM_PERCENT, LS_OFF);
	    lcd_segment(LS_SYM_VOLTS, LS_ON);
	    break;
    }
    return id;
}







static u8 km_trim_key(u8 key_id, u8 val_id, u8 action) {
    if (key_id < NUM_TRIMS)  return km_trim(key_id, val_id, action);
    else  return km_key((u8)(key_id - NUM_TRIMS), val_id, action);
}

void menu_key_mapping(void) {
    u8 key_id = 0;			// trims, keys, trim-keys
    u8 id_val = 0;			// now in key_id
    static const u8 key_ids[] = {
	1, 2, 3, L7_D, L7_C, L7_B, L7_E
    };

    lcd_set_blink(LMENU, LB_SPC);
    lcd_segment(LS_SYM_MODELNO, LS_OFF);
    lcd_segment(LS_SYM_LEFT, LS_OFF);
    lcd_segment(LS_SYM_RIGHT, LS_OFF);
    lcd_7seg(key_ids[0]);
    lcd_set_blink(L7SEG, LB_SPC);
    km_trim_key(0, 1, 0);		// show first setting for first trim
    lcd_update();

    while (1) {
	btnra();
	menu_stop();

	if (btn(BTN_BACK | BTN_END) || btnl(BTN_ENTER))  break;

	if (btn(BTN_ROT_ALL)) {
	    if (id_val) {
		// change selected key setting
		km_trim_key(key_id, id_val, 1);
		lcd_chars_blink(LB_SPC);
		lcd_update();
	    }
	    else {
		// change key-id
		while (1) {
		    if (btn(BTN_ROT_L)) {
			if (key_id)	key_id--;
			else		key_id = 3 * NUM_TRIMS + NUM_KEYS - 1;
		    }
		    else {
			if (++key_id >= 3 * NUM_TRIMS + NUM_KEYS)  key_id = 0;
		    }
		    if (key_id < NUM_TRIMS + NUM_KEYS) {
			// trims and 3keys (CH3/BACK/END) always
			lcd_7seg(key_ids[key_id]);
			lcd_segment(LS_SYM_LEFT, LS_OFF);
			lcd_segment(LS_SYM_RIGHT, LS_OFF);
			break;
		    }
		    // check trim keys and use them only when corresponding
		    //   trim is off
		    if (ck.key_map[key_id - NUM_TRIMS].is_trim)  continue;

		    lcd_7seg(key_ids[(u8)((u8)(key_id - NUM_TRIMS - NUM_KEYS) >> 1)]);
		    if ((u8)(key_id - NUM_TRIMS - NUM_KEYS) & 1) {
			// right trim key
			lcd_segment(LS_SYM_RIGHT, LS_ON);
			lcd_segment(LS_SYM_LEFT, LS_OFF);
		    }
		    else {
			// left trim key
			lcd_segment(LS_SYM_RIGHT, LS_OFF);
			lcd_segment(LS_SYM_LEFT, LS_ON);
		    }
		    break;
		}
		km_trim_key(key_id, 1, 0);	// show first key setting
		lcd_set_blink(L7SEG, LB_SPC);
		lcd_update();
	    }
	}

	else if (btn(BTN_ENTER)) {
	    // switch key-id/key-setting1/key-setting2/...
	    if (id_val) {
		// what to do depends on what was selected in this item
		id_val = km_trim_key(key_id, id_val, 2);
		if (id_val != 1) {
		    lcd_chars_blink(LB_SPC);
		}
		else {
		    // switch to key selection
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


