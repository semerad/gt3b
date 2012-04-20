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
#include "input.h"




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
#define TIMER_MAX_LAPCNT 255
static @near u8  timer_lap_count[TIMER_NUM];
// time for each lap
#define TIMER_MAX_LAPS 100
static @near menu_timer_s timer_lap_time[TIMER_NUM][TIMER_MAX_LAPS];
// time when lap button can be pressed (to eliminate double-clicks)
static @near u16 next_timer_sec[TIMER_NUM];
static @near u16 next_timer_sec_display[TIMER_NUM];




// show timer value on 3-char display
// time format  - left-chars right-chars
//   0000	- minutes:seconds
//   0000 %	- seconds:hundredths
static void timer_value(menu_timer_s *pt) {
    u16 tsec;
    u8  min, sec, hdr;

    // atomically read timer value
    if (pt) {
	TIMER_READ(pt, tsec, hdr);
    }
    else {
	tsec = 0;
	hdr = 0;
    }

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
    u8 lap;

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
	    // if several seconds from last LAP press, show actual timer value
	    if (time_sec >= next_timer_sec_display[tid]) {
		timer_value(pt);
		break;
	    }
	    // show last lap time for several seconds
	    lap = timer_lap_count[tid];
	    if (lap > 1) {
		menu_timer_s *pt2, tm;
		s16 hdr;
		// to last lap id
		lap--;  // to last lap id
		while (lap >= TIMER_MAX_LAPS)  lap -= TIMER_MAX_LAPS;
		timer_value(&timer_lap_time[tid][lap]);
		// calculate lap time as difference between lap and lap -1
		if (lap) pt2 = &timer_lap_time[tid][lap - 1];
		else     pt2 = &timer_lap_time[tid][TIMER_MAX_LAPS - 1];
		pt = &timer_lap_time[tid][lap];
		tm.sec = pt->sec - pt2->sec;
		hdr = pt->hdr - pt2->hdr;
		if (hdr < 0) {
		    tm.sec--;
		    hdr += 100;
		}
		tm.hdr = (u8)hdr;
		timer_value(&tm);
	    }
	    else if (lap)
		// first lap
		timer_value(&timer_lap_time[tid][0]);
	    else
		// no lap
		timer_value(NULL);
	    // blink this last lap time
	    lcd_set_blink(L7SEG, LB_SPC);
	    lcd_chars_blink(LB_SPC);
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
    "OFF", "UP ", "DWN", "LAP", "LPC"
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
    // stop timer
    menu_timer_running &= (u8)~(u8)(1 << tid);
    next_timer_sec_display[tid] = 0;	// display last value
    // set timer_id for subfunctions
    timer_id = tid;
    menu_list(timer_setup_funcs, sizeof(timer_setup_funcs) / sizeof(void *), MCF_NONE);
    config_global_save();
}





// show timer lap times
//   menu_id:
//   	0       first lap
//   	1       second lap
//   	....
//   	nlap    RES reset and end menu, after last lap
//   	nlap+1  AVG average time
//   	nlap+2  TOT total time
static void lap_times_func(u8 action, u8 *ptid) {
    u8 tid = *ptid;
    u8 laps = timer_lap_count[tid];
    menu_timer_s *pt, tm;
    u8 type = TIMER_TYPE(tid);

    // max 100 laps
    if (laps > TIMER_MAX_LAPS)  laps = TIMER_MAX_LAPS;

    switch (action) {
	case MCA_INIT:
	    // to TOTal item when not down timer
	    if (type != TIMER_DOWN)  menu_id = (u8)(laps + 2);
	    break;
	case MCA_SET_CHG:
	case MCA_ID_CHG:
	    // change to other lap, skip TOT ang AVG for down timer
	    menu_id = (u8)menu_change_val(menu_id, 0,
		      laps + (type != TIMER_DOWN ? 2 : 0), LAP_SHOW_FAST, 1);
	    break;
	case MCA_SWITCH:
	    // switch between lap numbers and times
	    if (menu_id == laps) {
		// if RESset selected, delete all values end exit
		menu_timer_clear(tid, 1);
		menu_set = 255;		// flag to exit
		return;
	    }
	    // actual switch
	    menu_id_set ^= 1;
	    break;
    }

    // show value
    menu_blink = 0;	// no blinking
    timer_show_id(tid);
    // blink arrow to indicate lap times viewing
    lcd_segment_blink(LS_SYM_LEFT, LB_SPC);
    lcd_segment_blink(LS_SYM_RIGHT, LB_SPC);

    // show lap number
    if (!menu_id_set) {
	lcd_7seg(L7_L);
	if (menu_id == laps)
	    lcd_chars("RES");
	else if (menu_id > laps) {
	    u8 laps10 = (u8)(laps / 10);
	    // show ID Average/Total
	    if (menu_id == laps + 1)  lcd_char(LCHR1, 'A');  // average
	    else                      lcd_char(LCHR1, 'T');  // total
	    // show number of laps
	    if (laps10 == 10)      lcd_char(LCHR2, LCHAR_ONE_ZERO);
	    else if (laps10 == 0)  lcd_char(LCHR2, ' ');
	    else                   lcd_char(LCHR2, (u8)(laps10 + '0'));
	    lcd_char(LCHR3, (u8)(laps % 10 + '0'));
	}
	else lcd_char_num3(menu_id + 1);
	return;
    }

    // show timer value
    if (menu_id == laps) {
	// reset
	lcd_set(L7SEG, LB_EMPTY);
	lcd_chars("RES");
	return;
    }
    else if (menu_id == laps + 1) {
	// average lap times
	if (laps) {
	    // we have at least one lap
	    u32 tot;
	    pt = &timer_lap_time[tid][laps - 1];
	    // count in hundredths
	    tot = (u32)pt->sec * 100 + pt->hdr;
	    // average
	    tot = (tot + laps / 2) / laps;
	    // assign to timer val
	    tm.sec = (u16)(tot / 100);
	    tm.hdr = (u8)(tot % 100);
	}
	else {
	    // no laps, show zero
	    tm.sec = 0;
	    tm.hdr = 0;
	}
	pt = &tm;
    }
    else if (menu_id == laps + 2)
	// total, read time of last lap
	if (laps) pt = &timer_lap_time[tid][laps - 1];
	else {
	    // no laps, show zero
	    tm.sec = 0;
	    tm.hdr = 0;
	    pt = &tm;
	}
    else {
	// show lap time
	if (menu_id == 0 || type == TIMER_DOWN)
	    // first lap or down timer
	    pt = &timer_lap_time[tid][menu_id];
	else {
	    // calculate lap time as difference between lap and lap -1
	    menu_timer_s *pt2 = &timer_lap_time[tid][menu_id - 1];
	    s16 hdr;
	    pt = &timer_lap_time[tid][menu_id];
	    tm.sec = pt->sec - pt2->sec;
	    hdr = pt->hdr - pt2->hdr;
	    if (hdr < 0) {
		tm.sec--;
		hdr += 100;
	    }
	    tm.hdr = (u8)hdr;
	    pt = &tm;
	}
    }
    timer_value(pt);
}

