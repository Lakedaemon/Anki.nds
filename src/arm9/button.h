#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "list.h"
#include "text.h"

class Button {
   protected:
   int _top, _left, _bottom, _right, _fontsize;
   List<const char*>* _states;
   const char* _buttonText;
   bool _drawn, _highlighted;
   Text* _text;
   u16* _screen;
   virtual void ClearRect();
   virtual void Highlight();
   virtual void DrawText();

   public:
   Button(int top, int left, int bottom, int right, int fontsize, const char* buttonText, List<const char*>* states);
   virtual ~ Button();

   virtual bool IsInButtonArea(int x, int y);
   virtual bool UsedInState(const char* state);
   virtual bool HandleDown(int x, int y); // return if handled
   virtual bool HandleHeld(int x, int y); // return if graphics changed
   virtual bool HandleUp(); // return if to execute
   virtual void DrawButton(Text* text, u16* screen);
   virtual void RemoveButton();
};

#endif // _BUTTON_H_
