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
#include "lcd.h"
#include "buzzer.h"
#include "input.h"




_Bool menu_takes_adc;
_Bool menu_wants_battery;
_Bool menu_battery_low;




// calibrate menu
static void calibrate(void) {
    menu_takes_adc = 1;

    menu_takes_adc = 0;
}


// key test menu
static void key_test(void) {
    
}


// show model number, extra function to handle more than 10 models
static void show_model_number(u8 model) {
    lcd_7seg(cg.model);
}


// show main screen (model number and name)
static void main_screen(void) {
    lcd_clear();
    show_model_number(cg.model);
    lcd_chars(cm.name);
}


// menu stop - checks low battery
static void menu_stop(void) {
    static _Bool battery_low_on;
    stop();
    if (battery_low_on == menu_battery_low)  return;
    if (menu_battery_low) {
	// battery low firstly
	battery_low_on = 1;
	lcd_segment(LS_SYM_LOWPWR, LS_ON);
	lcd_segment_blink(LS_SYM_LOWPWR, LB_SPC);
	buzzer_on(40, 160, BUZZER_MAX);
    }
    else {
	// battery low now OK
	battery_low_on = 0;
	lcd_segment(LS_SYM_LOWPWR, LS_OFF);
	buzzer_off();
    }
}


// main menu loop, shows main screen and menus
static void menu_loop(void) {
    main_screen();
    while (1) {

	menu_stop();
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
    ppm_set_channels(cm.channels);

    // and main loop
    menu_loop();
}

