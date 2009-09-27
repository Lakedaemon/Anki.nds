#ifndef T_COMMON_H
#define T_COMMON_H

/*
 * A collection of random utility functions and unspecific macro's
 */

#include <nds.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#define HEAP_SIZE (mallinfo().uordblks)

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

class Rect {
    public:
        int x, y, w, h;

        Rect() {
            x = y = w = h = 0;
        }
        Rect(int x, int y, int w, int h) {
            this->x = x;
            this->y = y;
            this->w = w;
            this->h = h;
        }
};

u8 chartohex(char c);

//Copy a block of pixels, ignore transparency
void blit(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, ignore transparency
void blit(const u8* src, u16 sw, u16 sh,
          u8* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, use bit 15 for binary transparency
void blit2(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, use the alpha channel for binary transparency
void blit2(const u16* src, const u8* alpha, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

//Copy a block of pixels, use the alpha channel for 8-bit transparency
void blitAlpha(const u16* src, const u8* srcA, u16 sw, u16 sh,
               u16* dst,           u16 dw, u16 dh,
               s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch);

void trimString(char* string);
void unescapeString(char* string);
char* basename(char*);

#endif
