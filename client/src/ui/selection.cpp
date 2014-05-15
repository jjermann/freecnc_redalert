#include <cstdlib>
#include "ccmap.h"
#include "common.h"
#include "dispatcher.h"
#include "path.h"
#include "playerpool.h"
#include "selection.h"
#include "structure.h"
#include "unit.h"
#include "unitorstructure.h"
#include "weaponspool.h"

using std::map;
using std::vector;

Selection::Selection()
{
    numinfantry = 0;
    numattacking = 0;
    enemies_selected = false;
}

Selection::~Selection()
{
    Uint16 i,j;
    for (i=0;i<sel_units.size();++i) {
        if (sel_units[i] != NULL) {
            sel_units[i]->unrefer();
        }
    }
    for (i=0;i<sel_structures.size();++i) {
        if (sel_structures[i] != NULL) {
            sel_structures[i]->unrefer();
        }
    }
    sel_units.resize(0);
    sel_structures.resize(0);
    for (i=0;i<10;++i) {
        for (j=0;j<saved_unitsel[i].size();++j) {
            if (saved_unitsel[i][j] != NULL) {
                saved_unitsel[i][j]->unrefer();
            }
            saved_unitsel[i][j] = NULL;
        }
        saved_unitsel[i].resize(0);
    }
    for (i=0;i<10;++i) {
        for (j=0;j<saved_structsel[i].size();++j) {
            if (saved_structsel[i][j] != NULL) {
                saved_structsel[i][j]->unrefer();
            }
            saved_structsel[i][j] = NULL;
        }
        saved_structsel[i].resize(0);
    }
}

bool Selection::targetinRange(Uint16 pos)
{
    Uint16 i;
    for (i=0;i<sel_units.size();++i) {
        if (!sel_units[i]->targetinRange(pos)) return false;
    }
    for (i=0;i<sel_structures.size();++i) {
        if (!sel_structures[i]->targetinRange(pos)) return false;
    }
    return true;
}

bool Selection::legalMove(Uint16 pos)
{
    for (Uint8 i=0;i<sel_units.size();++i) {
        if (!p::ccmap->isBuildableAt(pos,sel_units[i]->getType())) return false;
    }
    return true;
}

void Selection::addUnit(Unit *selunit, bool enemy)
{
    selunit->select();
    selunit->referTo();
    sel_units.push_back(selunit);
    if( ((UnitType *)selunit->getType())->isInfantry() )
        numinfantry++;

    enemies_selected = enemy;
    if( !enemy && selunit->canAttack() ) {
        numattacking++;
    }
}

void Selection::removeUnit(Unit *selunit)
{
    Uint32 i;
    bool removing = false;

    selunit->unSelect();

    for( i = 0; i < sel_units.size(); i++ ) {
        if( removing )
            sel_units[i-1] = sel_units[i];
        if( sel_units[i] == selunit )
            removing = true;
    }
    sel_units.resize(sel_units.size()-1);
    if( ((UnitType *)selunit->getType())->isInfantry() )
        numinfantry--;

    if( enemies_selected ) {
        enemies_selected = false;
    } else if( selunit->canAttack() ) {
        numattacking--;
    }
    selunit->unrefer();
}

void Selection::addStructure(Structure *selstruct, bool enemy)
{
    selstruct->select();
    selstruct->referTo();
    sel_structures.push_back(selstruct);
    enemies_selected = enemy;
    if( !enemy && selstruct->canAttack() ) {
        ++numattacking;
    }
}

void Selection::removeStructure(Structure *selstruct)
{
    Uint32 i;
    bool removing = false;

    selstruct->unSelect();
    selstruct->unrefer();

    for( i = 0; i < sel_structures.size(); i++ ) {
        if( removing )
            sel_structures[i-1] = sel_structures[i];
        if( sel_structures[i] == selstruct )
            removing = true;
    }
    sel_structures.resize(sel_structures.size()-1);
    if( enemies_selected ) {
        enemies_selected = false;
    } else if( selstruct->canAttack() ) {
        --numattacking;
    }
}

