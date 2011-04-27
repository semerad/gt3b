/*
    lcd - LCD routines
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



/* LCD controler is HOLTEK HT1621B */

#include "gt3b.h"
#include <string.h>


// HT1621B modes
#define HT_COMMAND	0b100
// 6 address bits (but only 5 used in this chip) and 4 bits of data
#define HT_WRITE	0b101
#define HT_READ		0b110

// HT1621B commands, 9bits, but last bit is not use in all commands
#define HT_SYS_DIS	0b00000000
#define HT_SYS_EN	0b00000001
#define HT_LCD_OFF	0b00000010
#define HT_LCD_ON	0b00000011
#define HT_TIMER_DIS	0b00000100
#define HT_WDT_DIS	0b00000101
#define HT_TIMER_EN	0b00000110
#define HT_WDT_EN	0b00000111
#define HT_TONE_OFF	0b00001000
#define HT_TONE_ON	0b00001001
#define HT_CLR_TIMER	0b00001100
#define HT_CLR_WDT	0b00001110
#define HT_XTAL_32K	0b00010100
#define HT_RC_256K	0b00011000
#define HT_EX_256K	0b00011100
#define HT_BIAS_12	0b00100000
#define HT_BIAS_13	0b00100001
    #define HT_BIAS_SHIFT 2
#define HT_TONE_4K	0b01000000
#define HT_TONE_2K	0b01100000
#define HT_IRQ_DIS	0b10000000
#define HT_IRQ_EN	0b10001000
#define HT_F1		0b10100000
#define HT_F2		0b10100001
#define HT_F4		0b10100010
#define HT_F8		0b10100011
#define HT_F16		0b10100100
#define HT_F32		0b10100101
#define HT_F64		0b10100110
#define HT_F128		0b10100111
#define HT_TEST		0b11100000
#define HT_NORMAL	0b11100011


// pins setting
#define CS0 BRES(PC_ODR, 3)
#define CS1 BSET(PC_ODR, 3)
#define WR0 BRES(PF_ODR, 4)
#define WR1 BSET(PF_ODR, 4)
#define DATA0 BRES(PE_ODR, 5)
#define DATA1 BSET(PE_ODR, 5)
#define BCK0 BRES(PD_ODR, 2)
#define BCK1 BSET(PD_ODR, 2)



// send cnt bits to LCD controller, only low bits of "bits" are used
// use timer4 to get 600kHz clock for driving WR/ signal
// XXX move it to interrupt code and sleep current task till done
static void lcd_send_bits(u8 cnt, u16 bits) {
    bits <<= 16 - cnt;
    TIM4_CNTR = 0;  // reset timer value
    BSET(TIM4_CR1, 0);  // enable timer
    do {
	// WR low and set data pin
	WR0;
	if (bits & 0x8000) {
	    DATA1;
	}
	else {
	    DATA0;
	}
	// wait til 1.7usec
	while (!(TIM4_SR & 1)) {}
	BRES(TIM4_SR, 0);
	// WR high
	WR1;
	bits <<= 1;
	// wait til 1.7usec
	while (!(TIM4_SR & 1)) {}
	BRES(TIM4_SR, 0);
	// next bit
    } while (--cnt > 0);
    BRES(TIM4_CR1, 0);  // disable timer
}


// send command to LCD
static void lcd_command(u8 cmd) {
    CS0;
    lcd_send_bits(12, (HT_COMMAND << 9) | (cmd << 1));
    CS1;
}


