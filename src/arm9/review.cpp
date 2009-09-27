#include "review.h"

#include "png.h"

#include <malloc.h>
#include <math.h>

Review::Review(Preferences* p)
{
    VideoInit();
    
    running = true;
    tindex = -1;
    score = 1;
    display = false;
    txthinting = true;
    answercache = NULL;
    questioncache = NULL;
    prefs = p;
    oldx = oldy = 0;
    
    ClearScreen(BOTH);
    
    /*text = new Text("default.ttf");
    text->SetPixelSize(prefs->topfontsize);
    text->SetScreen(screen_top);
    text->SetPen(6, 16);
    text->SetMargins(6, 6, 10, 16);
    text->SetInvert(true);*/
    fcache = new FontCache("default.ttf");
    text = new Text(fcache);
    text->SetBuffer(256, 192);
    text->SetMargins(6,0,0,0);
    text->SetColor(RGB15(0,0,0));
    text->SetFontSize(prefs->topfontsize);
        
    LoadTodo();
    //Shuffle();
    
    NextCard();
    DrawTop();
    DrawBottom();
    DisplayStats();
}



Review::~Review()
{
    delete text;
    delete fcache;
    for(int i = 0; i < prefs->todo; ++i)
        delete todo[i];
    delete[] todo;
    
    if (questioncache) {
        delete[] questioncache;
        questioncache = NULL;
    }
    if (answercache) {
        delete[] answercache;
        answercache = NULL;
    }
}

void Review::DisplayQuestion()
{
    if (questioncache) {
        memcpy(screen_top, questioncache, sizeof(screen_top));
    }
    else {
        text->ClearBuffer();
        text->SetFontSize(prefs->topfontsize);
        text->SetPen(6, prefs->topfontsize);
        
        text->PrintStringWrap(todo[tindex]->Question());
        
        text->BlitToScreen(screen_top);
        
        questioncache = new u16[192*256];
        memcpy(questioncache, screen_top, sizeof(screen_top));
    }
}

void Review::DisplayAnswer()
{        
    if (answercache)
        memcpy(screen_btm, answercache, sizeof(screen_btm));
    else {
        text->ClearBuffer();
        text->SetFontSize(prefs->bottomfontsize);
        text->SetPen(6, prefs->bottomfontsize);
        text->PrintStringWrap(todo[tindex]->Answer());
        
        text->BlitToScreen(screen_btm);
        
        answercache = new u16[192*256];
        memcpy(answercache, screen_btm, sizeof(screen_btm));
    }
}

void Review::DisplayStats()
{
    text->ClearBuffer();
    
    text->SetFontSize(8);
    
    char buf[20];

    sprintf(buf, "%d/%d", tindex+1, prefs->todo);
    text->SetPen(6,186);
    text->PrintString(buf);

    sprintf(buf, "Score: %d", score);
    text->SetPen(220,186);
    text->PrintString(buf);
    
    text->BlitToScreen(screen_btm);
}

void Review::DrawTop()
{
    if (running == false)
        return;
    ClearScreen(TOP);
    DisplayQuestion();
    
    // debuggin`
    struct mallinfo info = mallinfo();
    char buf[128];
    sprintf(buf, "Memory: %d/%d\n", info.usmblks + info.uordblks, info.arena);
    
    text->ClearBuffer();
    text->SetFontSize(8);
    text->SetPen(6,186);
    text->PrintString(buf);
    text->BlitToScreen(screen_top);
    
    DrawScreen(TOP);
}

void Review::DrawBottom()
{
    if (running == false)
        return;
    ClearScreen(BOTTOM);
    if (display)
        DisplayAnswer();
    
    DisplayStats();
    DrawScreen(BOTTOM);
}

void Review::Shuffle()
{
    Card* temp;
    int r;
    for(int i = 0; i < prefs->todo; ++i) {
        r = rand() % prefs->todo;
        temp = todo[i];
        todo[i] = todo[r];
        todo[r] = temp;
    }
}


void Review::LoadTodo()
{
    int ti = 0;
    todo = new Card*[prefs->todo];
    memset(todo, 0, sizeof(Card*)*prefs->todo);
    FILE* srs = fopen(prefs->srs, "r");
    
    char* sbuf = new char[1024*8]; // well if its more than 8k...
 
 
    // this looks slow, fix sometime maybe
    fakelong id;
    while (true) {
        readline(srs, sbuf);
        if (sbuf[0] == 0)
            break;
        if (ti >= prefs->todo)
            break;
        
        memset(id, 0, sizeof(fakelong));
        sscanf(sbuf, "%s\t", id);
        for(int i = 0; i < prefs->todo; ++i) {
            if (!strncmp(prefs->todolist[i], id, 32)) {
                todo[ti++] = new Card(sbuf);
                break;
            }
        }
    }
    
    delete[] sbuf;
    fclose(srs);
}

void Review::NextCard()
{
    tindex++;
    score = 1;
    
    display = false;
    
    if (questioncache) {
        delete[] questioncache;
        questioncache = NULL;
    }
    if (answercache) {
        delete[] answercache;
        answercache = NULL;
    }
    
    if (tindex >= prefs->todo) {
        tindex = 0;
        running = false;
    }
}

