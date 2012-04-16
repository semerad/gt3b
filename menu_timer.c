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
#include "lcd.h"
#include "config.h"




// menu task will be waked-up peridodically to show timer value
_Bool menu_timer_wakeup;


// actual timer values
u8        menu_timer_running;		// running timers
@near u8  menu_timer_throttle;		// throttle start for each timer
@near menu_timer_s menu_timer[TIMER_NUM];	// actual timer values
// number of laps
static @near u8  timer_lap_count[TIMER_NUM];
// time for each lap
#define TIMER_MAX_LAPS 100
static @near menu_timer_s timer_lap_time[TIMER_NUM][TIMER_MAX_LAPS];





// show actual timer value
void menu_timer_show(u8 tid) {
    u8 type = TIMER_TYPE(tid);

    menu_clear_symbols();
    lcd_7seg((u8)(tid + 1));

    switch (type) {

	case TIMER_OFF:
	    lcd_chars("OFF");
	    break;

	case TIMER_UP:
	    lcd_chars("NDY");	// XXX
	    break;

	case TIMER_DOWN:
	    lcd_chars("NDY");	// XXX
	    break;

	case TIMER_LAP:
	    lcd_chars("NDY");	// XXX
	    break;

	case TIMER_LAPCNT:
	    lcd_char_num3(timer_lap_count[tid]);
	    break;

    }
}




// clear timer
static void timer_clear(u8 tid) {
    menu_timer_s *pt = &menu_timer[tid];
    pt->sec = 0;
    pt->hdr = 0;
    menu_timer_running &= (u8)~(u8)(1 << tid);
    timer_lap_count[tid] = 0;
    memset(&timer_lap_time[tid], 0, TIMER_MAX_LAPS * sizeof(menu_timer_s));
}




// setup timer

static u8 timer_id;	// for setup to know which timer to operate
static u8 timer_setup_throttle(u8 val_id, u8 action, u8 *chars_blink) {
    // change value
    if (action == 1)
	menu_timer_throttle ^= (u8)(1 << timer_id);

    // select next value
    else if (action == 2)  timer_clear(timer_id);

    // show value
    lcd_7seg(L7_H);
    lcd_chars(menu_timer_throttle & (u8)(1 << timer_id) ? "ON " : "OFF");

    return 1;	// only one value
}

static u8 timer_setup_alarm(u8 val_id, u8 action, u8 *chars_blink) {
    u8 val = TIMER_ALARM(timer_id);

    // change value
    if (action == 1) {
	val = (u8)menu_change_val(val, 0, 255, TIMER_ALARM_FAST, 0);
	TIMER_ALARM_SET(timer_id, val);
    }

    // select next value
    else if (action == 2)  timer_clear(timer_id);

    // show value
    lcd_7seg(L7_A);
    lcd_char_num3(val);

    return 1;	// only one value
}

static const u8 timer_type_labels[][5] = {
    "OFF", "UP ", "DWN", "LPT", "LPC"
};
static u8 timer_setup_type(u8 val_id, u8 action, u8 *chars_blink) {
    u8 val = TIMER_TYPE(timer_id);

    // change value
    if (action == 1) {
	val = (u8)menu_change_val(val, 0, TIMER_TYPE_MAX, 1, 1);
	TIMER_TYPE_SET(timer_id, val);
    }

    // select next value
    else if (action == 2)  timer_clear(timer_id);

    // show value
    lcd_7seg(L7_P);
    lcd_chars(timer_type_labels[val]);

    return 1;	// only one value
}

static const menu_func_t timer_setup_funcs[] = {
    timer_setup_throttle,
    timer_setup_alarm,
    timer_setup_type,
};

void menu_timer_setup(u8 tid) {
    timer_id = tid;
    menu_common(timer_setup_funcs, sizeof(timer_setup_funcs) / sizeof(void *), 0);
    config_global_save();
}





// show timer lap times
void menu_timer_lap_times(u8 tid) {
    u8 type = TIMER_TYPE(tid);

    switch (type) {

	case TIMER_OFF:
	case TIMER_UP:
	case TIMER_LAPCNT:
	    // no lap times
	    return;
	    break;

	case TIMER_DOWN:
	case TIMER_LAP:
	    // show lap times XXX
	    break;

    }
}




// key functions
void kf_menu_timer_start(u8 *id, u8 *param, u8 flags, s16 *pv) {
    u8 tid = (u8)(u16)param;
    u8 type = TIMER_TYPE(tid);
    menu_timer_s *pt = &menu_timer[tid];

    switch (type) {

	case TIMER_OFF:
	    return;
	    break;

	case TIMER_UP:
	    // XXX
	    break;

	case TIMER_DOWN:
	    // XXX
	    break;

	case TIMER_LAP:
	    // XXX
	    break;

	case TIMER_LAPCNT:
	    timer_lap_count[tid]++;
	    break;

    }

    menu_main_screen = (u8)(MS_TIMER1 + tid);
}

void kf_menu_timer_reset(u8 *id, u8 *param, u8 flags, s16 *pv) {
    u8 tid = (u8)(u16)param;
    u8 type = TIMER_TYPE(tid);
    menu_timer_s *pt = &menu_timer[tid];

    switch (type) {

	case TIMER_OFF:
	    return;
	    break;

	case TIMER_UP:
	    // XXX
	    break;

	case TIMER_DOWN:
	    // XXX
	    break;

	case TIMER_LAP:
	    // XXX
	    break;

	case TIMER_LAPCNT:
	    timer_lap_count[tid] = 0;
	    break;

    }

    menu_main_screen = (u8)(MS_TIMER1 + tid);
}

