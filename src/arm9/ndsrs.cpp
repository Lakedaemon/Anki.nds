#include "ndsrs.h"
#include "card.h"

#include "png.h"
#include <stdio.h>
Preferences::Preferences()
{

    /*if (!diropen(".") ) {
                            text->PrintString("Couldn't open the directory");
                            text->BlitToScreen(videobuf_sub);
                            text->ClearBuffer();
                            swiWaitForVBlank(); 
                            waitForAnyKey();	
                            return;
    }*/
    FILE* p = fopen("Anki.conf", "r");
    char* buf = new char[1024];
    
    todolist = NULL;
    
    strcpy(ip, "192.168.1.100");
    timezone = 0;
    topfontsize = 14;
    bottomfontsize = 12;
    
    AutoConnect=true;
    strcpy(SSID,"FreeWifi");
    strcpy(Host,"wifi.free.fr");
    strcpy(WebPortalIp,"212.27.40.236");
    memset(LookFor,0,sizeof(LookFor));
    memset(Post,0,sizeof(Post));
    strcpy(Success,"No Success String"); 
    
    while (true) {
        readline(p, buf);
        if (buf[0] == 0)
            break;
        if (buf[0] == '#')
            continue;
            
        int l = strlen(buf);
        bool ok = false;
        for (int i = 0; i < l; ++i) {
            if (buf[i] == '=') {
                ok = true;
                break;
            }
        }
        if (!ok)
            continue;
        
        char key[128];
        char value[128];
        char TempKey[128];
        char TempValue[128];
        
        sscanf(buf, "%[^=]=%[^\n]", key, value);       
        if (!strncmp(key, "ip", 128))
            sscanf(value, "%s", &ip);
        if (!strncmp(key, "timezone", 128))
            sscanf(value, "%d", &timezone);
        if (!strncmp(key, "topfontsize", 128))
            sscanf(value, "%d", &topfontsize);
        if (!strncmp(key, "bottomfontsize", 128))
            sscanf(value, "%d", &bottomfontsize);
    
        if (!strncmp(key, "AutoConnect", 128)){
                char Temp[128];
                sscanf(value, "%s", &Temp);
                AutoConnect=true;
                if (!strncmp(Temp,"False",128)) AutoConnect=false;
        }
        if (!strncmp(key, "SSID", 128))
            sscanf(value, "%s", &SSID);   
    
        if (!strncmp(key, "Host", 128))
            sscanf(value, "%s", &Host);  
    
        if (!strncmp(key, "WebPortal Ip", 128))
            sscanf(value, "%s", &WebPortalIp);   
    
        if (!strncmp(key, "Post", 128)){
                char* Pointer=strstr(value,"<--End-->");
                if (!Pointer){
                        sscanf(value, "%[^:]:%s", TempKey,TempValue);
                        sprintf(value,"%s=%s&",TempKey,TempValue);
                        strcat(Post,value);
                }else{
                        Pointer += strlen("<--End-->");
                        *Pointer = 0;
                        strcat(LookFor,value);
                }
        }

        if (!strncmp(key, "Success", 128)){
                char* Pointer = strstr(value,"<--End-->");
                if (Pointer){
                        *Pointer=0;
                        strcpy(Success,value);
                }else{
                        strcpy(Success,"Bad Success String, missing\"<--End-->\"");
                }
        }
        
    }
    
    delete[] buf;
    fclose(p);
}

Preferences::~Preferences()
{
    if (todolist)
        delete[] todolist;
}



NDSRS::NDSRS()
{
    
}

void NDSRS::VideoInit()
{
    videobuf_main = (u16*)BG_BMP_RAM_SUB(0);
    videobuf_sub = (u16*)BG_BMP_RAM(0);
    videobuf_sub_stats = (u16*)BG_BMP_RAM(8);

    int w = RGB15(31, 31, 31) | BIT(15);
    memset(videobuf_main, w, 256*192*2);
    memset(videobuf_sub, w, 256*192*2);
    memset(videobuf_sub_stats, 0, 256*192*2);
}


void NDSRS::ScreenShot() {
    u16* temp = new u16[256 * 192 * 2];
    memcpy(temp, screen_top, 256 * 192 * sizeof(u16));
    memcpy(temp + 256 * 192, screen_btm, 256 * 192 * sizeof(u16));

    PNGSaveImage(temp, 256, 192 * 2);

    delete[] temp;
}

void NDSRS::DrawScreen(SCREEN screen)
{
    if (screen & TOP)
        swiCopy(screen_top, videobuf_main, 256 * 192 / 2 | COPY_MODE_WORD);
    if (screen & BOTTOM)
        swiCopy(screen_btm, videobuf_sub, 256 * 192 / 2 | COPY_MODE_WORD);
    if (screen & STAT)
        swiCopy(screen_sta, videobuf_sub_stats, 256 * 192 / 2 | COPY_MODE_WORD);
}


void NDSRS::ClearScreen(SCREEN screen)
{
    u16 w = RGB15(31, 31, 31) | BIT(15);
    if (screen & TOP)
        memset(screen_top, w, sizeof(screen_top));
    if (screen & BOTTOM)
        memset(screen_btm, w, sizeof(screen_btm));
    if (screen & STAT)
        memset(screen_sta, 0, sizeof(screen_sta));
}


void NDSRS::ClearRect(SCREEN screen, int x1, int y1, int x2, int y2)
{
    if (screen & TOP) {
        for(int i = x1; i < x2; ++i) {
            for(int n = y1; n < y2; ++n) {
                screen_top[n*SCREEN_WIDTH + i] = RGB15(31, 31, 31) | BIT(15);
            }
        }
    }
    if (screen & BOTTOM) {
        for(int i = x1; i < x2; ++i) {
            for(int n = y1; n < y2; ++n) {
                screen_btm[n*SCREEN_WIDTH + i] = RGB15(31, 31, 31) | BIT(15);
            }
        }
    }
}
