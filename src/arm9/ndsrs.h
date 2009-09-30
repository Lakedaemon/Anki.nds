#ifndef _NDSRS_H_
#define _NDSRS_H_

#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <fat.h>
#include <sys/dir.h>

#include "util.h"
#include "../common/fifo.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

const int DAYSECONDS = 60*60*24;

typedef char fakelong[32];

enum MENUITEM {
    SCORE0,
    SCORE1,
    SCORE2,
    SCORE3,
    DOODLE,
    SHOW,
};

enum SCREEN {
    TOP = 0x01,
    BOTTOM = 0x02,
    BOTH = 0x03,
    STAT = 0x08,
};

class Preferences {
    public:
        Preferences();
        ~Preferences();
    
        char ip[32];
        int timezone;
        int topfontsize;
        int bottomfontsize;
        bool AutoConnect;
        char SSID[32];
        char Host[64];
        char WebPortalIp[32];
        char Post[512];
        char LookFor[512];
        char Success[128];  
        
        char path[128];
        char srs[128];
        char rep[128];
        int todo;
        int total;
        int syncloop;
        fakelong* todolist;
};

class NDSRS {    
    protected:
        u16 screen_top[256*192];
        u16 screen_btm[256*192];
        u16 screen_sta[256*192];
        u16* videobuf_main;
        u16* videobuf_sub;
        u16* videobuf_sub_stats;
        //u16* videobuf_sub_doodle;
        
        // drawing stuff!
        void VideoInit();
        void DrawScreen(SCREEN);
        void ClearScreen(SCREEN);
        void ClearRect(SCREEN, int, int, int, int);
        
        int NumberTodo();
    
        void ScreenShot();
    
    public:
        NDSRS();
        //~NDSRS();
};












#endif /* _NDSRS_H_ */
