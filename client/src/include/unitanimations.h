// mode: -*- C++ -*-
#ifndef UNITANIMATIONS_H
#define UNITANIMATIONS_H

#include "SDL_types.h"
#include "actioneventqueue.h"
#include "unit.h"
#include "structure.h"

class Unit;
class UnitOrStructure;
class UnitAnimEvent;
class Path;

class UnitAnimEvent : public ActionEvent {
public:
    UnitAnimEvent(Uint32 p, Unit* un);
    virtual ~UnitAnimEvent();
    void setSchedule(UnitAnimEvent* e);
    void stopScheduled();
    virtual void stop() = 0;
    virtual void update() {}
    virtual void run() = 0;
private:
    Unit* un;
    UnitAnimEvent* scheduled;
};

class MoveAnimEvent : public UnitAnimEvent
{
public:
    MoveAnimEvent(Uint32 p, Unit* un);
    virtual ~MoveAnimEvent();
    virtual void stop();
    virtual void run();
    virtual void update();
    virtual void setRange(Uint32 nr)
    {
        range = nr;
    }
private:
    bool stopping;
    void startMoveOne(bool wasblocked);
    void moveDone();
    Uint16 dest,newpos;
    bool blocked, moved_half, pathinvalid, waiting;
    Sint8 xmod, ymod;
    Unit* un;
    Path* path;
    Uint8 istep,dir;
    Uint32 range,speed;
};

class WalkAnimEvent : public UnitAnimEvent
{
public:
    WalkAnimEvent(Uint32 p, Unit* un, Uint8 dir, Uint8 layer);
    virtual ~WalkAnimEvent();
    virtual void stop()
    {
        stopping = true;
    }
    virtual void run();
    virtual void changedir(Uint8 ndir)
    {
        stopping = false;
        dir = ndir;
        calcbaseimage();
    }
    void update() {}
private:
    bool stopping;
    void calcbaseimage(void);
    Unit* un;
    Uint8 dir, istep, layer, baseimage;
};

class TurnAnimEvent : public UnitAnimEvent
{
public:
    TurnAnimEvent(Uint32 p, Unit *un, Uint8 dir, Uint8 layer);
    virtual ~TurnAnimEvent();
    virtual void run();
    virtual void stop()
    {
        stopping = true;
    }
    void update() {}
    virtual void changedir(Uint8 ndir)
    {
        stopping = false;
        dir = ndir;
    }
private:
    bool stopping,runonce;
    Sint8 turnmod;
    Unit *un;
    Uint8 dir;
    Uint8 layer;
};

class UAttackAnimEvent : public UnitAnimEvent
{
public:
    UAttackAnimEvent(Uint32 p, Unit *un);
    virtual ~UAttackAnimEvent();
    void stop();
    virtual void update();
    virtual void run();
private:
    Unit *un;
    bool stopping;
    Uint8 waiting;
    UnitOrStructure* target;
};

class HarvestAnimEvent : public UnitAnimEvent
{
public:
    HarvestAnimEvent(Uint32 p, Unit *un);
    virtual ~HarvestAnimEvent();
    void stop();
    virtual void update();
    virtual void run();
private:
    bool findNearbyOre();
    inline bool harvestOreAt(const Uint16 x, const Uint16 y);
    Unit *un;
    bool stopping;
    Uint8 waiting;
    //Structure the harvester will return to (refinery)
    Structure* refstr;
    //Refinery structure type
    StructureType* reftype;
    //Owner of the unit
    Player* player;
    //harvest cell (targetcell) and return cell nearby the refinery (refpos) 
    Uint16 targetcell,refpos;
    //Each harvester has a maximum amount of bails he can carry, they should be made visible (usually 5)
    //Each bail consists of a fixed amount of ore (usually around 3)
    //Each bail has a certain bail type (empty, ore or gem)
    Uint8 maxbails;
    //There are 4 states the harvester can be in:
    //searching: The harvester tries to move to the assigned harvesting position (targetcell)
    //gathering: The harvester gathers as much ore as possible at it's current position (usually targetcell ;)
    //returning: The harvester tries to move back to it's refinery (or tries to find another refinery)
    //refining:  The harvester returns the ore to the refinery (at refpos, nearby refstr)
    enum harv_s { harv_search = 1, harv_gather = 2, harv_return = 3, harv_refine = 4 } harv_t;
};

#endif /* UNITANIMATIONS_H */
