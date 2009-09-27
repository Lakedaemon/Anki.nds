#ifndef _text_h
#define _text_h

#include <string>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_BITMAP_H
#include FT_GLYPH_H

/* define custom face identification structure */
typedef struct MyFaceRec_ {
    const char* file_path;
    int         face_index;
    int         cmap_index;
} MyFaceRec, *MyFace;

#define FONT "default.ttf"
#define LINESPACING 0
#define PARASPACING 0
#define PAGE_HEIGHT SCREEN_WIDTH
#define PAGE_WIDTH SCREEN_HEIGHT
#define PIXELSIZE 12

class Text {
    
    FT_Error error;
    FT_Library library;
    MyFace font;
    FTC_ScalerRec scaler;
        
    FTC_Manager manager;
    FTC_CMapCache cmapCache;
    FTC_SBitCache sbitCache;
    FTC_SBit sbit;

    int penX, penY;
    u16 colormod;

    bool hinting;
    int hintingarg;
    int width_max, height_max;

    char fileName[128];
    u16* screen;
    u8 pixelsize;
    bool invert;
    bool justify;
    
    int marginLeft, marginRight, marginTop, marginBottom;
    
public:
    Text(const char*);
    //Text();
    ~Text();
    void InitDefault();
    void InitPen();

    u8   GetAdvance(u16 code);
    u8   GetCharCode(const char *txt, u16 *code);
    u8   GetHeight();
    bool GetInvert();
    void GetPen(int* x, int* y);
    int  GetPenX();
    int  GetPenY();
    u8   GetPixelSize();
    u16* GetScreen();
    int  GetStringWidth(const char *txt);

    void SetColorMod(u16);
    void SetInvert(bool);
    void SetPen(int x, int y);
    void SetPenX(int x);
    void SetPenY(int y);
    void SetPixelSize(u8);
    void SetScreen(u16 *s);
    void SetMargins(int left, int right, int top, int bottom);
    void SetHinting(bool);
    void SetBounds(int, int);

    int CacheGlyph(u16 codepoint);
    void GetGlyph(u16 ucs, FTC_SBit* sbit);
    void ClearCache();

    void ClearRect(u16 xl, u16 yl, u16 xh, u16 yh);
    void ClearScreen();

    int WrapString(const char* string, bool draw);
    int PrintStringWrapLines(const char* string);

    void PrintChar(u16 code);
    bool PrintNewLine(void);
    void PrintStatusMessage(const char *msg);
    void PrintString(const char *string);
    void PrintStringWrap(const char* string);
};

#endif

