#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include "text.h"
#include "button.h"
#include "list.h"

#define STATE_ASK_QUESTION  "AskQuestion"
#define STATE_SHOW_ANSWER   "ShowAnswer"

#define BUTTON_SHOW_ANSWER  "BShowAnswer"
#define BUTTON_AGAIN        "BAgain"
#define BUTTON_HARD         "BHard"
#define BUTTON_GOOD         "BGood"
#define BUTTON_EASY         "BEasy"
#define BUTTON_CLEAR_DOODLE "BClearDoodle"

class Buttons {
   protected:
   List<Button*>* _buttons;
   List<const char*>* _names;
   const char* _state;
   int _handledIndex;

   public:
   Buttons();
   virtual ~ Buttons();

   virtual void SetState(const char* state);
   virtual bool IsInButtonsArea(int x, int y);
   virtual bool HandleDown(int x, int y); // return if handled
   virtual bool HandleHeld(int x, int y); // return if graphics changed
   virtual const char* HandleUp(); // return button to execute
   virtual void DrawButtons(Text* text, u16* screen);
};

#endif // _BUTTONS_H_
