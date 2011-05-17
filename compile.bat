@if "%TOOLSET%"=="" goto NoToolset
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac task.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac main.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac ppm.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac lcd.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac input.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac buzzer.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac timer.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac eeprom.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac config.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac calc.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac menu.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac menu_service.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac menu_global.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac vector.c
%TOOLSET%/clnk -l%TOOLSET%/Lib -o gt3b.sm8 -mgt3b.map compile.lkf
%TOOLSET%/cvdwarf gt3b.sm8
%TOOLSET%/chex -o gt3b.s19 gt3b.sm8
@goto:EOF

:NoToolset
@echo TOOLSET variable is not set, set it with "set TOOLSET=path"
