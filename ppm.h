/*
    ppm include file
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


#ifndef _PPM_INCLUDED
#define _PPM_INCLUDED



#include "stm8.h"
#include "task.h"


#define MAX_CHANNELS 8
extern u8 channels;	// actual number of channels
extern void ppm_set_channels(u8 n);
extern void ppm_set_value(u8 channel, u16 microsec);
extern void ppm_calc_sync(void);


#endif

