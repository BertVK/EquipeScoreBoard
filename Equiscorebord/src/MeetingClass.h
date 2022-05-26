#ifndef MeetingClass_h
#define MeetingClass_h

#include <Arduino.h>
#include <vector>

class MeetingClass
{
public:
    MeetingClass();
    int id;
    int classSection0Id;
    String name;
    String horse_ponies;
    String display_time;
    // std::vector<Rider> riders;
    
private:
};

#endif