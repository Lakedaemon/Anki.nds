#include <nds.h>
#include <fat.h>

#include "text.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


/* our custom face requester is dead simple */
static FT_Error
simpleFaceRequester(FTC_FaceID face_id, FT_Library library, FT_Pointer request_data, FT_Face* aface ) {
    FT_Error error;

    MyFace face = (MyFace)face_id;
    error = FT_New_Face(library, face->file_path, face->face_index, aface);
    FT_Select_Charmap(*aface, FT_ENCODING_UNICODE);
    face->cmap_index = ((*aface)->charmap ? FT_Get_Charmap_Index((*aface)->charmap) : 0);

    return error;
}

Text::Text(const char* name) {
//Text::Text() {
    penX = penY = 0;    
    colormod = 0xFFFF;
    
    hinting = false;
    hintingarg = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL | FT_LOAD_NO_AUTOHINT;
    
    //strcpy(fileName, FONT);
    strcpy(fileName, name);
    font = NULL;
    screen = NULL;
    pixelsize = 0;
    invert = false;
    justify = false;
    width_max = SCREEN_WIDTH;
    height_max = SCREEN_HEIGHT;
    SetMargins(0, 0, 0, 0);
    
    //Error checking is pretty useless here. If this doesn't work, there's no chance
    //of recovery. Maybe in the future if we implement a logging system it'll become
    //useful again, but right now there's no reason to bother.
    
    FT_Init_FreeType(&library);
    FTC_Manager_New(library, 0, 0, 256 * 1024, &simpleFaceRequester, NULL, &manager);
    FTC_SBitCache_New(manager, &sbitCache);    
    FTC_CMapCache_New(manager, &cmapCache);

    //Set Font
    font = new MyFaceRec();
    font->file_path = fileName;
    font->face_index = 0;
    scaler.face_id = (FTC_FaceID)font;
    
    SetPixelSize(PIXELSIZE);

    InitPen();
}

Text::~Text() {
    scaler.face_id = NULL;
    if (font) {
        delete font;
    }

    FTC_Manager_Done(manager);
    FT_Done_FreeType(library);    
}

void Text::SetMargins(int left, int right, int top, int bottom) {
    marginLeft = left;
    marginRight = right;
    marginTop = top;
    marginBottom = bottom;
}

void Text::SetHinting(bool b)
{
    if (b)
        hintingarg = FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL;
    else
        hintingarg = FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL|FT_LOAD_NO_AUTOHINT;
}

void Text::SetBounds(int w, int h) {
    if (w != -1)
        width_max = w;
    if (h != -1)
        height_max = h;
}


void Text::GetGlyph(u16 ucs, FTC_SBit* sbit) {
    u32 index = FTC_CMapCache_Lookup(cmapCache, scaler.face_id, font->cmap_index, ucs);                                 
    FTC_SBitCache_LookupScaler(sbitCache, &scaler, hintingarg, index, sbit, NULL);
}

void Text::ClearCache() {
    FTC_Manager_Reset(manager);
}

int Text::GetStringWidth(const char* text) {
    int textL = strlen(text);
    int w = 0;
    for (int i = 0; i < textL; i++) {
        u16 c;
        int bytes = GetCharCode(text + i, &c);
        if (bytes > 0) {
            i += bytes - 1;
        }
        w += GetAdvance(c);
    }
    return w;
}


u8 Text::GetCharCode(const char *utf8, u16 *ucs) {
    // given a UTF-8 encoding, fill in the Unicode/UCS code point.
    // returns the bytelength of the encoding, for advancing
    // to the next character.
    // returns 0 if encoding could not be translated.
    // TODO - handle 4 byte encodings.

    if (utf8[0] < 0x80) { // ASCII
        *ucs = utf8[0];
        return 1;
    } else if (utf8[0] > 0xc1 && utf8[0] < 0xe0) { // latin
        *ucs = ((utf8[0]-192)*64) + (utf8[1]-128);
        return 2;

    } else if (utf8[0] > 0xdf && utf8[0] < 0xf0) { // asian
        *ucs = (utf8[0]-224)*4096 + (utf8[1]-128)*64 + (utf8[2]-128);
        return 3;

    } else if (utf8[0] > 0xef) { // rare
        return 4;

    }
    return 0;
}

