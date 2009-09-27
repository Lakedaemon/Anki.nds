#ifndef _REVIEW_H_
#define _REVIEW_H_

#include "ndsrs.h"
#include "text.h"
#include "card.h"

class Review : public NDSRS {
    private:
        u16* answercache;
        u16* questioncache;
        
        bool running;
        Preferences* prefs;
    
        int tindex;
        Card** todo;
        int score;
        
        int oldx, oldy;
        
        bool display;
        bool txthinting;
        
        Text* text;
        FontCache* fcache;
        
        void ShowCardStats();
        void LoadTodo();
        void Shuffle();
        void NextCard();
        
        void DrawTop();
        void DrawBottom();
        void DisplayQuestion();
        void DisplayAnswer();
        void DisplayStats();
        
        bool IsDisplayed();
        void TouchScreen();
        void WriteAnswer();
        
        void DrawLine(int,int,int,int);
    
    public:
        Review(Preferences*);
        ~Review();
        
        void Run();
};












#endif /* _REVIEW_H_ */
