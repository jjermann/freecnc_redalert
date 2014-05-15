// mode: -*- C++ -*-
#ifndef SELECTION_H
#define SELECTION_H

#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif

#include <vector>
#include "SDL.h"

class Unit;
class UnitType;
class Structure;
class StructureType;
class UnitOrStructure;
class UnitOrStructureType;

class Selection
{
public:
    Selection();
    ~Selection();
    void addUnit(Unit *selunit, bool enemy);
    void removeUnit(Unit *selunit);
    void addStructure(Structure *selstruct, bool enemy);
    void removeStructure(Structure *selstruct);

    void saveSelection(Uint8 savepos);
    void loadSelection(Uint8 loadpos);

    void clearSelection();

    void purgeUnit(Unit* selunit);
    void purgeStructure(Structure* selstruct);

    bool targetinRange(Uint16 pos);
    bool legalMove(Uint16 pos);
    bool canHarvest(Uint16 pos=POS_INVALID);
    bool canAttack();
    bool canMove();
    bool isEnemy()
    {
        return enemies_selected;
    }
    bool empty()
    {
        return sel_units.empty() && sel_structures.empty();
    }
    void harvest(Uint16 stop);
    void moveUnits(Uint16 stop);
    void attackUnit(Unit* target);
    void attackStructure(Structure* target);
    void checkSelection();
    Unit* getRandomUnit();
    Structure* getRandomStructure();
    bool getWall();
    bool canLoad(Unit* target);
    bool canLoad(Structure* target);
    bool canLoad(UnitOrStructure* target);
    void loadUnits(Unit* target);
    void loadUnits(Structure* target);
    void loadUnits(UnitOrStructure* target);
    void stop();
private:
    std::vector<Unit *> sel_units;
    std::vector<Structure *> sel_structures;

    Uint16 numinfantry;
    Uint16 numattacking;
    bool enemies_selected;
    std::vector<Unit *>saved_unitsel[10];
    std::vector<Structure *>saved_structsel[10];
    void copySelection(std::vector<Unit*>* src_un, std::vector<Structure*>* src_st,
                       std::vector<Unit*>* trg_un, std::vector<Structure*>* trg_st);
};


#endif
