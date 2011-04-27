/*
    main include file
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



#include <iostm8s.h>


// frequency of crystal in kHz
#define KHZ  ((u16)18432)
// development board has another crystal
//#define KHZ  ((u16)16000)


/* enabling/disabling interrupts */
#define sim()  _asm("sim")
#define rim()  _asm("rim")


/* define types sX, uX */
typedef signed char    s8;
typedef unsigned char  u8;
typedef signed short   s16;
typedef unsigned short u16;
typedef signed long    s32;
typedef unsigned long  u32;
#define hi8(val) ((u8)((val) >> 8))
#define lo8(val) ((u8)((val) & 0xff))


/* macros for setting bit to 0 or 1 */
#define BSET(addr, pin) \
    addr |= 1 << pin
#define BRES(addr, pin) \
    addr &= (u8)~(1 << pin)


/* macros for defining I/O pins */
#define IO_IF(port, pin) \
    BRES(P ## port ## _DDR, pin); \
    BRES(P ## port ## _CR1, pin); \
    BRES(P ## port ## _CR2, pin)
#define IO_IP(port, pin) \
    BRES(P ## port ## _DDR, pin); \
    BSET(P ## port ## _CR1, pin); \
    BRES(P ## port ## _CR2, pin)
#define IO_OP(port, pin) \
    BSET(P ## port ## _DDR, pin); \
    BSET(P ## port ## _CR1, pin); \
    BRES(P ## port ## _CR2, pin)
#define IO_OPF(port, pin) \
    BSET(P ## port ## _DDR, pin); \
    BSET(P ## port ## _CR1, pin); \
    BSET(P ## port ## _CR2, pin)




/* PPM */
#define MAX_CHANNELS 8
extern u8 channels;
extern void ppm_set_channels(u8 n);
extern void ppm_set_value(u8 channel, u16 microsec);
extern void ppm_calc_sync(void);


/* LCD */
extern void lcd_clr_set(u8 on_off);
extern void lcd_end_write(void);
#define lcd_clr()  lcd_clr_set(0);

#define LCD_CHAR_COLS 5
#define LCD_MENU_MODEL 0x80
#define LCD_MENU_NAME  0x40
#define LCD_MENU_REV   0x20
#define LCD_MENU_EPO   0x10
#define LCD_MENU_TRIM  0x08
#define LCD_MENU_DR    0x04
#define LCD_MENU_EXP   0x02
#define LCD_MENU_ABS   0x01

#define LCD_SYM_LOWPWR  0x80
#define LCD_SYM_CHANNEL 0x40
#define LCD_SYM_MODELNO 0x20
#define LCD_SYM_PERCENT 0x10
#define LCD_SYM_LEFT    0x08
#define LCD_SYM_RIGHT   0x04
#define LCD_SYM_DOT     0x02
#define LCD_SYM_VOLTS   0x01


/* INPUT - keys, steering, throttle */


/* buzzer */


/* timer */
extern u16 time;

