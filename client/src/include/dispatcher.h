// mode: -*- C++ -*-
/*****************************************************************************
 * dispatcher.h - stores and plays back game sessions
 ****************************************************************************/
#ifndef DISPATCHER_H
#define DISPATCHER_H
#include <cstdio>
#include <queue>
#include <string>
#include <vector>
#include "SDL_types.h"

class Unit;
class UnitOrStructure;
class Structure;
class VFile;

namespace Disp {
    enum DispatchLogState {NORMAL, RECORDING, PLAYING};
}

class Dispatcher {
public:
    Dispatcher();
    ~Dispatcher();
    void unitMove(Unit* un, Uint32 dest);
    void unitHarvest(Unit* un, Uint32 dest);
    void unitAttack(Unit* un, UnitOrStructure* target, bool tisunit);
    void unitDeploy(Unit* un);
    void unitCreate(const char* tname, Uint32 pos, Uint8 subpos, Uint8 owner);
    void structureAttack(Structure* st, UnitOrStructure* target, bool tisunit);
    void structureCreate(const char* tname, Uint32 pos, Uint8 owner);
private:
    Disp::DispatchLogState logstate;
    Uint8 localPlayer;
};

#endif /* __DISPATCHER_H */
