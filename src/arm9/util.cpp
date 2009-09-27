#include "util.h"

#include <nds.h>

void makeifnotexist(char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) {
        FILE* f = fopen(path, "w");
        fclose(f);
    }
}

int readline(FILE* f, char* buf)
{
    int b = 0;
    while (true) {
        int c = fgetc(f);
        if (c == 0x0A)
            break;
        if (c == EOF)
            break;
        buf[b++] = c;
    }
    buf[b] = 0;
    return b;
}

int flen(FILE* f)
{
    int r = ftell(f);
    fseek(f, 0, SEEK_END);
    int l = ftell(f);
    fseek(f, r, SEEK_SET);
    return l;
}

void waitForAnyKey() {
    while (true) {
        scanKeys();
        if (keysDown() & (KEY_TOUCH|KEY_A|KEY_B|KEY_X|KEY_Y|KEY_START)) {
            break;
        }
        swiWaitForVBlank();
    }
}

