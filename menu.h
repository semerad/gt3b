/*
    menu include file
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


#ifndef _MENU_INCLUDED
#define _MENU_INCLUDED


#include "gt3b.h"



// variables to be used in CALC task
extern _Bool ch3_state;		// state of channel 3 button






// task MENU will be alias for task OPER
#define MENU OPER


// menu tasks will be handling ADC now (for calibrate)
extern _Bool menu_wants_adc;
// flag low battery voltage
extern _Bool menu_battery_low;
// raw battery ADC value for check to battery low
extern u16 battery_low_raw;


#endif

