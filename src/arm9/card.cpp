#include "card.h"
#include <stdio.h>
#include <string.h>



Card::Card(char* s)
{
    int tabp[4], ti = 0;
    int sl = strlen(s);
    for (int i = 0; i < sl; ++i) {
        if (s[i] == '\t')
            tabp[ti++] = i;
    }
        
    question = new char[tabp[3] - tabp[2] + 1];
    answer = new char[sl - tabp[3] + 1];
    
    sscanf(s, "%s\t%d\t%d\t%[^\t]\t%[^\n]", id, &time, &reps, question, answer);
}

Card::~Card()
{
    delete[] question;
    delete[] answer; 
}


