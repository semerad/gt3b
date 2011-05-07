/*
    lcd - include file
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


#ifndef _LCD_INCLUDED
#define _LCD_INCLUDED



// manipulating individual segments, pos is one of 128 segments
//   as special value (see below)
#define LS_OFF	0
#define LS_ON	1
extern void lcd_segment(u8 pos, u8 on_off);
#define LB_OFF	0
#define LB_SPC	1
#define LB_INV	2
extern void lcd_segment_blink(u8 pos, u8 on_off);

// arrays of segment positions for lcd_segment()
// for chars and 7seg is order from top left to down, then next column
// for menu is order from top left to right, then next line
#define LCD_CHAR_COLS 5
#define LCD_CHAR_ROWS 7
#define LCD_CHAR_BYTES (LCD_CHAR_COLS * LCD_CHAR_ROWS)
extern const u8 lcd_seg_char1[];
extern const u8 lcd_seg_char2[];
extern const u8 lcd_seg_char3[];
extern const u8 lcd_seg_7seg[];
extern const u8 lcd_seg_menu[];
// lcd segments addresses for symbols
// XXX last two can be swapped
#define LS_SYM_LOWPWR	0xdf
#define LS_SYM_CHANNEL	0xde
#define LS_SYM_MODELNO	0xdd
#define LS_SYM_PERCENT	0xc7
#define LS_SYM_LEFT	0xcf
#define LS_SYM_RIGHT	0xc8
#define LS_SYM_DOT	0xc6
#define LS_SYM_VOLTS	0xdc
// lcd segment addresses for menu
#define LS_MENU_MODEL	0xd0
#define LS_MENU_NAME	0xcd
#define LS_MENU_REV	0xcb
#define LS_MENU_EPO	0xca
#define LS_MENU_TRIM	0xd1
#define LS_MENU_DR	0xce
#define LS_MENU_EXP	0xcc
#define LS_MENU_ABS	0xc9




// manipulating group of segment together by applying bitmaps
extern void lcd_set(u8 id, u8 *bitmap);
extern void lcd_set_blink(u8 id, u8 on_off);

// IDs for lcd_set(), lcd_set_blink()
#define LCHR1		0
#define LCHR2		1
#define LCHR3		2
#define L7SEG		4
#define LMENU		5
// special (fake) bitmaps for lcd_set()
#define LB_EMPTY	(u8 *)0xff00
#define LB_FULL		(u8 *)0xffff




// high level functions to write chracters and numbers
#define LCHAR_MIN ' '
#define LCHAR_MAX 'Z'
#define L7SEG_MAX 15
extern void lcd_char(u8 id, u8 c);  // id the same as in lcd_set()
extern void lcd_chars(u8 *chars);   // 3 chars
extern void lcd_char_num3(u16 num); // num 0-... to 3 chars
extern void lcd_char_num2(s8 num);  // num -99..99
extern void lcd_char_num2_lbl(s8 num, u8 *labels);  // num -99..99 with labels for <0, =0, >0
extern void lcd_7seg(u8 number);    // num 0-15 (>=10 as hexa numbers)
extern void lcd_menu(u8 menus);     // OR-ed selected menus

// values for lcd_menu()
#define LM_MODEL	0x80
#define LM_NAME		0x40
#define LM_REV		0x20
#define LM_EPO		0x10
#define LM_TRIM		0x08
#define LM_DR		0x04
#define LM_EXP		0x02
#define LM_ABS		0x01




// update/clear/set
extern void lcd_update(void);  // this one actually write to display
extern void lcd_clear(void);
extern void lcd_set_full_on(void);




// for lcd_blink_cnt, maximal value and value when changing to space
// in 5ms steps
#define LCD_BLNK_CNT_MAX	160
#define LCD_BLNK_CNT_BLANK	120
extern volatile _Bool lcd_blink_flag;
extern volatile u8    lcd_blink_cnt;
extern volatile _Bool lcd_blink_something;




// for backlight
#define LCD_BCK0 BRES(PD_ODR, 2)
#define LCD_BCK1 BSET(PD_ODR, 2)
extern _Bool lcd_bck_on;
extern u16   lcd_bck_count;

#define BACKLIGHT_MAX  (u16)(0xffff)
extern void backlight_set_default(u16 seconds);
extern void backlight_on_sec(u16 seconds);
extern void backlight_on(void);
extern void backlight_off(void);




// LCD task
E_TASK(LCD);


#endif

