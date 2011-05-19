/*
    eeprom - routines to accessing eeprom
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
#include "eeprom.h"



void eeprom_init(void) {
    // enable writing to eeprom
    /*  moved to eeprom_write to do it only temporary
    FLASH_DUKR = 0xAE;
    FLASH_DUKR = 0x56;
    */
}




// read from eeprom

@inline static void eeprom_read(u8 *ee_addr, void *ram_addr, u16 length) {
    memcpy(ram_addr, ee_addr, length); 
}


void eeprom_read_global(void) {
    eeprom_read(EEPROM_CONFIG_GLOBAL, &config_global, sizeof(config_global_s));
}


void eeprom_read_model(u8 model) {
    u8 size = sizeof(config_model_s);
    eeprom_read(EEPROM_CONFIG_MODEL + model * size, &config_model, size);
}





// write to eeprom

static void eeprom_write(u8 *ee_addr, u8 *ram_addr, u16 length) {
    u8 i = 10;

    // enable write to eeprom
    FLASH_DUKR = 0xAE;
    FLASH_DUKR = 0x56;
    // check if write is enabled for some time
    do {
	if (BCHK(FLASH_IAPSR, 3))  break;
    } while (--i);
    // write only values, which are different
    do {
	if (*ee_addr != *ram_addr) {
	    *ee_addr = *ram_addr;
	}
	ee_addr++;
	ram_addr++;
    } while (--length);
    // wait to end of write and disable write to eeprom
    while (!BCHK(FLASH_IAPSR, 2))  pause();
    BRES(FLASH_IAPSR, 3);
}


void eeprom_write_global(void) {
    eeprom_write(EEPROM_CONFIG_GLOBAL, (u8 *)&config_global, sizeof(config_global_s));
}


void eeprom_write_model(u8 model) {
    u8 size = sizeof(config_model_s);
    eeprom_write(EEPROM_CONFIG_MODEL + model * size, (u8 *)&config_model, size);
}




// initialize model memories to empty one
void eeprom_empty_models(void) {
    config_model_s *cm = (config_model_s *)EEPROM_CONFIG_MODEL;
    u8 cnt = CONFIG_MODEL_MAX;
    do {
	cm->name[0] = CONFIG_MODEL_EMPTY;
	cm++;
    } while (--cnt);
}

