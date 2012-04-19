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
#include "buzzer.h"
#include "timer.h"




// menu task will be waked-up peridodically to show timer value
_Bool menu_timer_wakeup;


// actual timer values
u8        menu_timer_running;		// running timers
u8	  menu_timer_direction;		// up(0)/down(1) timer direction
u8	  menu_timer_alarmed;		// down timer was over, one bit for one timer
@near u8  menu_timer_throttle;		// throttle start for each timer
@near menu_timer_s menu_timer[TIMER_NUM];	// actual timer values
@near u16 menu_timer_alarm[TIMER_NUM];		// alarm in seconds
// number of laps
static @near u8  timer_lap_count[TIMER_NUM];
// time for each lap
#define TIMER_MAX_LAPS 100
static @near menu_timer_s timer_lap_time[TIMER_NUM][TIMER_MAX_LAPS];




// show timer value on 3-char display
// time format  - left-chars right-chars
//   0000	- minutes:seconds
//   0000 %	- seconds:hundredths
static void timer_value(menu_timer_s *pt) {
    u16 tsec;
    u8  min, sec, hdr;

    // atomically read timer value
    TIMER_READ(pt, tsec, hdr);

    // convert to minutes, seconds, 0.01 seconds
    min = (u8)(tsec / 60);
    sec = (u8)(tsec % 60);

    // show value
    if (min) {
	// show minutes and seconds
	// rounding up
	if (hdr >= 50)  sec++;
	if (sec == 60)  { sec = 0; min++; }
	lcd_7seg((u8)(min / 10));
	lcd_char(LCHR1, (u8)(min % 10 + '0'));
	lcd_char(LCHR2, (u8)(sec / 10 + '0'));
	lcd_char(LCHR3, (u8)(sec % 10 + '0'));
    }
    else {
	// show seconds and hundredths
	lcd_segment(LS_SYM_PERCENT, LS_ON);
	lcd_7seg((u8)(sec / 10));
	lcd_char(LCHR1, (u8)(sec % 10 + '0'));
	lcd_char(LCHR2, (u8)(hdr / 10 + '0'));
	lcd_char(LCHR3, (u8)(hdr % 10 + '0'));
    }
}


// show timer ID as left and right arrow
static void timer_show_id(u8 tid) {
    if (tid)  lcd_segment(LS_SYM_RIGHT, LS_ON); \
    else      lcd_segment(LS_SYM_LEFT, LS_ON);
}



// show actual timer value
void menu_timer_show(u8 tid) {
    menu_timer_s *pt = &menu_timer[tid];
    u8 type = TIMER_TYPE(tid);
    u8 tbit = (u8)(1 << tid);

    menu_clear_symbols();
    timer_show_id(tid);

    switch (type) {

	case TIMER_OFF:
	    lcd_set(L7SEG, LB_EMPTY);
	    lcd_chars("OFF");
	    return;
	    break;

	case TIMER_UP:
	    timer_value(pt);
	    break;

	case TIMER_DOWN:
	    timer_value(pt);
	    if (menu_timer_alarmed & tbit) {
		lcd_segment(LS_SYM_VOLTS, LS_ON);
		lcd_segment_blink(LS_SYM_VOLTS, LS_ON);
	    }
	    break;

	case TIMER_LAP:
	    lcd_chars("NDY");	// XXX
	    lcd_set(L7SEG, LB_EMPTY);
	    break;

	case TIMER_LAPCNT:
	    lcd_set(L7SEG, LB_EMPTY);
	    lcd_char_num3(timer_lap_count[tid]);
	    return;
	    break;

    }

    // if timer is running, set it to wakeup this task
    if (menu_timer_running & (u8)tbit)
	menu_timer_wakeup = 1;
}




// clear timer
void menu_timer_clear(u8 tid, u8 laps) {
    menu_timer_s *pt = &menu_timer[tid];
    u8 type = TIMER_TYPE(tid);
    u8 tbit = (u8)(1 << tid);

    // set it not running
    menu_timer_running &= (u8)~tbit;

    // zero values, laps only when requested
    pt->hdr = 0;
    menu_timer_alarmed &= (u8)~tbit;
    if (laps == 1 || (laps == 2 && type != TIMER_DOWN)) {
	timer_lap_count[tid] = 0;
	memset(&timer_lap_time[tid], 0, TIMER_MAX_LAPS * sizeof(menu_timer_s));
    }

    // set alarm
    if (type == TIMER_LAPCNT)
	menu_timer_alarm[tid] = TIMER_ALARM(tid);
    else menu_timer_alarm[tid] = TIMER_ALARM(tid) * 60;

    // set direction and seconds
    if (type == TIMER_DOWN) {
	// down timer
	pt->sec = menu_timer_alarm[tid];
	if (!pt->sec)  pt->sec = 1;	// at least 1 second alarm time
	menu_timer_direction |= tbit;
    }
    else {
	// up timer
	pt->sec = 0;
	menu_timer_direction &= (u8)~tbit;
    }
}




