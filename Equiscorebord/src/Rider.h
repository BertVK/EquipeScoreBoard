#ifndef Rider_h
#define Rider_h

#include <Arduino.h>

class Rider
{
public:
    Rider();
    int id;
    String rider_name;
    String horse_name;
    int horse_combination_no;
    String start_at;
    int rank;
    String start_no;
    
private:
};

#endif