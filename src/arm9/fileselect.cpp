#include "fileselect.h"

#include <dswifi9.h>
/*
extern "C" {
	#include "sha1/sha1.h"
	#include "md5/global.h"
	#include "md5/md5.h"
}*/
#include <openssl/ssl.h>


//#include <sys/types.h>//useful ?
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

FileSelect::FileSelect(Preferences* p)
{
    VideoInit();
    
    prefs = p;
    
    fcache = new FontCache("default.ttf");
    text = new Text(fcache);
    //text->SetFont("default.ttf");
    text->SetBuffer(256,192);
    text->SetMargins(6,0,0,0);
    text->SetColor(RGB15(0,0,0));
    text->SetFontSize(12);
    text->PrintString("loading...");
    
    /*text = new Text("default.ttf");
    text->SetPixelSize(12);
    text->SetScreen(videobuf_main);
    text->SetPen(6, 16);
    text->SetMargins(6, 6, 10, 16);
    text->SetInvert(true);
    text->PrintString("loading...");*/
    
    struct stat st;
    char filename[MAXPATHLEN];
    DIR_ITER* dir;

    memset(&files, 0, sizeof(FOption)*MAXSRS);

    dir = diropen(".");

    int findex = 0;

    if (dir == NULL) {
        strncpy(files[findex].display, "dir error!", 128);
        ++findex;
    }
    else {
        while (dirnext(dir, filename, &st) == 0) {
            int p = strlen(filename)-4;
            if (stricmp(filename+p, ".srs") != 0)
                continue;
            
            strncpy(files[findex].display, filename, p);
            strncpy(files[findex].srs, filename, 128);
            snprintf(files[findex].rep, 128, "%s.rep", files[findex].display);
            makeifnotexist(files[findex].rep);
            
            files[findex].total = -1;
            files[findex].todo = -1;
            
            ++findex;
        }
    }
    if (findex == 0) {
        strcpy(files[0].display, "no files");
        files[0].total = 0;
        files[0].todo = 0;
        findex++;
    }
        
    dirclose(dir);

    selected = 0;
    filesmax = findex;
    running = true;
    
    ShowFileStats();
    DrawTop();
}


FileSelect::~FileSelect()
{
    delete text;
    delete fcache;
    for(int i = 0; i < filesmax; ++i)
        delete[] files[i].todolist;
}

void FileSelect::ShowFileStats()
{  
    files[selected].total = TotalCards();
    files[selected].todo = NumberCardsDue();
   
    ClearScreen(BOTTOM);
    text->ClearBuffer();
    
    char buf[256];
    text->SetPen(6, 16);
    sprintf(buf, "Total: %d", files[selected].total);
    text->PrintString(buf);
    text->PrintNewline();
    sprintf(buf, "Todo: %d", files[selected].todo);
    text->PrintString(buf);
    text->PrintNewline();
    
    text->BlitToScreen(screen_btm);
    
    /*text->PrintNewLine();
    text->PrintNewLine();
    
    time_t ut = time(NULL);
    tm* z = localtime(&ut);
    char d[12];
    memset(d, 0, 12);
    sprintf(d, "%d-%.2d-%.2d(%d)", 1900+ z->tm_year, z->tm_mon+1, z->tm_mday, ut);
    text->PrintString(d);
    text->PrintNewLine();
    sprintf(d, "%d:%d:%d",  z->tm_hour, z->tm_min, z->tm_sec);
    text->PrintString(d);*/
    
    DrawScreen(BOTTOM);    
}

int FileSelect::TotalCards()
{
    if (files[selected].total != -1)
        return files[selected].total;
    FILE* f = fopen(files[selected].srs, "r");
    int t = 1;
    while (true) {
        int c = fgetc(f);
        if (c == 0x0A)
            t++;
        if (c == EOF)
            break;
    }
    files[selected].total = t;
    fclose(f);
    return t;
}

