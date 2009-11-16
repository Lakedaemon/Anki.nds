#include <stdlib.h>
#include <fat.h>
#include <unistd.h>
#include <nds/registers_alt.h>
#include <dswifi9.h>

#include "fileselect.h"
#include "review.h"

//#include "memcheck.h"

void VideoInit()
{
    videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE | DISPLAY_BG2_ACTIVE);
    videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);

    vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
    vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
    vramSetBankC(VRAM_C_SUB_BG_0x06200000);

    SUB_BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(0);
    BG3_CR = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_PRIORITY(3);
    BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(8) | BG_PRIORITY(2);

    // no rotation or shearing
    SUB_BG3_XDY = 0;
    SUB_BG3_XDX = 1 << 8;
    SUB_BG3_YDX = 0;
    SUB_BG3_YDY = 1 << 8;
    BG3_XDY = 0;
    BG3_XDX = 1 << 8;
    BG3_YDX = 0;
    BG3_YDY = 1 << 8;
    BG2_XDY = 0;
    BG2_XDX = 1 << 8;
    BG2_YDX = 0;
    BG2_YDY = 1 << 8;

    lcdMainOnBottom();
}



int main(int argc, char** argv)
{
    powerOn(POWER_ALL);
    defaultExceptionHandler();
    //fifoInit();
    VideoInit();
    
    fatInitDefault();
    Wifi_InitDefault(INIT_ONLY);
    
    srand(time(NULL));

    //chdir("ankids");
    chdir("Anki");
    //createDefaultFontCache("default.ttf");

    while (true) {
        Preferences* pref = new Preferences;
        
        FileSelect* fs = new FileSelect(pref);
        fs->Run();
        delete fs;
        
        if (!pref->syncloop) {
            Review* re = new Review(pref);
            re->Run();
            delete re;
        }

        delete pref;
    }

    return 0;
}