void Selection::clearSelection()
{
    Uint32 i;

    numinfantry = 0;
    numattacking = 0;
    for( i = 0; i < sel_units.size(); i++ ) {
        sel_units[i]->unSelect();
        sel_units[i]->unrefer();
    }
    for( i = 0; i < sel_structures.size(); i++ ) {
        sel_structures[i]->unSelect();
        sel_structures[i]->unrefer();
    }
    sel_units.resize(0);
    sel_structures.resize(0);
    enemies_selected = false;
}

bool Selection::canAttack()
{
    return !enemies_selected && (numattacking > 0);
}

bool Selection::canHarvest(Uint16 pos)
{
    Uint8 tmp1,tmp2;
    if ((pos!=POS_INVALID) && !(p::ccmap->getResource(pos,&tmp1,&tmp2))) {
        return false;
    }
    for (Uint8 i=0;i<sel_units.size();++i) {
        if (!(((UnitType*)sel_units[i]->getType())->canHarvest())) return false;
    }
    if (sel_units.size()>0) return true;
    else return false;
}

bool Selection::canMove()
{
    // comment out check against enemies_selected
    // to test heat seeking code
    return !enemies_selected && (sel_units.size() > 0);
}


void Selection::moveUnits(Uint16 pos)
{
    Uint32 i;

    for( i = 0; i < sel_units.size(); i++ ) {
        //      pt = new Path(sel_units[i]->getPos(), stop, costmatrix);
        //      if( pt != NULL )
        // printf("Found a path\n");
        if( sel_units[i]->isAlive() ) {
            p::dispatcher->unitMove(sel_units[i],pos);
        }
    }

}

void Selection::harvest(Uint16 pos)
{
    Uint32 i;

    for( i = 0; i < sel_units.size(); i++ ) {
        //      pt = new Path(sel_units[i]->getPos(), stop, costmatrix);
        //      if( pt != NULL )
        // printf("Found a path\n");
        if( sel_units[i]->isAlive() ) {
            p::dispatcher->unitHarvest(sel_units[i],pos);
        }
    }

}

void Selection::attackUnit(Unit *target)
{
    Uint32 i;

    for( i = 0; i < sel_units.size(); i++ ) {
        //      pt = new Path(sel_units[i]->getPos(), stop, costmatrix);
        //      if( pt != NULL )
        // printf("Found a path\n");
        if( sel_units[i]->isAlive() && sel_units[i]->canAttack() ) {
            p::dispatcher->unitAttack(sel_units[i],target,true);
        }
    }
    for( i = 0; i < sel_structures.size(); ++i) {
        /* add: when power checking is done, check if structure selected
         * can work given the player's power levels */
        if( sel_structures[i]->isAlive() && sel_structures[i]->canAttack() ) {
            p::dispatcher->structureAttack(sel_structures[i],target,true);
        }
    }
}

void Selection::attackStructure(Structure *target)
{
    Uint32 i;

    for( i = 0; i < sel_units.size(); i++ ) {
        if( sel_units[i]->isAlive() && ((UnitType *)sel_units[i]->getType())->getWeapon() != NULL &&
                (!target->isWall() || ((UnitType*)sel_units[i]->getType())->getWeapon()->getWall())) {
            p::dispatcher->unitAttack(sel_units[i],target,false);
        }
    }
    for( i = 0; i < sel_structures.size(); ++i) {
        /* add: when power checking is done, check if structure selected
         * can work given the player's power levels */
        if( sel_structures[i]->isAlive() && sel_structures[i]->canAttack() &&
                (!target->isWall() || ((StructureType*)sel_structures[i]->getType())->getWeapon()->getWall())) {
            p::dispatcher->structureAttack(sel_structures[i],target,false);
        }
    }
}

