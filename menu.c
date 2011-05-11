/*
    menu - handle showing to display, menus
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



#include "menu.h"
#include "config.h"
#include "calc.h"
#include "timer.h"
#include "ppm.h"




_Bool menu_takes_adc;




// calibrate menu
static void calibrate(void) {
    menu_takes_adc = 1;

    menu_takes_adc = 0;
}



// show main screen (model number and name)
static void main_screen(void) {

}




// main menu loop, shows main screen and menus
static void menu_loop(void) {
    while (1) {
	main_screen();

	pause();
    }
}



// initialize variables
void menu_init(void) {
    // variables
    
    // read global config from eeprom
    if (config_global_read())
	calibrate();

    // read model config from eeprom
    config_model_read();
    ppm_set_channels(config_model.channels);

    // and main loop
    menu_loop();
}

