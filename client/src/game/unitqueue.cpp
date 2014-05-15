#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif
#include "SDL.h"
#include "common.h"
#include "logger.h"
#include "unit.h"
#include "unitorstructure.h"
#include "unitqueue.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::map;
using std::priority_queue;
using std::vector;
#endif

UnitQueue::UnitQueue(UnitOrStructure* container, vector<Unit*> units)
{
    this->container = container;
    unit_queue = new priority_queue<Unit*, vector<Unit*>, GeneralUnitComp>;
    enqueue(units);
}

UnitQueue::~UnitQueue()
{
    delete unit_queue;
}

Unit* UnitQueue::getNextUnit()
{
    Unit* retval;
    if (unit_queue->empty()) {
        return NULL;
    } else {
        while ( (unit_queue->top() == NULL) || (cancelled[unit_queue->top()]) ) {
            logger->note("skipping\n");
            if (cancelled[unit_queue->top()]) {
                logger->note("skipping over cancelled unit\n");
            }
            unit_queue->pop();
        }
        retval = unit_queue->top();
        unit_queue->pop();
    }
    return retval;
}

void UnitQueue::enqueue(vector<Unit*> units)
{
    Uint16 i;
    for (i=0;i<units.size();++i) {
        if (container->canLoad(units[i])) {
            cancelled[units[i]] = false;
            unit_queue->push(units[i]);
            units[i]->referTo();
        }
    }
}

void UnitQueue::purge()
{
    map<Unit*,bool>::iterator i;
    while (!unit_queue->empty()) {
        unit_queue->top()->unrefer();
        unit_queue->pop();
    }
    for (i=cancelled.begin();i!=cancelled.end();++i) {
        i->second = false;
    }
}

void UnitQueue::addCancel(Unit* un)
{
    cancelled[un] = true;
}
