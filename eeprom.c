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



/*
    first N models are stored at EEPROM after global config
    last eeprom model config is used to temporarily store of FLASH model
	memory, because EEPROM has 1M write cycles (flash 10K) and
	writing to FLASH stops microcontroller (EEPROM has ReadWhileWrite)
    models at FLASH are stored from end of FLASH down to end of program
    when FLASH model is selected, it is copied to temporary EEPROM place
	and is stored back only after another model is selected
*/



#include <string.h>
#include "eeprom.h"





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

void flash_read_model(u8 model) {
    u8 size = sizeof(config_model_s);
    eeprom_read((u8 *)(0 - (model + 1) * size), &config_model, size);
}





// write to eeprom

static void eeprom_make_writable(void *addr) {
    u8 i = 10;
    // enable write to eeprom/flash
    if ((u16)addr < 0x8000) {
	// eeprom
	FLASH_DUKR = 0xAE;
	FLASH_DUKR = 0x56;
	// wait for Data EEPROM area unlocked flag
	do {
	    if (BCHK(FLASH_IAPSR, 3))  break;
	} while (--i);
    }
    else {
	// flash
	FLASH_PUKR = 0x56;
	FLASH_PUKR = 0xAE;
	// wait for Flash Program memory unlocked flag
	do {
	    if (BCHK(FLASH_IAPSR, 1))  break;
	} while (--i);
    }
}

static void eeprom_make_readonly(void *addr) {
    if ((u16)addr < 0x8000)  BRES(FLASH_IAPSR, 3);
    else		     BRES(FLASH_IAPSR, 1);
}


static void eeprom_write(u8 *ee_addr, u8 *ram_addr, u16 length) {
    eeprom_make_writable(ee_addr);
    // write only values, which are different, check and write at
    // Word mode (4 bytes)
    length /= 4;
    do {
	if (*(u16 *)ee_addr != *(u16 *)ram_addr ||
	    *(u16 *)(ee_addr + 2) != *(u16 *)(ram_addr + 2)) {
	    // enable Word programming
	    BSET(FLASH_CR2, 6);
	    BRES(FLASH_NCR2, 6);
	    // write 4-byte value
	    ee_addr[0] = ram_addr[0];
	    ee_addr[1] = ram_addr[1];
	    ee_addr[2] = ram_addr[2];
	    ee_addr[3] = ram_addr[3];
	    // wait for EndOfProgramming flag
	    while (!BCHK(FLASH_IAPSR, 2))  pause();
	}
	ee_addr += 4;
	ram_addr += 4;
    } while (--length);
    eeprom_make_readonly(ee_addr);
}


void eeprom_write_global(void) {
    eeprom_write(EEPROM_CONFIG_GLOBAL, (u8 *)&config_global, sizeof(config_global_s));
}


void eeprom_write_model(u8 model) {
    u8 size = sizeof(config_model_s);
    eeprom_write(EEPROM_CONFIG_MODEL + model * size, (u8 *)&config_model, size);
}

void flash_write_model(u8 model) {
    u8 size = sizeof(config_model_s);
    eeprom_write((u8 *)(0 - (model + 1) * size), (u8 *)&config_model, size);
}




// initialize model memories to empty one
void eeprom_empty_models(void) {
    u8 cnt = CONFIG_MODEL_MAX_EEPROM;
    config_model_s *cm = (config_model_s *)EEPROM_CONFIG_MODEL;

    // EEPROM first
    eeprom_make_writable(cm);
    // write 0x00 to first letter of each model config
    do {
	if (cm->name[0] != CONFIG_MODEL_EMPTY) {
	    cm->name[0] = CONFIG_MODEL_EMPTY;
	    // wait for EOP
	    while (!BCHK(FLASH_IAPSR, 2))  pause();
	}
	cm++;
    } while (--cnt);
    eeprom_make_readonly(cm);

    // FLASH second
    cnt = CONFIG_MODEL_MAX_FLASH;
    cm = (config_model_s *)(-sizeof(config_model_s));
    eeprom_make_writable(cm);
    // write 0x00 to first letter of each model config
    do {
	if (cm->name[0] != CONFIG_MODEL_EMPTY) {
	    cm->name[0] = CONFIG_MODEL_EMPTY;
	    // wait for EOP
	    while (!BCHK(FLASH_IAPSR, 2))  pause();
	}
	cm--;
    } while (--cnt);
    eeprom_make_readonly(cm);
}