u8 Text::GetHeight() {
    FT_Face face;
    FTC_Manager_LookupFace(manager, scaler.face_id, &face);
    return (face->size->metrics.height >> 6);
}

void Text::GetPen(int* x, int* y) {
    *x = penX;
    *y = penY;
}

void Text::SetPen(int x, int y) {
    penX = x;
    penY = y;
}

void Text::SetPenX(int x) {
    penX = x;
}

void Text::SetPenY(int y) {
    penY = y;
}

void Text::SetColorMod(u16 mod) {
    colormod = mod;
}

void Text::SetInvert(bool state) {
    invert = state;
}

bool Text::GetInvert() {
    return invert;
}

int Text::GetPenX() {
    return penX;
}

int Text::GetPenY() {
    return penY;
}

u8 Text::GetPixelSize()
{
    return pixelsize;
}

u16* Text::GetScreen()
{
    return screen;
}

void Text::SetPixelSize(u8 size) {
    if (size <= 0) {
        size = PIXELSIZE;
    }
    
    if (size != pixelsize) {
           ClearCache();

        pixelsize = size;
        scaler.width  = (FT_UInt)pixelsize;
        scaler.height = (FT_UInt)pixelsize;
        scaler.pixel  = 72;
        scaler.x_res  = 0;
        scaler.y_res  = 0;        
        
        //PrintStringWrap(" ");
    }
    
    //GetGlyph(' ', &sbit); //Update current font, or GetLineHeight() gives a wrong value
}

void Text::SetScreen(u16 *inscreen)
{
    screen = inscreen;
}

u8 Text::GetAdvance(u16 ucs) {
    GetGlyph(ucs, &sbit);
    return sbit->xadvance;
}

void Text::InitPen(void) {
    penX = marginLeft;
    penY = marginTop + GetHeight();
}


void Text::PrintChar(u16 ucs) {
    // Draw a character for the given UCS codepoint,
    // into the current screen buffer at the current pen position.

    // Consult the cache for glyph data and cache it on a miss
    // if space is available.
    
    //if (ucs == '|') {
    //    PrintNewLine();
    //    return;
    //}
    
    
    GetGlyph(ucs, &sbit);
    int tx = penX + sbit->xadvance;
    if (tx >= width_max) {
        //penX = tx;
        PrintNewLine();
        tx = marginLeft + sbit->xadvance;
        //return;
    }

    u8* bitmap = sbit->buffer;
    if (!bitmap) {
        penX = tx;       
        return;       
    }       
    
    int bx = sbit->left;
    int by = sbit->top;
    
    int t = (penY - by) * SCREEN_WIDTH;
    int u = 0;
    for (int gy = 0; gy < sbit->height; gy++) {
        for (int gx = 0; gx < sbit->width; gx++) {
            u16 a = (bitmap[u + gx] & 0xFF);
            if (a && screen) {
                int sx = (penX + gx + bx);
                int sy = (penY + gy - by);

                if (sx < 0 || sy < 0 || sx >= width_max || sy >= height_max) {
                    continue;
                }
                if (invert)
                    a = (255-a) >> 3;
                screen[t + sx] = RGB15(a, a, a) | BIT(15);
            }
        }
        t += SCREEN_WIDTH;
        u += sbit->width;
    }
    penX = tx;
}

bool Text::PrintNewLine(void) {
    penX = marginLeft;
    penY += GetHeight() + LINESPACING;
    return true;
}

//Draw a character string starting at the pen position.
void Text::PrintString(const char* string) {
    int stringL = strlen(string);
    for (int i = 0; i < stringL; i++) {
        u16 c = string[i];
        if (c == '\n' or c == '|') {
            PrintNewLine();
        } else {
            int bytes = GetCharCode(string + i, &c);
            if (bytes > 0) {
                i += bytes - 1;
                PrintChar(c);
            } else {
                i++;
            }            
        }
    }
}

