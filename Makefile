
PROGRAM	= gt3b
SRCC	= task.c main.c ppm.c lcd.c input.c buzzer.c timer.c
INTRS	= vector.c
SMODE	= 
#SMODE	= l
LIBS	=
#LIBS	= libfs0.sm8
DEBUG	=
DEBUG	= +debug
#DEBUG	= +debug -no
VERBOSE	=
#VERBOSE = -v
ENDMEMORY = 0x5ff
STACK	= 0x7ff

TOOLSET	= c:/sw/CXSTM8_16K
#TOOLSET = c:/sw/CXSTM8_32K
WINEPREFIX = $(HOME)/winsw/stm8

include Makefile.stm8