void Selection::checkSelection()
{
    Uint16 i;

    for( i = 0; i < sel_units.size(); i++ ) {
        if( !sel_units[i]->isAlive() ) {
            purgeUnit(sel_units[i]);
            removeUnit(sel_units[i]);
            i--;
        }
    }
    for( i = 0; i < sel_structures.size(); i++ ) {
        if( !sel_structures[i]->isAlive() ) {
            purgeStructure(sel_structures[i]);
            removeStructure(sel_structures[i]);
            i--;
        }
    }
}

Unit* Selection::getRandomUnit(void)
{
    Uint8 rnd,sze;
    sze = sel_units.size();
    if (sze > 0) {
        rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
        return sel_units[rnd];
    } else {
        return NULL;
    }
}

Structure* Selection::getRandomStructure(void)
{
    Uint8 rnd,sze;
    sze = sel_structures.size();
    if (sze > 0) {
        rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
        return sel_structures[rnd];
    } else {
        return NULL;
    }
}

bool Selection::getWall()
{
    Uint16 i;
    if (sel_units.empty() && sel_structures.empty())
        return false;
    for (i=0;i<sel_units.size();++i) {
        if ((((UnitType*)sel_units[i]->getType())->getWeapon() != NULL)&&(((UnitType*)sel_units[i]->getType())->getWeapon()->getWall())) {
            return true;
        }
    }
    for (i=0;i<sel_structures.size();++i) {
        if ((((StructureType*)sel_structures[i]->getType())->getWeapon() != NULL)&&(((StructureType*)sel_structures[i]->getType())->getWeapon()->getWall())) {
            return true;
        }
    }
    return false;
}

void Selection::saveSelection(Uint8 savepos)
{
    if (savepos > 10)
        return;
    copySelection(&sel_units,&sel_structures,&saved_unitsel[savepos],&saved_structsel[savepos]);
}

void Selection::loadSelection(Uint8 savepos)
{
    Uint16 i;
    Uint16 num_deadunits;
    Uint16 num_deadstructs;
    Uint16 lplayernum;
    bool enemy;
    if (savepos > 10) {
        return;
    }
    clearSelection();
    num_deadunits = num_deadstructs = 0;
    //for (i=saved_unitsel[savepos].size()-1;i>=0;--i) {
    for (i=0;i<saved_unitsel[savepos].size();++i) {
        if (saved_unitsel[savepos][i] == NULL) {
            ++num_deadunits;
        }
        if ((saved_unitsel[savepos][i] != NULL)&&(!saved_unitsel[savepos][i]->isAlive())) {
            saved_unitsel[savepos][i] = NULL;
            ++num_deadunits;
        }
    }
    //for (i=saved_structsel[savepos].size()-1;i>=0;--i) {
    for (i=0;i<saved_structsel[savepos].size();++i) {
        if (saved_structsel[savepos][i] == NULL) {
            ++num_deadstructs;
        }
        if ((saved_structsel[savepos][i] != NULL)&&(!saved_structsel[savepos][i]->isAlive())) {
            saved_structsel[savepos][i] = NULL;
            ++num_deadstructs;
        }
    }
    if (((saved_unitsel[savepos].size() > num_deadunits)&&(saved_unitsel[savepos].size() > 0)) ||
            ((saved_structsel[savepos].size()>num_deadstructs)&&(saved_structsel[savepos].size() > 0))) {
        copySelection(&saved_unitsel[savepos],&saved_structsel[savepos],&sel_units,&sel_structures);
        if (sel_units.size() > 0) {
            lplayernum = p::ppool->getLPlayerNum();
        } else {
            lplayernum = p::ppool->getLPlayerNum();
        }
        for (i=0;i<sel_units.size();++i) {
            sel_units[i]->select();
            if( ((UnitType*)sel_units[i]->getType())->isInfantry() ) {
                ++numinfantry;
            }
            enemy = (sel_units[i]->getOwner() != lplayernum);
            if (!enemies_selected && enemy)
                enemies_selected = enemy;
            if (!enemy && sel_units[i]->canAttack()) {
                ++numattacking;
            }
        }
        for (i=0;i<sel_structures.size();++i) {
            sel_structures[i]->select();
            enemy = (sel_structures[i]->getOwner() != lplayernum);
            if (!enemies_selected && enemy)
                enemies_selected = enemy;
            if (!enemy && sel_structures[i]->canAttack()) {
                ++numattacking;
            }
        }
        checkSelection();
    }
}

