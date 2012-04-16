
PROGRAM	= gt3b
SRCC	= task.c main.c ppm.c lcd.c input.c buzzer.c timer.c eeprom.c config.c calc.c menu_common.c menu.c menu_service.c menu_global.c menu_popup.c menu_mix.c menu_key.c menu_timer.c
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
ENDMEMORY = 0x6ff
STACK	= 0x7ff

include Makefile.toolset

include Makefile.stm8

include Makefile.doxygen

