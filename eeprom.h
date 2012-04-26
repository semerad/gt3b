/*
    eeprom include file
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


#ifndef _EEPROM_INCLUDED
#define _EEPROM_INCLUDED


#include "config.h"


// eeprom address and size
#define EEPROM_START  0x4000
#define EEPROM_SIZE   0x400


// position of global config
#define EEPROM_CONFIG_GLOBAL  (u8 *)EEPROM_START


// position of models config
#define EEPROM_CONFIG_MODEL  (u8 *)(EEPROM_CONFIG_GLOBAL + sizeof(config_global_s))


extern void eeprom_read_global(void);
extern void eeprom_read_model(u8 model);
extern void flash_read_model(u8 model);
extern void eeprom_write_global(void);
extern void eeprom_write_model(u8 model);
extern void flash_write_model(u8 model);  // to flash starting at 0
extern void eeprom_empty_models(void);


#endif

