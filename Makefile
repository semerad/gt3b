
PROGRAM	= gt3b
SRCC	= task.c main.c ppm.c lcd.c input.c buzzer.c timer.c eeprom.c config.c calc.c menu.c menu_service.c
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

include Makefile.toolset

include Makefile.stm8