//Warning: This function doesn't check for maxWidth < next_char_width and can consequently
//         fall into an infinite loop if that happens.
int Text::WrapString(const char* string, bool draw)
{
    if (string == NULL) {
        return -1;
    }
    
    if (strlen(string) == 0) {
        return -1;
    }
    
    const int maxWidth = SCREEN_WIDTH - marginLeft - marginRight - 1;
        
    int visibleChars = -1;
    int from = 0;
    int to = strlen(string);
    if (from == to) {
        return 1;
    }
    
    int spaceW = GetAdvance(' ');

    int line = 0;                    
    int lineWidth = penX - marginLeft;
    int lineHeight = 0;
    int wordCount = 0;
    int charCount = 0;        
    int lineDrawEnd = 0;

    int lastPenX = penX;
    int lastPenY = penY;
    int index = from;
    
    char tempString[to - from + 1];
    int tempIndex = 0;
    int lastFittingChar = 0; //Last char that fits on the current line
    bool allCharsDrawn = true;
    
    while (index < to) {        
        //Find next word start
        while (index < to && string[index] == ' ') { //Skip whitespace
            index++;
            charCount++;
        }

        int oldIndex = index;

        int ww = 0; //Word Width (includes optional prefixed space)
        int wh = 0; //Word Height
        bool wordFits = true;

        wh = GetHeight();

        if (index < to) {
            if (wordCount > 0) {
                ww = spaceW;
                tempString[tempIndex++] = ' ';
            }
                
            lastFittingChar = tempIndex; //Last char that fits on the current line
    
            //Add letters to word
            while (index < to && string[index] != ' ') {
                if (string[index] == '\n' or string[index] == '|') {
                    index++;
                    lastFittingChar = index;
                    wordFits = false;
                    break;
                }
                
                u16 c = string[index];
                int bytes = GetCharCode(string + index, &c);
                if (bytes == 0) bytes = 1;
                int advance = GetAdvance(c);
                
                if (lineWidth + ww + advance <= maxWidth) {
                    //Char still fits on this line
                    lastFittingChar += bytes;
                    
                } else if (ww + advance > maxWidth) {
                    //This single word is wider than an entire line. It needs to be
                    //cut on an unnatural position to be displayed.
                    
                    wordFits = false; //Word will never fit in this state
                    break;
                }
                
                for (int b = 0; b < bytes; b++) {
                    tempString[tempIndex] = string[index];
                    tempIndex++;
                    index++;
                }

                   charCount++;
                if (visibleChars < 0 || charCount <= visibleChars) {
                    lineDrawEnd = tempIndex;
                } else {
                    allCharsDrawn = (visibleChars < 0);
                }

                ww += advance;
                //wh = Math.max(wh, letterHeight);
            }

            if (!wordFits) {
                //Word longer than a full line, cut up word
                tempIndex = lastFittingChar + 1;                

                lineWidth += ww;
                lineHeight = MAX(lineHeight, wh);
            } else if (lineWidth + ww > maxWidth) {
                tempIndex -= (index - oldIndex);
                lineDrawEnd = MIN(lineDrawEnd, tempIndex);
                index = oldIndex;
                wordFits = false;
            }
        }
        
        if (!wordFits || index >= to) {
            //Word doesn't fit on the current line or the end of the string
            //has been reached
            
            if (draw) {
                //Draw Line
                tempString[lineDrawEnd] = '\0';
                PrintString(tempString);

                   lastPenX = penX;
                   lastPenY = penY;
                PrintNewLine();
            }
            
            //New Line
            line++;
            lineWidth = penX - marginLeft;
            lineHeight = 0;
            lineDrawEnd = 0;
            wordCount = 0;                
            tempIndex = 0;
        } else {
            //Add word to line
            wordCount++;
            lineWidth += ww;
            lineHeight = MAX(lineHeight, wh);
        }        
    }
    
    penX = lastPenX;
    penY = lastPenY;
    
    return MAX(1, line);
}

int Text::PrintStringWrapLines(const char* string) {
    return WrapString(string, false);
}

void Text::PrintStringWrap(const char* string) {
    WrapString(string, true);
}

void Text::PrintStatusMessage(const char *msg) {
    int x, y;
    GetPen(&x, &y);

    SetPen(10, 10);
    PrintString(msg);

    SetPen(x,y);
}