void Review::WriteAnswer()
{
    FILE* repfile = fopen(prefs->rep, "a");
        
    char buf[128];
    int l = sprintf(buf, "%s:%d:%d\n", todo[tindex]->ID(), score, todo[tindex]->Reps());
    fwrite(buf, 1, l, repfile);
    
    fclose(repfile);
}

/*#define TOUCH_R   0x0001
#define TOUCH_RU  0x0002
#define TOUCH_U  0x0004
#define TOUCH_LU  0x0008
#define TOUCH_L  0x0010
#define TOUCH_LD 0x00020
#define TOUCH_D   0x0040
#define TOUCH_RD  0x0080
#define TOUCH_CR  0x0100
#define TOUCH_CL 0x0200
int Touch_Down;*/


void Review::TouchScreen()
{
    static int tp_thresh = 256;
    int down = keysDown();
    int held = keysHeld();
    int up = keysUp();
    
    if (down & (KEY_L|KEY_R)) {
        ClearScreen(STAT);
        DrawScreen(STAT);
    }
    if (down & KEY_TOUCH) {
        tp_thresh = 30;
        touchPosition touch;
        touchRead(&touch);
        
        oldx = touch.px;
        oldy = touch.py;
        
        //if (oldx<32 || oldy<32){
        //        Touch_Down =(TOUCH_LD);
        //}//256*192
        
        
        
    }
    if (held & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);
        
        if (touch.px < 12 && touch.py < 12) {
            ClearScreen(STAT);
            DrawScreen(STAT);
        }
        
        int d = sqrt(((oldx -touch.px)*(oldx-touch.px)) + ((oldy -touch.py)*(oldy-touch.py)));
        
        if (d < tp_thresh) {
            DrawLine(touch.px, touch.py, oldx, oldy);
            oldx = touch.px;
            oldy = touch.py;
        }
    }
    if (up & KEY_TOUCH) {
        tp_thresh = 256;
    }
}

void Review::Run()
{   

    int NextMask = 0;
    int LastMask;
    while(running) {
        scanKeys();
        int down = keysDown();
        int held = keysHeld();      
        
     if (!display) {
        if (down & (KEY_A|KEY_B|KEY_X|KEY_Y)){
                display = true;
                DrawBottom();
                }
     }else{
           if (down & (KEY_A)) { // sets score to 3 
                if (display) {
                    score = 3;
                    WriteAnswer();
                    NextCard();
                    if (running) {
                        DrawTop();
                        DrawBottom(); 
                    }
                }    
            }      
            
            if (down & (KEY_Y)) { // sets score to 0
                if (display) {
                    score = 0;
                    WriteAnswer();
                    NextCard();
                    if (running) {
                        DrawTop();
                        DrawBottom(); 
                    }
                }
            }    
            
            if (down & (KEY_X)) { // sets score to 4
                if (display) {
                    score = 4;
                    WriteAnswer();
                    NextCard();
                    if (running) {
                        DrawTop();
                        DrawBottom(); 
                    }
                }
            }  
            
            if (down & (KEY_B)) { // sets score to 2
                if (display) {
                    score = 2;
                    WriteAnswer();
                    NextCard();
                    if (running) {
                        DrawTop();
                        DrawBottom(); 
                    }
                }
            } 
      }                                                  
                
            if (down & KEY_START) {
                running = false;
            }

            if (down & KEY_SELECT) {
                fifoSendValue32(FIFO_BACKLIGHT, 0);
            }
      
            if (held == (KEY_L|KEY_R|KEY_X)) {
                ScreenShot();
            }

        TouchScreen();  
        swiWaitForVBlank();
    }
}


// from some tutorial
void Review::DrawLine(int x1, int y1, int x2, int y2)
{
    int yStep = SCREEN_WIDTH;
    int xStep = 1;
    int xDiff = x2 - x1;
    int yDiff = y2 - y1;
    int color = RGB15(0,0,0) | BIT(15);
 
    int errorTerm = 0;
    int offset = y1 * SCREEN_WIDTH + x1;
    int i;
    
    if (yDiff < 0) {
        yDiff = -yDiff;
        yStep = -yStep;
    }
    
    if (xDiff < 0) {
        xDiff = -xDiff;
        xStep = -xStep;
    }
 
    if (xDiff > yDiff) {
        for (i = 0; i < xDiff + 1; i++) {
            for(int x = -1; x < 1; ++x) {
                for(int y = -1; y < 1; ++y) {
                    int nx = offset%256 + x;
                    int ny = offset/256 + y;
                    videobuf_sub_stats[nx + ny * SCREEN_WIDTH] = color;
                }
            }
            offset += xStep;
            errorTerm += yDiff;
 
            if (errorTerm > xDiff) {
                errorTerm -= xDiff;
                offset += yStep;
          }
       }
    }

    else  {
        for(i = 0; i < yDiff + 1; i++)  {
            for(int x = -1; x < 1; ++x) {
                for(int y = -1; y < 1; ++y) {
                    int nx = offset%256 + x;
                    int ny = offset/256 + y;
                    videobuf_sub_stats[nx + ny * SCREEN_WIDTH] = color;
                }
            }
            offset += yStep;
            errorTerm += xDiff;

            if (errorTerm > yDiff) {
               errorTerm -= yDiff;
               offset += xStep;
            }
        }
    }
}
