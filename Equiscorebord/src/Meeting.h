#ifndef Meeting_h
#define Meeting_h

#include <Arduino.h>
#include <MeetingClass.h>
#include <vector>

class Meeting
{
public:
    Meeting();
    int id;
    String date;
    String name;
    String type;
    std::vector<MeetingClass> classes;
    
private:
};

#endif