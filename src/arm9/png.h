#ifndef _PNG_H_
#define _PNG_H_
 
#include <nds.h>
#include <png.h>

typedef struct {
    int x, y;
    int w, h;
} NDSRECT;
 
png_structp PNGFromMemory(char* data, int dataL);
bool PNGGetBounds(char* data, int dataL, int* w, int* h);
bool PNGGetBounds(char* data, int dataL, NDSRECT* destrect);
bool PNGLoadImage(char* data, int dataL, u16* out, u8* alphaOut, int w, int h);
bool PNGLoadImage(char* data, int dataL, u16* out, u8* alphaOut, NDSRECT destrect);
bool PNGSaveImage(u16* data, int width, int height);

#endif /* _PNG_H_ */
