@if "%TOOLSET%"=="" goto NoToolset
@if not "%CHANNELS%"=="" goto ChannelOK
@set CHANNELS=3
:ChannelOK
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% task.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% main.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% ppm.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% lcd.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% input.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% buzzer.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% timer.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% eeprom.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% config.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% calc.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% menu.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% menu_service.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% menu_global.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% menu_popup.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% menu_mix.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% menu_key.c
%TOOLSET%/cxstm8 +warn +proto +mods0 +debug -i. -i%TOOLSET%/Hstm8 -l  -pxp -ac -dMAX_CHANNELS=%CHANNELS% vector.c
%TOOLSET%/clnk -l%TOOLSET%/Lib -o gt3b.sm8 -mgt3b.map compile.lkf
%TOOLSET%/cvdwarf gt3b.sm8
%TOOLSET%/chex -o gt3b.s19 gt3b.sm8
@goto:EOF

:NoToolset
@echo TOOLSET variable is not set, set it with "set TOOLSET=path"