int FileSelect::NumberCardsDue()
{
    if (files[selected].todo != -1)
        return files[selected].todo;
    int now = time(NULL) + (-1*60*60*prefs->timezone);
    int count = 0;
    FILE* srs = fopen(files[selected].srs, "r");
    FILE* rep = fopen(files[selected].rep, "r");
    
    int si = 0, ri = 0;
    fakelong* slist = new fakelong[TotalCards()];
    fakelong* rlist = new fakelong[TotalCards()];
    char* buf = new char[1024*2];
    files[selected].todolist = new fakelong[TotalCards()];
    
    memset(slist, 0, sizeof(fakelong)*TotalCards());
    memset(rlist, 0, sizeof(fakelong)*TotalCards());
    memset(buf, 0, 1024*2);
    memset(files[selected].todolist, 0, sizeof(fakelong)*TotalCards());
    
    while (true) {
        readline(srs, buf);
        if (buf[0] == 0)
            break;
        
        fakelong id;
        memset(id, 0, sizeof(fakelong));
        int time;
        sscanf(buf, "%s\t%d\t", id, &time);

        if (now > time)
            strncpy(slist[si++], id, 32);
    }
    while (true) {
        readline(rep, buf);
        if (buf[0] == 0)
            break;
        
        fakelong id;
        memset(id, 0, sizeof(fakelong));
        sscanf(buf, "%[^:]:", id);
        
        strncpy(rlist[ri++], id, 32);
    }
    for(int i = 0; i < si; ++i) {
        bool keep = true;
        for(int n = 0; n < ri; ++n) {
            if (!strncmp(slist[i], rlist[n], 32)) {
                keep = false;
                break;
            }
        }
        if (keep) {
            strncpy(files[selected].todolist[count++], slist[i], 32);
        }
    }    
    
    delete[] buf;
    delete[] rlist;
    delete[] slist;
    fclose(srs);
    fclose(rep);
    
    files[selected].todo = count;
    return count;
}

void FileSelect::PrepareReview()
{
    strncpy(prefs->srs, files[selected].srs, 128);
    strncpy(prefs->rep, files[selected].rep, 128);
    
    prefs->total = files[selected].total;
    prefs->todo = files[selected].todo;
    prefs->syncloop = 0;
    
    prefs->todolist = new fakelong[prefs->todo];
    for(int i = 0; i < prefs->todo; ++i) {
        memset(prefs->todolist[i], 0, sizeof(fakelong));
        strncpy(prefs->todolist[i], files[selected].todolist[i], 32);
    }
}

void FileSelect::DrawTop()
{
    ClearScreen(TOP);
    text->ClearBuffer();
    text->SetPen(6, 16);
    for(int i = 0; i < filesmax; ++i) {
        if (i == selected)
            text->PrintString("* ");
        text->PrintString(files[i].display);
        text->SetPen(6, 16+(i+1)*(text->GetLineHeight()));
    }
    
    text->BlitToScreen(screen_top);
    DrawScreen(TOP);
}


