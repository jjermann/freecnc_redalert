// mode: -*- C++ -*-
#ifndef ACTIONEVENTQUEUE_H
#define ACTIONEVENTQUEUE_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <queue>
#include <vector>
#include "SDL.h"

/** An abstract class which all actionevents must extend. the run must
 * be implemented. */
class ActionEvent
{
public:
    friend class Comp;
    ActionEvent( Uint32 p )
    {
        delay = p;
    }
    void addCurtick( Uint32 curtick )
    {
        prio=delay+curtick;
    }
    virtual void run()
    {}

    void setDelay(Uint32 p)
    {
        delay = p;
    }
    Uint32 getPrio()
    {
        return prio;
    }
    virtual ~ActionEvent()
    {}
    virtual void stop()
    {}
private:
    Uint32 prio, delay;
};

// Friend class which compares ActionEvents priority
class Comp
{
public:
    bool operator()(ActionEvent *x, ActionEvent *y)
    {
        return x->prio > y->prio;
    }
};

class ActionEventQueue
{
public:
    ActionEventQueue();
    ~ActionEventQueue();
    void scheduleEvent(ActionEvent *ev);
    void runEvents();
    Uint32 getElapsedTime();
    Uint32 getCurtick();
private:
    Uint32 starttick;
    std::priority_queue<ActionEvent*, std::vector<ActionEvent*>, Comp> eventqueue;
};

#endif
