#ifndef _FILESELECT_H_
#define _FILESELECT_H_

#include <nds.h>
#include <fat.h>
#include <sys/dir.h>
#include <unistd.h>
#include "text.h"
#include "ndsrs.h"

const int MAXSRS = 8; // eh, I`ll make this more if its needed...



struct FOption {
    char display[128];
    char srs[128];
    char rep[128];
    int total;
    int todo;
    fakelong* todolist;
};



class FileSelect : public NDSRS {
    private:
        FOption files[MAXSRS];
        Text* text;
        FontCache* fcache;
        
        bool running;
        Preferences* prefs;
        
        int selected;
        int filesmax;
        int itemstodo;
        
        int TotalCards();
        int NumberCardsDue();
        bool SyncToAnki();
        
        void ShowFileStats();
        void PrepareReview();
        void DrawTop();
    
    public:
        FileSelect(Preferences*);
        ~FileSelect();
        
        void Run();
        Preferences* GetPrefs() {return prefs;};
};




#endif /* _FILESELECT_H_ */
