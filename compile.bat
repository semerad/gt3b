make[1]: Entering directory `/home/semerad/c/stm8/GT3B'
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac task.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac main.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac ppm.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac lcd.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac input.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac buzzer.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac timer.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac eeprom.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac config.c
cxstm8 +warn +proto +mods0 +debug -i.  -l  -pxp -ac vector.c
clnk  -o gt3b.sm8 -mgt3b.map compile.lkf
cvdwarf gt3b.sm8
chex -o gt3b.s19 gt3b.sm8
make[1]: Leaving directory `/home/semerad/c/stm8/GT3B'
