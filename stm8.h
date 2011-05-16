/*
    stm8 - common stm8 include
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


#ifndef _STM8_INCLUDED
#define _STM8_INCLUDED


#include <iostm8s.h>


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
    addr |= (u8)(1 << pin)
#define BRES(addr, pin) \
    addr &= (u8)~(1 << pin)
#define BCHK(addr, pin) \
    (addr & (u8)(1 << pin))


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
#define IO_OO(port, pin) \
    BSET(P ## port ## _DDR, pin); \
    BRES(P ## port ## _CR1, pin); \
    BRES(P ## port ## _CR2, pin)
#define IO_OPF(port, pin) \
    BSET(P ## port ## _DDR, pin); \
    BSET(P ## port ## _CR1, pin); \
    BSET(P ## port ## _CR2, pin)
#define IO_OOF(port, pin) \
    BSET(P ## port ## _DDR, pin); \
    BRES(P ## port ## _CR1, pin); \
    BSET(P ## port ## _CR2, pin)


#endif

