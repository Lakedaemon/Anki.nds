#ifndef _CARD_H_
#define _CARD_H_

typedef char fakelong[32];

class Card {
    private:
        fakelong id;
        int time;
        int reps;
        char* question;
        char* answer;
        
    public:
        Card(char*);
        ~Card();
        
        char* ID() {return id;};
        int Time() {return time;};
        int Reps() {return reps;};
        char* Question() {return question;};
        char* Answer() {return answer;};
};














#endif /* _CARD_H_ */
