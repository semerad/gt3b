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



// manipulating individual segments
#define LS_OFF	0
#define LS_ON	1
extern void lcd_segment(u8 pos, u8 on_off);
#define LB_OFF	0
#define LB_SPC	1
#define LB_INV	2
extern void lcd_segment_blink(u8 pos, u8 on_off);

// arrays of segment positions for LCD items
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


// IDs for lcd_set(), lcd_set_blink()
#define LCHR1		0
#define LCHR2		1
#define LCHR3		2
#define L7SEG		4
#define LMENU		5
// special (fake) bitmaps for lcd_set()
#define LB_EMPTY	(u8 *)0xff00
#define LB_FULL		(u8 *)0xffff
#define LBM_MODEL	(u8 *)0xff80
#define LBM_NAME	(u8 *)0xff40
#define LBM_REV		(u8 *)0xff20
#define LBM_EPO		(u8 *)0xff10
#define LBM_TRIM	(u8 *)0xff08
#define LBM_DR		(u8 *)0xff04
#define LBM_EXP		(u8 *)0xff02
#define LBM_ABS		(u8 *)0xff01
extern void lcd_set(u8 id, u8 *bitmap);
extern void lcd_set_blink(u8 id, u8 on_off);


// high level functions to write chracters and numbers
extern void lcd_char(u8 id, u8 c);
extern void lcd_chars(u8 *chars);
extern void lcd_char_num3(u16 num);
extern void lcd_char_num2(s8 num);
extern void lcd_char_num2_lbl(s8 num, u8 *labels);
extern void lcd_7seg(u8 number);


// update/clear/set
extern void lcd_update(void);
extern void lcd_clr(void);
extern void lcd_set_full_on(void);


// for lcd_blink_cnt, maximal value and value when changing to space
#define LCD_BLNK_CNT_MAX	30
#define LCD_BLNK_CNT_BLANK	15

