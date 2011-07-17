/**
 *  @copyright
 *    Copyright (C) 2011 Pavel Semerad
 *
 *  @file
 *    $RCSfile: $
 *
 *  @purpose
 *    gt3b alternative firmware - buzzer routines
 *
 *  @cfg_management
 *    $Source: $
 *    $Author: $
 *    $Date: $
 *
 *  @comment
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "buzzer.h"
#include "config.h"

/** @brief
 *    buzzer initialization
 */
void buzzer_init(void) {
    IO_OP(D, 4);	// buzzer pin
    BUZZER0;		// stop buzzer
}


// buzzer counters and flags
_Bool buzzer_running;	// 1 when running
u8 buzzer_cnt_on;	// time to be ON
u8 buzzer_cnt_off;	// time to be OFF
u16 buzzer_count;	// length of beeping
u8 buzzer_cnt;		// helper counter


// more beeps: on time, off time and count of beeps
void buzzer_on(u8 on_5ms, u8 off_5ms, u16 count) {
    buzzer_cnt_on = buzzer_cnt = on_5ms;
    buzzer_cnt_off = off_5ms;
    buzzer_count = count;
    BUZZER1;
    buzzer_running = 1;
}


/// @brief
///    stop buzzer
///
void buzzer_off(void) {
    BUZZER0;
    buzzer_running = 0;
}


// one beep
void beep(u8 len_5ms) {
    if (buzzer_running)  return;
    buzzer_on(len_5ms, 0, 1);
}


// key beep if enabled in config
void key_beep(void) {
    if (cg.key_beep)  beep(1);
}