// initialize LCD pins and LCD controller
void lcd_init(void) {
    // set direction of pins
    IO_OPF(C, 3);  // CS/ pin
    IO_OP(E, 5);   // DATA pin
    IO_OP(F, 4);   // WR/ pin
    IO_OP(D, 2);   // Backlight pin

    // set default values to pins
    CS1;
    WR1;
    DATA1;
    BCK0;

    // initialize timer 4 used to time WR/ signal
    BSET(CLK_PCKENR1, 4);   // enable clock to TIM4
    TIM4_CR1 = 0b00000100;  // no auto-reload, URS-overflow, disable
    TIM4_IER = 0;           // no interrupts XXX change to 1
    TIM4_PSCR = 0;          // prescaler = 1
    TIM4_ARR = 30;          // it will be about 600kHz for WR/ signal

    // initialize HT1621B
    lcd_command(HT_BIAS_13 | (0b10 << HT_BIAS_SHIFT));  // BIAS 1/3, 4 COMs
    lcd_command(HT_RC_256K);  // clock RC 256kHz
    lcd_command(HT_SYS_DIS);  // OSC+BIAS off
    lcd_command(HT_WDT_DIS);  // disable WDT
    lcd_command(HT_SYS_EN);   // OSC on
    lcd_command(HT_LCD_ON);   // BIAS on
}


// array of current values of LCD controller memory of segments (32x 4bit)
#define MAX_SEGMENT 32
static u8 lcd_segments[MAX_SEGMENT];
static u32 lcd_modified_segments;


// following variables represents current state of display
// it is used when blinking items
static u8 lcd_sav_char1[LCD_CHAR_COLS];
static u8 lcd_sav_char2[LCD_CHAR_COLS];
static u8 lcd_sav_char3[LCD_CHAR_COLS];
static u8 lcd_sav_number;	// 7-segment number (channel, model, ...)
static u8 lcd_sav_menu;		// bit for each menu in the top of LCD
static u8 lcd_sav_symbols;	// symbols: % V - CHANNEL_NO ...


// set LCD segment "pos" ON of OFF
// pos contains SEGMENT address in bits 0-4 and COM number in bits 7-8
// set bit in array lcs_segments only and set flag that this segment was
//   changed (segment can be changed more times and we will send it to
//   LCD controller only once)
static void lcd_data(u8 pos, u8 on_off) {
    u8 segment = (u8)(pos & 0b00011111);
    u8 com_bit = (u8)(1 << ((pos & 0b11000000) >> 6));
    if (on_off) {
	lcd_segments[segment] |= com_bit;
    }
    else {
	lcd_segments[segment] &= (u8)~com_bit;
    }
    lcd_modified_segments |= (u32)1 << segment;
}


// send changed segments to LCD controller
void lcd_end_write(void) {
    u8 i;
    for (i = 0; i < MAX_SEGMENT; i++) {
	if (!(lcd_modified_segments & ((u32)1 << i)))  continue;
	CS0;
	lcd_send_bits(13, (HT_WRITE << 10) | (i << 4) | lcd_segments[i]);
	CS1;
    }
    lcd_modified_segments = 0;
}


// full LCD clear or set all segments ON
// quicker way, doing continuous-write to LCD controller
// memory locations representing LCD state must be set accordingly
void lcd_clr_set(u8 on_off) {
    u8 data;
    u8 i;
    u16 data4;
    if (on_off) {
	data = 0xf;
	data4 = 0xffff;
	memset(lcd_sav_char1, 0x7f, LCD_CHAR_COLS);
	memset(lcd_sav_char2, 0x7f, LCD_CHAR_COLS);
	memset(lcd_sav_char3, 0x7f, LCD_CHAR_COLS);
	lcd_sav_number = 0x7f;
	lcd_sav_menu = lcd_sav_symbols = 0xff;
    }
    else {
	data = 0;
	data4 = 0;
	memset(lcd_sav_char1, 0, LCD_CHAR_COLS);
	memset(lcd_sav_char2, 0, LCD_CHAR_COLS);
	memset(lcd_sav_char3, 0, LCD_CHAR_COLS);
	lcd_sav_number = lcd_sav_menu = lcd_sav_symbols = 0;
    }
    memset(lcd_segments, data, MAX_SEGMENT);
    CS0;
    lcd_send_bits(9, HT_WRITE << 6);  // set lcd address to 0
    // send 32x 4bits of data in group of 16 bits
    for (i = 0; i < MAX_SEGMENT / 4; i++) {
	lcd_send_bits(16, data4);
    }
    CS1;
    lcd_modified_segments = 0;
}