// setup timer

static u8 timer_id;	// for setup to know which timer to operate
static void timer_setup_throttle(u8 action) {
    // change value
    if (action == MLA_CHG)
	menu_timer_throttle ^= (u8)(1 << timer_id);

    // select next value
    else if (action == MLA_NEXT)  menu_timer_clear(timer_id, 2);

    // show value
    lcd_7seg(L7_H);
    lcd_chars(menu_timer_throttle & (u8)(1 << timer_id) ? "ON " : "OFF");
    timer_show_id(timer_id);
}

static void timer_setup_alarm(u8 action) {
    u8 val = TIMER_ALARM(timer_id);

    // change value
    if (action == MLA_CHG) {
	val = (u8)menu_change_val(val, 0, 255, TIMER_ALARM_FAST, 0);
	TIMER_ALARM_SET(timer_id, val);
	// set value to global var as lap counts or as minutes
	if (TIMER_TYPE(timer_id) == TIMER_LAPCNT)
	    menu_timer_alarm[timer_id] = val;
	else menu_timer_alarm[timer_id] = val * 60;
    }

    // select next value
    else if (action == MLA_NEXT)  menu_timer_clear(timer_id, 2);

    // show value
    lcd_7seg(L7_A);
    lcd_char_num3(val);
    timer_show_id(timer_id);
}

static const u8 timer_type_labels[][4] = {
    "OFF", "UP ", "DWN", "LPT", "LPC"
};
static void timer_setup_type(u8 action) {
    u8 tid = timer_id;	// compiler hack
    u8 val = TIMER_TYPE(tid);

    // change value
    if (action == MLA_CHG) {
	val = (u8)menu_change_val(val, 0, TIMER_TYPE_MAX, 1, 1);
	TIMER_TYPE_SET(timer_id, val);
    }

    // select next value
    else if (action == MLA_NEXT)  menu_timer_clear(timer_id, 1);

    // show value
    lcd_7seg(L7_P);
    lcd_chars(timer_type_labels[val]);
    timer_show_id(timer_id);
}

static const menu_list_t timer_setup_funcs[] = {
    timer_setup_throttle,
    timer_setup_alarm,
    timer_setup_type,
};

void menu_timer_setup(u8 tid) {
    timer_id = tid;
    menu_list(timer_setup_funcs, sizeof(timer_setup_funcs) / sizeof(void *), MCF_NONE);
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
	    break;

    }

    // show lap times XXX
}




// key functions
void kf_menu_timer_start(u8 *id, u8 *param, u8 flags, s16 *pv) {
    u8 tid = (u8)(u16)param;
    menu_timer_s *pt = &menu_timer[tid];
    u8 type = TIMER_TYPE(tid);
    u8 tbit = (u8)(1 << tid);
    static u16 next_timer_sec[TIMER_NUM];

    switch (type) {

	case TIMER_OFF:
	    return;
	    break;

	case TIMER_UP:
	case TIMER_DOWN:
	    // start/pause timer
	    menu_timer_running ^= tbit;
	    menu_timer_throttle &= (u8)~tbit;
	    break;

	case TIMER_LAP:
	    // XXX
	    break;

	case TIMER_LAPCNT:
	    if (time_sec < next_timer_sec[tid])  return;  // pressed too early
	    next_timer_sec[tid] = time_sec + 3;
	    if (++timer_lap_count[tid] == menu_timer_alarm[tid]) {
		// alarm when number of laps elapsed
		buzzer_on(60, 0, 1);
	    }
	    break;

    }

    menu_main_screen = (u8)(MS_TIMER0 + tid);
}

void kf_menu_timer_reset(u8 *id, u8 *param, u8 flags, s16 *pv) {
    u8 tid = (u8)(u16)param;
    menu_timer_s *pt = &menu_timer[tid];
    u8 type = TIMER_TYPE(tid);
    u8 tbit = (u8)(1 << tid);

    switch (type) {

	case TIMER_OFF:
	    return;
	    break;

	case TIMER_UP:
	    // stop and reset timer
	    menu_timer_clear(tid, 1);
	    menu_timer_throttle &= (u8)~tbit;
	    break;

	case TIMER_DOWN:
	    // stop and reset timer
	    menu_timer_running &= (u8)~(u8)(1 << tid);
	    // save rest value to lap count XXX
	    // reset timer
	    menu_timer_clear(tid, 0);
	    menu_timer_throttle &= (u8)~tbit;
	    break;

	case TIMER_LAP:
	    // XXX
	    break;

	case TIMER_LAPCNT:
	    timer_lap_count[tid] = 0;
	    break;

    }

    menu_main_screen = (u8)(MS_TIMER0 + tid);
}

