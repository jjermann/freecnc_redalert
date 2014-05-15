// mode: -*- C++ -*-
#ifndef UNITQUEUE_H
#define UNITQUEUE_H

#include <map>
#include <queue>
#include <vector>
#include "unit.h"

class UnitOrStructure;

/// Default behavior: no priorities used
class GeneralUnitComp
{
public:
    bool operator() (Unit* x, Unit* y)
    {
        return 1;
    }
};

/// Units with lower health/maxhealth ratios get seen to first
class HealthUnitComp :  public GeneralUnitComp
{
public:
    bool operator() (Unit*x, Unit* y)
    {
        return (x->getRatio() > y->getRatio());
    }
};

class UnitQueue
{
public:
    UnitQueue(UnitOrStructure* container, std::vector<Unit*> units);
    ~UnitQueue();
    void enqueue(std::vector<Unit*> units);
    Unit* getNextUnit();
    void purge();
    void addCancel(Unit* un);
    bool hasCancelled(Unit* un)
    {
        return cancelled[un];
    }
private:
    UnitOrStructure* container;
    std::map<Unit*,bool> cancelled;
    std::priority_queue<Unit*, std::vector<Unit*>, GeneralUnitComp>* unit_queue;
};

#endif /* UNITQUEUE_H */