void Selection::copySelection(vector<Unit*>* src_un, vector<Structure*>* src_st,
                              vector<Unit*>* trg_un, vector<Structure*>* trg_st)
{
    Uint16 i;
    for (i=0;i<(*trg_un).size();++i) {
        if ((*trg_un)[i] != NULL) {
            (*trg_un)[i]->unrefer();
        }
        (*trg_un)[i] = NULL;
    }
    (*trg_un).resize(0);
    for (i=0;i<(*src_un).size();++i) {
        if (((*src_un)[i] != NULL)&&(*src_un)[i]->isAlive()) {
            (*src_un)[i]->referTo();
            (*trg_un).push_back((*src_un)[i]);
        }
    }
    for (i=0;i<(*trg_st).size();++i) {
        if ((*trg_st)[i] != NULL) {
            (*trg_st)[i]->unrefer();
        }
        (*trg_st)[i] = NULL;
    }
    (*trg_st).resize(0);
    for (i=0;i<(*src_st).size();++i) {
        if (((*src_st)[i] != NULL)&&(*src_st)[i]->isAlive()) {
            (*src_st)[i]->referTo();
            (*trg_st).push_back((*src_st)[i]);
        }
    }
}

void Selection::purgeUnit(Unit* selunit)
{
    Uint16 i,j;
    for (i=0;i<10;++i) {
        for (j=0;j<saved_unitsel[i].size();++j) {
            if (saved_unitsel[i][j] == selunit) {
                selunit->unrefer();
                saved_unitsel[i][j] = NULL;
            }
        }
    }
}

void Selection::purgeStructure(Structure* selstruct)
{
    Uint16 i,j;
    for (i=0;i<10;++i) {
        for (j=0;j<saved_structsel[i].size();++j) {
            if (saved_structsel[i][j] == selstruct) {
                selstruct->unrefer();
                saved_structsel[i][j] = NULL;
            }
        }
    }
}

void Selection::stop()
{
    Uint32 i;
    for (i=0;i<sel_units.size();++i)
        sel_units[i]->stop();
    for (i=0;i<sel_structures.size();++i)
        sel_structures[i]->stop();
}

bool Selection::canLoad(Unit* target)
{
    return canLoad((UnitOrStructure*)target);
}

bool Selection::canLoad(Structure* target)
{
    return canLoad((UnitOrStructure*)target);
}

bool Selection::canLoad(UnitOrStructure* target)
{
    Uint32 i;
    if (sel_units.empty()) {
        return false;
    }
    for (i=0;i<sel_units.size();++i) {
        if (!target->canLoad(sel_units[i])) {
            return false;
        }
    }
    return true;
}

void Selection::loadUnits(Unit* target)
{
    loadUnits((UnitOrStructure*)target);
}

void Selection::loadUnits(Structure* target)
{
    loadUnits((UnitOrStructure*)target);
}

void Selection::loadUnits(UnitOrStructure* target)
{
    Uint32 i;
    vector<Unit*> tmp;
    if (canLoad(target)) {
        for (i=0;i<sel_units.size();++i) {
            if (target->canLoad(sel_units[i])) {
                tmp.push_back(sel_units[i]);
            }
        }
        target->loadUnits(tmp);
        for (i=0;i<tmp.size();++i) {
            removeUnit(tmp[i]);
            purgeUnit(tmp[i]);
        }
    }
}