void menu_timer_lap_times(u8 tid) {
    u8 type = TIMER_TYPE(tid);

    switch (type) {

	case TIMER_OFF:
	case TIMER_UP:
	case TIMER_LAPCNT:
	    // no lap times
	    menu_timer_setup(tid);
	    return;
	    break;

	case TIMER_DOWN:
	case TIMER_LAP:
	    break;

    }

    // show lap times
    menu_common(lap_times_func, &tid, MCF_SWITCH);
}



// save lap time
static void timer_lap_time_save(u8 tid) {
    menu_timer_s *pt;
    u8  *plc = &timer_lap_count[tid];
    u16 tsec;
    u8  thdr;
    u8  lap;

    // read timer
    pt = &menu_timer[tid];
    TIMER_READ(pt, tsec, thdr);

    // find lap position where to save it
    //   if over, start from zero
    lap = *plc;
    if (lap == TIMER_MAX_LAPCNT)  return;  // too many laps
    while (lap >= TIMER_MAX_LAPS)  lap -= TIMER_MAX_LAPS;

    // save value
    pt = &timer_lap_time[tid][lap];
    pt->sec = tsec;
    pt->hdr = thdr;

    // increment lap count
    (*plc)++;
}



// key functions
void kf_menu_timer_start(u8 *id, u8 *param, u8 flags, s16 *pv) {
    u8 tid = (u8)(u16)param;
    menu_timer_s *pt = &menu_timer[tid];
    u8 type = TIMER_TYPE(tid);
    u8 tbit = (u8)(1 << tid);

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
	    if (time_sec < next_timer_sec[tid])  return;  // pressed too early
	    next_timer_sec[tid] = time_sec + 3;
	    // when not running, start it
	    if (!(menu_timer_running & tbit)) {
		menu_timer_running |= tbit;
		next_timer_sec_display[tid] = 0;	// display running value
		break;
	    }
	    // when alarmed already, stop timer
	    if (menu_timer_alarmed & tbit) {
		menu_timer_running &= (u8)~tbit;
		menu_timer_throttle &= (u8)~tbit;
		next_timer_sec_display[tid] = 0;	// display last value
		timer_lap_time_save(tid);
		break;
	    }
	    // save to lap times
	    next_timer_sec_display[tid] = time_sec + 3;
	    timer_lap_time_save(tid);
	    break;

	case TIMER_LAPCNT:
	    if (time_sec < next_timer_sec[tid])  return;  // pressed too early
	    next_timer_sec[tid] = time_sec + 3;
	    // max 255 laps
	    if (timer_lap_count[tid] == TIMER_MAX_LAPCNT)  break;
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
	    menu_timer_running &= (u8)~tbit;
	    // save rest value to lap count
	    if (menu_timer_alarmed & tbit) {
		// if alarmed, save zeroes
		pt->sec = 0;
		pt->hdr = 0;
	    }
	    timer_lap_time_save(tid);
	    // reset timer
	    menu_timer_clear(tid, 0);
	    menu_timer_throttle &= (u8)~tbit;
	    break;

	case TIMER_LAP:
	    // stop timer
	    menu_timer_running &= (u8)~tbit;
	    menu_timer_throttle &= (u8)~tbit;
	    next_timer_sec_display[tid] = 0;	// display last value
	    break;

	case TIMER_LAPCNT:
	    timer_lap_count[tid] = 0;
	    break;

    }

    menu_main_screen = (u8)(MS_TIMER0 + tid);
}

