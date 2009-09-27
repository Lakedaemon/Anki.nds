#include <nds.h>
#include <dswifi7.h>

#include "../common/fifo.h"


u8 isOldDS = 0;
u8 backlight = 1;

void vCountHandler() {
    u16 pressed = REG_KEYXY;

    inputGetAndSend();
}

void vBlankHandler() {
    Wifi_Update();
}

void FIFOBacklight(u32 value, void* data)
{
    if (isOldDS) {
        u32 power = readPowerManagement(PM_CONTROL_REG) & ~(PM_BACKLIGHT_TOP|PM_BACKLIGHT_BOTTOM);
        if (backlight) {
            backlight = 0;
        } else {
            backlight = 3;
            power = power | PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM;
        }
        writePowerManagement(PM_CONTROL_REG, power);
    } else {
        backlight = ((backlight+1) & 0x3); //b should be in range 0..3
        writePowerManagement(4, backlight);
    }
}


int main(int argc, char** argv) {
    irqInit();
    fifoInit();

    readUserSettings();
    initClockIRQ();
    SetYtrigger(80);

    fifoSetValue32Handler(FIFO_BACKLIGHT, &FIFOBacklight, NULL);

    //Setup FIFO
    installWifiFIFO();
    installSystemFIFO();

    //Setup IRQ
    irqSet(IRQ_VCOUNT, vCountHandler);
    irqSet(IRQ_VBLANK, vBlankHandler);
    irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);

    isOldDS = !(readPowerManagement(4) & BIT(6));
    backlight = isOldDS ? 3 : readPowerManagement(4);


    while(1)
        swiWaitForVBlank();
}
