#include "buttons.h"
#include <stdio.h>
#include "expat.h"

#define BUFSIZE 1024

FILE* file;
const XML_Char* name;
const XML_Char* text;
int top, left, bottom, right, fontsize;
const char* state;
List<const char*>* states;
Button* button;
List<Button*>* buttons;
List<const char*>* names;
bool expectState, showInfoText;

void start(void *userData, const XML_Char *el, const XML_Char **attr)
{
    if (strcmp(el,"button") == 0) {
        top = bottom = left = right = 0;
        fontsize = 16;
        name = "";
        text = "";
        states = NULL;
        showInfoText = true;
        for (int i = 0; attr[i]; i += 2) {
            if (strcmp(attr[i],"name") == 0) {
                char* str = (char*)malloc(strlen(attr[i+1])+1);
                strcpy(str,attr[i+1]);
                name = str;
            }
            else if (strcmp(attr[i],"top") == 0) {
                top = atoi(attr[i+1]);
            }
            else if (strcmp(attr[i],"left") == 0) {
                left = atoi(attr[i+1]);
            }
            else if (strcmp(attr[i],"bottom") == 0) {
                bottom = atoi(attr[i+1]);
            }
            else if (strcmp(attr[i],"right") == 0) {
                right = atoi(attr[i+1]);
            }
            else if (strcmp(attr[i],"fontsize") == 0) {
                fontsize = atoi(attr[i+1]);
            }
            else if (strcmp(attr[i],"text") == 0) {
                char* str = (char*)malloc(strlen(attr[i+1])+1);
                strcpy(str,attr[i+1]);
                text = str;
            }
            else if (strcmp(attr[i],"showInfoText") == 0) {
                if (strcmp(attr[i+1],"no") == 0) {
                    showInfoText = false;
                }
            }
        }
    }
    else if (strcmp(el,"state") == 0) {
        expectState = true;
        if (states == NULL) {
            states = new List<const char*>();
        }
    }
}  /* End of start handler */

void end(void *userData, const XML_Char *el)
{
    if (strcmp(el,"button") == 0) {
        button = new Button(top,left,bottom,right,fontsize,text,showInfoText,states);
        buttons->appendElem(button);
        names->appendElem(name);
    }
    else if (strcmp(el,"state") == 0) {
        expectState = false;
        states->appendElem(state);
    }
}  /* End of end handler */

void character(void *userData, const XML_Char *s, int len)
{
    if (expectState) {
        char* str = (char*)malloc(len + 1);
        strncpy(str,s,len);
        str[len] = '\0';
        state = str;
    }
}

Buttons::Buttons()
{
    buttons = new List<Button*>();
    names   = new List<const char*>();
    XML_Parser p = XML_ParserCreate(NULL);
    XML_SetElementHandler(p,start,end);
    XML_SetCharacterDataHandler(p,character);
    XML_Char Buffer[BUFSIZE];
    FILE* xml = fopen("buttons.xml","r");
    bool eof = false;
    while(!eof) {
        int len = fread(Buffer,1,BUFSIZE,xml);
        eof = feof(xml);
        XML_Parse(p,Buffer,len,eof);
    }
    fclose(xml);
    fclose(file);
    _buttons = buttons;
    _names   = names;
    _state = NULL;
    _handledIndex = 0;
}

Buttons::~ Buttons()
{
    for (int i = 1; i <= _buttons->getAnzElems(); i++) {
        _buttons->getElem(i)->~Button();
    }
    _buttons->~List();
}

void Buttons::SetState(const char* state)
{
    for (int i = 1; i <= _buttons->getAnzElems(); i++) {
        Button* button = _buttons->getElem(i);
        if (button->UsedInState(_state))
        {
            button->RemoveButton();
        }
    }
    _state = state;
}

bool Buttons::IsInButtonsArea(int x, int y)
{
    for (int i = 1; i <= _buttons->getAnzElems(); i++) {
        if (_buttons->getElem(i)->IsInButtonArea(x,y))
        {
            return true;
        }
    }
    return false;
}

bool Buttons::HandleDown(int x, int y)
{
    int i;

    for (i = 1; i <= _buttons->getAnzElems(); i++) {
        if (_buttons->getElem(i)->HandleDown(x,y))
        {
            _handledIndex = i;
            return true;
        }
    }
    return false;
}

bool Buttons::HandleHeld(int x, int y)
{
    if (_handledIndex > 0) {
        return _buttons->getElem(_handledIndex)->HandleHeld(x,y);
    }
    return false;
}

const char* Buttons::HandleUp()
{
    if (_handledIndex > 0) {
        if (_buttons->getElem(_handledIndex)->HandleUp()) {
            return _names->getElem(_handledIndex);
        }
    }
    return NULL;
}

void Buttons::DrawButtons(Text* text, u16* screen)
{
    for (int i = 1; i <= _buttons->getAnzElems(); i++) {
        Button* button = _buttons->getElem(i);
        if (button->UsedInState(_state))
        {
            button->DrawButton(text,screen);
        }
    }
}