bool FileSelect::SyncToAnki()
{
    ClearScreen(BOTTOM);
    text->ClearBuffer();
    
    u16 w = RGB15(31, 31, 31) | BIT(15);
    memset(videobuf_sub, w, sizeof(screen_btm));
    
    text->SetPen(6, 14);
    text->SetFontSize(12);
    text->PrintString("Initializing wifi...\n");
    text->BlitToScreen(videobuf_sub);
    text->ClearBuffer();
    swiWaitForVBlank();
    
    int wifiStatus = ASSOCSTATUS_DISCONNECTED;
    Wifi_EnableWifi(); 
    if (prefs->AutoConnect)
    {
            Wifi_AutoConnect();
    }else{ 



            char Temp[100];            
            Wifi_AccessPoint ap;
            Wifi_AccessPoint BestAp;            
            int OldMaxRssi ;
            int MaxRssi = -1;
            int MaxTries = 30;

            while ((MaxRssi < 60 || OldMaxRssi<MaxRssi) || MaxTries >0){
                    Wifi_ScanMode(); // scans the surroundings for wifi Access point -> puts the info in an intern list
                    swiWaitForVBlank();
                    
                    OldMaxRssi = MaxRssi;                     
                    for (int i=0;i<Wifi_GetNumAP(); i++) {
                                if (Wifi_GetAPData(i,&ap)==WIFI_RETURN_OK && ap.rssi > MaxRssi &&  !strcmp(ap.ssid,prefs->SSID) ) {
                                        MaxRssi = ap.rssi;
                                        Wifi_GetAPData(i,&BestAp);
                                }      
                        }
                    MaxTries-= 1;
            }
            if (MaxRssi<0){
                sprintf(Temp,"Can't find any %s access point",prefs->SSID);
                text->PrintString(Temp);
                text->BlitToScreen(videobuf_sub);
                text->ClearBuffer();
                swiWaitForVBlank();
                waitForAnyKey();
                return false;                                
            }
            
            sprintf(Temp,"Connecting to %s (%2i%%): ",BestAp.ssid,(BestAp.rssi*100)/0xD0);
            text->PrintString(Temp);
            text->BlitToScreen(videobuf_sub);
            text->ClearBuffer();
            swiWaitForVBlank();
    
            Wifi_SetIP(0,0,0,0,0);
            int wepkeyid=0;
            int wepmode=0;
            unsigned char wepkeys[4][32];
            Wifi_ConnectAP(&BestAp,wepmode,wepkeyid,wepkeys[0]);

    
    
    
            while(wifiStatus != ASSOCSTATUS_ASSOCIATED && ASSOCSTATUS_CANNOTCONNECT != wifiStatus) {
                    wifiStatus = Wifi_AssocStatus();
                    scanKeys();
                    int down = keysDown();
                    if (down & KEY_START) {
                            return false;
                    }
                    swiWaitForVBlank();
            }
            if (wifiStatus == ASSOCSTATUS_CANNOTCONNECT) {
                    text->PrintString("fail\n");
                    text->BlitToScreen(videobuf_sub);
                    text->ClearBuffer();
                    waitForAnyKey();
                    return false;
            }
            text->PrintString("success\n");
            text->BlitToScreen(videobuf_sub);
            text->ClearBuffer();
            swiWaitForVBlank();  

            SSL_METHOD*  method  = 0;
            SSL_CTX*     ctx     = 0;
            SSL*         ssl     = 0;
            int passSock;
            sockaddr_in addr;
            
            char Content[512] ;
            memset(Content,0,sizeof(Content));
            char Received[1024];
            char Buffer[4096];
            int i;
            
            if (prefs->LookFor){
                    
                    method  = TLSv1_client_method();       
                    ctx     = SSL_CTX_new(method);
                    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
                    ssl = SSL_new(ctx);
                    passSock = socket(AF_INET, SOCK_STREAM, 0);
                    memset(&addr, 0, sizeof(addr)); 
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(443);
                    addr.sin_addr.s_addr =inet_addr((const char*) prefs->WebPortalIp);
                    connect(passSock, (sockaddr*)&addr, sizeof(addr));
                    SSL_set_fd(ssl, passSock);
            
                    if (SSL_connect(ssl) != SSL_SUCCESS) {
                            text->PrintString("SSL connection failed");
                            text->BlitToScreen(videobuf_sub);
                            text->ClearBuffer();
                            swiWaitForVBlank(); 
                            waitForAnyKey();	
                            return false;
                    }

                    char Get[256] ;
                    sprintf(Get,"GET / HTTP/1.1\r\n"
                            "Host: %s\r\n"
                            "Accept: text/html\r\n"
                            "User-Agent: Nintendo DS\r\n\r\n",prefs->Host);
        
                    if (SSL_write(ssl, Get, strlen(Get)) != strlen(Get))
                    {
                            text->PrintString("SSL write failed");
                            text->BlitToScreen(videobuf_sub);
                            text->ClearBuffer();
                            swiWaitForVBlank(); 
                            waitForAnyKey();	
                            return false;
                    }
     

                    memset(Buffer,0,sizeof(Buffer));
                    i=0;

                    while (!strstr(Buffer,"</HTML>") && !strstr(Buffer,"</html>")){
                            int Read = SSL_read(ssl, Received, 250);
                            int j=0;
                            if (Read==0) {
                                    text->PrintString("Socket deconnected");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            Received[Read]=0;
                            /*ClearScreen(BOTTOM);
                            text->ClearBuffer();
                            text->SetPen(6, 16);
                            text->PrintString(Received);
                            text->BlitToScreen(screen_btm);
                            text->ClearBuffer();
                            DrawScreen(BOTTOM);
                            swiWaitForVBlank(); 
                            waitForAnyKey();*/
                            while (Read>0){
                                    Buffer[i]=Received[j];
                                    i++;
                                    j++;
                                    Read--;
                            }
                        
                    }
                    Buffer[i] = 0;
                    


                    char * PointerA;
                    char * PointerB = prefs->LookFor- strlen("<--End-->");  
                    char * PointerC;
                    char * PointerD;
                    while (PointerB + strlen("<--End-->") < prefs->LookFor + strlen(prefs->LookFor)){
                            PointerA = PointerB + strlen("<--End-->");
                            PointerB = strstr(PointerA,":");
                            if (PointerB==0) {
                                    text->PrintString("Bad field in Anki.conf, missing \":\"");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            *PointerB=0;
                            strcat(Content,PointerA);
                            char Temp[128] ;
                            sprintf(Temp,"No \"%s\" inside HTML welcome page\n", PointerA);
                            PointerA = strstr(PointerB+1,"<--Start-->");
                            if (!PointerA) {
                                    text->PrintString("Bad field in Anki.conf, missing \"<--Start-->\"");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            PointerB = strstr(PointerA,"<--Value-->");
                            if (!PointerB) {
                                    text->PrintString("Bad field in Anki.conf, missing \"<--Value-->\"");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            PointerA += strlen("<--Start-->");
                            *PointerB = 0;                         
                            PointerC = strstr(Buffer,PointerA);
                            if (!PointerC) {
                                    text->PrintString(Temp);
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            PointerC += strlen(PointerA);
                            PointerA = PointerB + strlen("<--Value-->");
                            PointerB = strstr(PointerA,"<--End-->");
                            if (!PointerB) {
                                    text->PrintString("Bad field in Anki.conf, missing \"<--End-->\"");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            *PointerB=0;
                            PointerD = strstr(PointerC,PointerA);                            
                            if (!PointerD) {
                                    text->PrintString(Temp);
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            char Swap = *PointerD;
                            *PointerD=0;
                            strcat(Content,"=");
                            strcat(Content,PointerC);
                            strcat(Content,"&");                            
                            *PointerD=Swap;
                    }
                    SSL_CTX_free(ctx); 
                    SSL_free(ssl);
            }
            
            method  = 0;
            ctx     = 0;
            ssl     = 0;
            method  = TLSv1_client_method();        
            ctx     = SSL_CTX_new(method);   
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
            ssl = SSL_new(ctx);     
            passSock = socket(AF_INET, SOCK_STREAM, 0);
            memset(&addr, 0, sizeof(addr)); 
            addr.sin_family = AF_INET;
            addr.sin_port = htons(443);
            addr.sin_addr.s_addr =inet_addr((const char*)prefs->WebPortalIp);
            connect(passSock, (sockaddr*)&addr, sizeof(addr));	
            SSL_set_fd(ssl, passSock);
    	        
            if (SSL_connect(ssl) != SSL_SUCCESS) {
                    text->PrintString("Second SSL connection failed");
                    text->BlitToScreen(videobuf_sub);
                    text->ClearBuffer();
                    swiWaitForVBlank(); 
                    waitForAnyKey();	
                    return false;
            }         

            strcat(Content,prefs->Post);
        	    Content[strlen(Content)-1] = 0;

            char Header[512];
            sprintf(Header,"POST / HTTP/1.1\r\n"
                                        "Host: %s\r\n"
                                        "Content-Length: %d\r\n"
                                        "Content-Type: application/x-www-form-urlencoded\r\n"
                                        "\r\n"
                                        "%s\r\n",prefs->Host, strlen(Content),Content);

            if (SSL_write(ssl, Header, strlen(Header)) != strlen(Header)){
                    text->PrintString("Authentification failed");
                    text->BlitToScreen(videobuf_sub);
                    text->ClearBuffer();
                    swiWaitForVBlank(); 
                    waitForAnyKey();
            }  
                
            text->PrintString("Sending crypted authentification\n");
            text->BlitToScreen(videobuf_sub);
            text->ClearBuffer();
            swiWaitForVBlank(); 
            
            if (!strcmp(prefs->Success,"Bad Success String, missing\"<--End-->\"") || !strcmp(prefs->Success,"No Success String")){  
                                    text->PrintString(prefs->Success);
                                    text->PrintString("\n");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
            }else{   
                    memset(Buffer,0,sizeof(Buffer));
                    i=0;
                    while (!strstr(Buffer,"</HTML>") && !strstr(Buffer,"</html>")){
                            int Read = SSL_read(ssl, Received, 250);
                            int j=0;
                            if (Read==0) {
                                    text->PrintString("Socket deconnected");
                                    text->BlitToScreen(videobuf_sub);
                                    text->ClearBuffer();
                                    swiWaitForVBlank(); 
                                    waitForAnyKey();
                                    return false;
                            }
                            Received[Read]=0;
                            while (Read>0){
                                    Buffer[i]=Received[j];
                                    i++;
                                    j++;
                                    Read--;
                            }             
                    }
                    Buffer[i] = 0;
        
               
        

                    char * Pointer = strstr(Buffer,prefs->Success);
                    if (!Pointer) {
                            text->PrintString("Failed\n");
                            text->PrintString(prefs->Success);
                            text->BlitToScreen(videobuf_sub);
                            text->ClearBuffer();
                            swiWaitForVBlank(); 
                            waitForAnyKey();
                            return false;
                    }

                    sprintf(Temp, "Connected to the %s network\n",prefs->SSID);
                    text->PrintString(Temp);
                    text->BlitToScreen(videobuf_sub);
                    text->ClearBuffer();
                    swiWaitForVBlank();
            }
        	   
        	   SSL_CTX_free(ctx); 
            SSL_free(ssl);  
    }
   
    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_port = htons(24550);
    inet_aton(prefs->ip, &sain.sin_addr);
    text->PrintString("Connecting to anki : ");
    text->BlitToScreen(videobuf_sub);
    text->ClearBuffer();
    swiWaitForVBlank();
    
    int e;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    e = connect(fd, (struct sockaddr*) &sain, sizeof(sain));
    if (e < 0) {
        text->PrintString("failed\n");
        text->BlitToScreen(videobuf_sub);
        text->ClearBuffer();
        waitForAnyKey();
        return false;
    }
    
    text->PrintString("success\n");   
    int len;
    int res;
    res = recv(fd, &len, 4, 0);
    if (res ==0) {
            text->PrintString("Connection closed by Anki");            
            text->BlitToScreen(videobuf_sub);
            text->ClearBuffer();
            swiWaitForVBlank();
            waitForAnyKey();
            return false;            
    }
    text->PrintString("Syncing the deck \"");
    text->BlitToScreen(videobuf_sub);
    text->ClearBuffer();
    swiWaitForVBlank();
    char* name = new char[len+1];
    memset(name, 0, len+1);
    recv(fd, name, len, 0);  
    text->PrintString(name);
    text->PrintString("\"\n");
    text->BlitToScreen(videobuf_sub);
    text->ClearBuffer();
    swiWaitForVBlank();
    
    
    char srs[128];
    char rep[128];
    sprintf(srs, "%s.srs", name);
    sprintf(rep, "%s.rep", name);
    delete[] name;
    
    FILE* frep = fopen(rep, "r");
    if (frep == NULL) {
        len = -1;
        send(fd, &len, 4, 0);
        text->PrintString("No rep file\n");
    }
    else {
        len = flen(frep);
        if (len == 0) {
            len = -1;
            send(fd, &len, 4, 0);
            text->PrintString("Empty rep file\n");
        }
        else {
            text->PrintString("Sending rep file");
            text->BlitToScreen(videobuf_sub);
            text->ClearBuffer();
             swiWaitForVBlank();
            char* rbuf = new char[len+1];
            fread(rbuf, 1, len, frep);   
            send(fd, &len, 4, 0);  
            int s = 0;
            int status = 0;
            int step = len / 10 + 1;
            while (s < len) {
                s += send(fd, rbuf+s, len-s, 0);
                while (s > status + step){
                     text->PrintString(".");  
                     text->BlitToScreen(videobuf_sub);
                     text->ClearBuffer();
                     status += step;
                }
                swiWaitForVBlank();
                
            }

            delete[] rbuf;
            char temp[50];
            sprintf(temp, "\n%d bytes sent\n", len);
            text->PrintString(temp);
        }
        fclose(frep);
        unlink(rep);
    }
    text->PrintString("Getting srs file");
    text->BlitToScreen(videobuf_sub);
    text->ClearBuffer();
    swiWaitForVBlank();
    
    FILE* fsrs = fopen(srs, "w"); 

    char* sbuf = new char[32*1024]; 
    memset(sbuf, 0, 32*1024);
    
    recv(fd, &len, 4, 0); 
    char temp[20];
    sprintf(temp, " (%d bytes)", len);
    text->PrintString(temp);
    int r = 0;
    int status = 0;
    int step = len / 10 + 1;
    int tr = 1;

    while(r < len and tr!=0) {
        tr = recv(fd, sbuf, 32*1024, 0); 
        fwrite(sbuf, 1, tr, fsrs);
        r += tr;
        while (r > status + step){
             text->PrintString(".");  
             text->BlitToScreen(videobuf_sub);
             text->ClearBuffer();
             status += step;
        }
        swiWaitForVBlank();
    }
    if (tr==0){
       text->PrintString("\nConnection Broken");
       text->BlitToScreen(videobuf_sub);
       text->ClearBuffer();
       swiWaitForVBlank();  
       waitForAnyKey();
       return false;
    }
    text->PrintString("\ndone");
    text->BlitToScreen(videobuf_sub);
    text->ClearBuffer();
    swiWaitForVBlank();
    
    delete[] sbuf;
    fclose(fsrs);
    shutdown(fd,0); 
    closesocket(fd); 
    makeifnotexist(rep);
    
    return true;

}

void FileSelect::Run()
{
    while (running) {
        scanKeys();
        int down = keysDown();
        int held = keysHeld();
        
        if (down & KEY_UP) {
            --selected;
            selected = MAX(selected, 0);
            DrawTop();
            ShowFileStats();
        }
        if (down & KEY_DOWN) {
            ++selected;
            selected = MIN(selected, filesmax-1);
            DrawTop();
            ShowFileStats();
        }
        if (down & KEY_LEFT) {
            selected = 0;
            DrawTop();
            ShowFileStats();
        }
        if (down & KEY_RIGHT) {
            selected = filesmax -1;
            DrawTop();
            ShowFileStats();
        }
        if (down & (KEY_B|KEY_A)) {
            if (files[selected].todo != 0) {
                PrepareReview();
                running = false;
            }
        }
        if (held == (KEY_L|KEY_R|KEY_X)) {
            ScreenShot();
        }
    
        if (down & KEY_SELECT) {
            fifoSendValue32(FIFO_BACKLIGHT, 0);
        }
        if (down & KEY_START) {
            bool b = SyncToAnki();
            Wifi_DisconnectAP();
            Wifi_DisableWifi();
            if (b) {
                prefs->syncloop = 1;
                running = false;
            }
            else
                ShowFileStats();
        }
        
        swiWaitForVBlank();
    }
}


