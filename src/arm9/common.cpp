#include "common.h"

#include <fat.h>
#include <ctype.h>
#include <sys/dir.h>
#include <errno.h>

#define BLT_ALPHA_BITS  8
#define BLT_MAX_ALPHA   ((1 << BLT_ALPHA_BITS) - 1)
#define BLT_ROUND       (1 << (BLT_ALPHA_BITS - 1))
#define BLT_THRESHOLD   8
#define BLT_THRESHOLD2  128

#define LOWC(x)         ((x)&31)

u8 chartohex(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	return 0;
}

inline s16 blitMinX(s16 sx, s16 dx) {
    return sx + MAX(0, -dx);
}
inline s16 blitMinY(s16 sy, s16 dy) {
    return sy + MAX(0, -dy);
}
inline s16 blitMaxX(s16 minX, u16 sw, u16 dw, s16 sx, s16 dx, u16 cw) {
    return sx + MIN(MIN(dw-dx, sw-sx), cw);
}
inline s16 blitMaxY(s16 minY, u16 sh, u16 dh, s16 sy, s16 dy, u16 ch) {
    return sy + MIN(MIN(dh-dy, sh-sy), ch);
}

void blit2(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const int sInc = sw - maxX + minX;
    const int dInc = dw - maxX + minX;
    const int srcOffset = (minY * sw) + minX;

    const u16* sc = src + srcOffset;
    u16* dc = dst + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
    	for (int x = minX; x < maxX; x++) {
    		if (*sc & BIT(15)) {
    			*dc = *sc;
    		}

    		sc++;
    		dc++;
    	}
        sc += sInc;
        dc += dInc;
    }
}

void blit2(const u16* src, const u8* alpha, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const int sInc = sw - maxX + minX;
    const int dInc = dw - maxX + minX;
    const int srcOffset = (minY * sw) + minX;

    const u8*  sa = alpha + srcOffset;
    const u16* sc = src   + srcOffset;
    u16* dc = dst   + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
    	for (int x = minX; x < maxX; x++) {
    		*dc = ((*sa & 0x80) ? *sc : 0);

    		sa++;
    		sc++;
    		dc++;
    	}
    	alpha += sInc;
        sc += sInc;
        dc += dInc;
    }
}

void blit(const u16* src, u16 sw, u16 sh,
          u16* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const u16* sc = src + (minY * sw) + minX;
    u16* dc = dst + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
        memcpy(dc, sc, (maxX - minX) << 1);
        sc += sw;
        dc += dw;
    }
}

void blit(const u8* src, u16 sw, u16 sh,
          u8* dst, u16 dw, u16 dh,
          s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const u8* sc = src + (minY * sw) + minX;
    u8* dc = dst + (MAX(0, dy) * dw) + MAX(0, dx);

    for (int y = minY; y < maxY; y++) {
        memcpy(dc, sc, maxX - minX);
        sc += sw;
        dc += dw;
    }
}

void blitAlpha(const u16* src, const u8* srcA, u16 sw, u16 sh,
               u16* dst,           u16 dw, u16 dh,
               s16 sx, s16 sy, s16 dx, s16 dy, u16 cw, u16 ch)
{
    //optimized asm for blending 2 16-bit palettes: http://forum.gbadev.org/viewtopic.php?p=53322

    const int minX = blitMinX(sx, dx);
    const int minY = blitMinY(sy, dy);
    const int maxX = blitMaxX(minX, sw, dw, sx, dx, cw);
    const int maxY = blitMaxY(minY, sh, dh, sy, dy, ch);

    const int sInc = sw - maxX + minX;
    const int dInc = dw - maxX + minX;
    const int srcOffset = (minY * sw) + minX;

    const u8*  sa = srcA + srcOffset;
    const u16* sc = src  + srcOffset;
    u16* dc = dst  + (MAX(0, dy) * dw) + MAX(0, dx);

    int s;
    int a;
    int d;
    int sbr;
    int dbr;
    int sg;
    int dg;

    for (int y = minY; y < maxY; y++) {
        for (int x = minX; x < maxX; x++) {
            a = *sa;
			if (a >= BLT_THRESHOLD) {
	            s = *sc;
	            d = *dc;
				if (d & BIT(15) && a <= BLT_MAX_ALPHA - BLT_THRESHOLD) {
					sbr = (s & 0x1F) | (s<<6 & 0x1F0000);
					dbr = (d & 0x1F) | (d<<6 & 0x1F0000);
					sg = s & 0x3E0;
					dg = d & 0x3E0;

					dbr = (dbr + (((sbr-dbr)*a + BLT_ROUND) >> BLT_ALPHA_BITS));
					dg  = (dg  + (((sg -dg )*a + BLT_ROUND) >> BLT_ALPHA_BITS));
					*dc = (dbr&0x1F) | (dg&0x3E0) | (dbr>>6 & 0x7C00) | BIT(15);
				} else {
					if (a >= BLT_THRESHOLD2) {
						*dc = s | BIT(15);
					}
				}
			}
            sa++;
            sc++;
            dc++;
        }
        sc += sInc;
        sa += sInc;
        dc += dInc;
    }
}

void trimString(char* string) {
    int a = 0;
    int b = strlen(string);

    while (isspace(string[a]) && a < b) a++;
    while (isspace(string[b-1]) && b > a) b--;

    if (b-a <= 0) {
        //string length is zero
        string[0] = '\0';
        return;
    }
    memmove(string, string+a, b-a);
	string[b-a] = '\0';
}
void unescapeString(char* str) {
	char* s = str;
    while (*s != '\0') {
        if (*s == '\\') {
            s++;
            switch (*s) {
                case '\\': *str = '\\'; break;
                case '\'': *str = '\''; break;
                case '\"': *str = '\"'; break;
                case 'n':  *str = '\n'; break;
                case 'r':  *str = '\r'; break;
                case 't':  *str = '\t'; break;
                case 'f':  *str = '\f'; break;
                default: /*Error*/ break;
            }
        } else {
        	*str = *s;
        }
        str++;
        s++;
    }
    *str = '\0';
}
