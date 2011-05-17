/*
    buzzer include file
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


#ifndef _BUZZER_INCLUDED
#define _BUZZER_INCLUDED


#include "gt3b.h"


// setting on/off
#define BUZZER0  BRES(PD_ODR, 4)
#define BUZZER1  BSET(PD_ODR, 4)
#define BUZZER_CHK  BCHK(PD_ODR, 4)



// functions to use
#define BUZZER_MAX (u16)(0xffff)
extern void buzzer_on(u8 on_5ms, u8 off_5ms, u16 count);
extern void buzzer_off(void);
extern void beep(u8 len_5ms);
extern void key_beep(void);



// variables used at timer
extern _Bool buzzer_running;
extern u8 buzzer_cnt_on;
extern u8 buzzer_cnt_off;
extern u16 buzzer_count;
extern u8 buzzer_cnt;


#endif

