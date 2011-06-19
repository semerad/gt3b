/*
    timer include file
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


#ifndef _TIMER_INCLUDED
#define _TIMER_INCLUDED


#include "gt3b.h"


// current time from power on, in seconds and 5ms steps
extern volatile u16 time_sec;
extern volatile u8  time_5ms;


// delay in task MENU - will be interrupted by buttons/ADC
extern u16 delay_menu(u16 len_5ms);
extern void delay_menu_always(u8 len_s);


// inactivity timer
extern void reset_inactivity_timer(void);


#endif

