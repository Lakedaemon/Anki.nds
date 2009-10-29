#include "button.h"
#include <stdio.h>

#define DEBUGFILE "debug.txt"

Button::Button(int top, int left, int bottom, int right, int fontsize, const char* buttonText, List<const char*>* states)
{
    _top    = top;
    _left   = left;
    _bottom = bottom;
    _right  = right;
    _fontsize = fontsize;
    _buttonText = buttonText;
    _states = states;
    _drawn  = false;
    _highlighted = false;
    _text = NULL;
    _screen = NULL;
/*    FILE* deb = fopen(DEBUGFILE,"a");
    char buf[128];
    int l = sprintf(buf,"t %d, l %d, b %d, r %d\n",top,left,bottom,right);
    fwrite(buf,1,l,deb);
    fclose(deb);*/ 
}

Button::~ Button()
{
    if (_states != NULL) {
        _states->~List();
    }
}

bool Button::IsInButtonArea(int x, int y)
{
/*    FILE* deb = fopen(DEBUGFILE,"a");
    char buf[128];
    int l = sprintf(buf,"IsInArea x %d, y %d\n",x,y);
    fwrite(buf,1,l,deb);
    fclose(deb);*/ 
    return ((_left <= x) && (x <= _right) && (_top <= y) && (y <= _bottom));
}

bool Button::UsedInState(const char* state)
{
    if (_states == NULL) {
        return true;
    }
    for (int i = 1; i <= _states->getAnzElems(); i++) {
        if (strcmp(_states->getElem(i),state) == 0)
        {
            return true;
        }
    }
    return false;
}

bool Button::HandleDown(int x, int y)
{
    if (_drawn && IsInButtonArea(x,y)) {
        Highlight();
        DrawText();
        return true;
    }
    return false;
}

bool Button::HandleHeld(int x, int y)
{
    bool inArea = IsInButtonArea(x,y);
    if (inArea && !_highlighted) {
        Highlight();
        DrawText();
        return true;
    }
    else if (!inArea && _highlighted) {
        ClearRect();
        DrawText();
        return true;
    }
    return false;
}

bool Button::HandleUp()
{
    if (_drawn && _highlighted) {
        ClearRect();
        DrawText();
        return true;
    }
    return false;
}

void Button::ClearRect()
{
    for (int x = _left+1; x < _right; x++) {
        for (int y = _top+1; y < _bottom; y++) {
            _screen[x + y*SCREEN_WIDTH] = RGB15(31,31,31) | BIT(15);
        }
    }
    _highlighted = false;
}

void Button::Highlight()
{
    for (int x = _left+1; x < _right; x++) {
        for (int y = _top+1; y < _bottom; y++) {
            _screen[x + y*SCREEN_WIDTH] = RGB15(12,12,12) | BIT(15);
        }
    }
    _highlighted = true;
}

void Button::DrawText()
{
    _text->ClearBuffer();
    _text->SetFontSize(_fontsize);
    int x = _left + (_right - _left - _text->GetStringWidth(_buttonText))/2;
    int y = _top + (_bottom - _top + _text->GetLineHeight())/2;
    _text->SetPen(x,y);
    _text->PrintStringWrap(_buttonText);
    _text->BlitToScreen(_screen);
}

void Button::DrawButton(Text* text, u16* screen)
{
    _text = text;
    _screen = screen;
    int color = RGB15(0,0,0);
    for (int x = _left+1; x < _right; x++) {
        screen[x +    _top*SCREEN_WIDTH] = color;
        screen[x + _bottom*SCREEN_WIDTH] = color;
    }
    for (int y = _top+1; y < _bottom; y++) {
        screen[_left  + y*SCREEN_WIDTH] = color;
        screen[_right + y*SCREEN_WIDTH] = color;
    }
    DrawText();
/*    FILE* deb = fopen(DEBUGFILE,"a");
    char buf[128];
    int l = sprintf(buf,"DrawButton\n");
    fwrite(buf,1,l,deb);
    fclose(deb);*/ 
    _drawn = true;
    _highlighted = false;
}

void Button::RemoveButton()
{
    _drawn = false;
    _highlighted = false;
    _text = NULL;
    _screen = NULL;
}