// functions to set LCD items to given bitmap
static void lcd_set_multi(u8 *sav, u8 bits, u8 *seg, u8 *bitmap, u8 count) {
}


// bits from left-top down, then next column
static const u8 lcd_seg_char1[7 * LCD_CHAR_COLS] = {
    0xda, 0x1a, 0x5a, 0x9a, 0x9d, 0x5d, 0x1d,
    0xd9, 0x19, 0x59, 0x99, 0x9e, 0x5e, 0x1e,
    0xd8, 0x18, 0x58, 0x98, 0x9f, 0x5f, 0x1f,
    0xd7, 0x17, 0x57, 0x97, 0x91, 0x51, 0x11,
    0xd6, 0x16, 0x56, 0x96, 0x90, 0x50, 0x10
};
void lcd_set_char1(u8 *bitmap) {
    lcd_set_multi(lcd_sav_char1, 7, lcd_seg_char1, bitmap, LCD_CHAR_COLS);
}


// bits from left-top down, then next column
static const u8 lcd_seg_char2[7 * LCD_CHAR_COLS] = {
    0xd5, 0x15, 0x55, 0x95, 0x8f, 0x4f, 0x0f,
    0xd4, 0x14, 0x54, 0x94, 0x8e, 0x4e, 0x0e,
    0xd3, 0x13, 0x53, 0x93, 0x8d, 0x4d, 0x0d,
    0xd2, 0x12, 0x52, 0x92, 0x8c, 0x4c, 0x0c,
    0xc0, 0x00, 0x40, 0x80, 0x8b, 0x4b, 0x0b
};
void lcd_set_char2(u8 *bitmap) {
    lcd_set_multi(lcd_sav_char2, 7, lcd_seg_char2, bitmap, LCD_CHAR_COLS);
}


// bits from left-top down, then next column
static const u8 lcd_seg_char3[7 * LCD_CHAR_COLS] = {
    0xc1, 0x01, 0x41, 0x81, 0x8a, 0x4a, 0x0a,
    0xc2, 0x02, 0x42, 0x82, 0x89, 0x49, 0x09,
    0xc3, 0x03, 0x43, 0x83, 0x88, 0x48, 0x08,
    0xc4, 0x04, 0x44, 0x84, 0x87, 0x47, 0x07,
    0xc5, 0x05, 0x45, 0x85, 0x86, 0x46, 0x06
};
void lcd_set_char3(u8 *bitmap) {
    lcd_set_multi(lcd_sav_char3, 7, lcd_seg_char3, bitmap, LCD_CHAR_COLS);
}


static const u8 lcd_seg_number[7] = {  // from left-top down, then next column
    0x9c, 0x1c, 0x9b, 0x5c, 0xdb, 0x5b, 0x1b
};
void lcd_set_number(u8 bitmap) {
    lcd_set_multi(&lcd_sav_number, 7, lcd_seg_number, &bitmap, 1);
}


static const u8 lcd_seg_menu[8] = {  // first line, second line
    0xd0, 0xcd, 0xcb, 0xca, 0xd1, 0xce, 0xcc, 0xc9
};
void lcd_set_menu(u8 bitmap) {
    lcd_set_multi(&lcd_sav_menu, 8, lcd_seg_menu, &bitmap, 1);
}


static const u8 lcd_seg_symbols[8] = {
    0xdf, 0xde, 0xdd, 0xc7, 0xcf, 0xc8, 0xc6, 0xdc  // XXX last two may be swapped, check it
};
void lcd_set_symbols(u8 bitmap) {
    lcd_set_multi(&lcd_sav_symbols, 8, lcd_seg_symbols, &bitmap, 1);
}


// functions to write characters from character map
void lcd_char1(u8 c) {
}

void lcd_char2(u8 c) {
}

void lcd_char3(u8 c) {

}

void lcd_chars(u8 *chars) {
}


// write given number to 7-segment display
void lcd_number(u8 number) {
}

