#include <algorithm>
#include <cctype>
#include "ccmap.h"
#include "common.h"
#include "game.h"
#include "inifile.h"
#include "logger.h"
#include "playerpool.h"
#include "structure.h"
#include "unit.h"
#include "unitorstructure.h"
#include "unitandstructurepool.h"
#include "weaponspool.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::make_pair;
using std::map;
using std::multimap;
using std::pair;
using std::string;
using std::vector;
#endif

using p::ccmap;

/** Constructor, loads all the units from the inifile and create
 * those units/structures in the unit/structure pool
 */
UnitAndStructurePool::UnitAndStructurePool() : structini(0), unitini(0),
    tbackini(0), artini(0), deleted_unitorstruct(false), numdeletedunit(0),
    numdeletedstruct(0)
{
    unitandstructmat.resize(ccmap->getWidth() * ccmap->getHeight());
    structini = new INIFile("structure.ini");
    unitini = new INIFile("unit.ini");
    tbackini = new INIFile("talkback.ini");
    artini = new INIFile("art.ini");
    p::weappool = new WeaponsPool();
    strcpy(theaterext, ".");
    strncat(theaterext, p::ccmap->getMissionData().theater, 3);
}


/** Destructor, empties the various matricies and deletes the WeaponsPool
 * and inifile members.
 */
UnitAndStructurePool::~UnitAndStructurePool()
{
    Uint32 i;

    // this is for cleaning up the multimaps
    typedef multimap<UnitType*, vector<StructureType*>* >::const_iterator Iu;
    typedef multimap<StructureType*, vector<StructureType*>* >::const_iterator Is;
    std::pair<Is,Is> structpair;
    std::pair<Iu,Iu> unitpair;

    std::map<std::string, Talkback*>::const_iterator It;

    delete structini;
    delete unitini;
    delete tbackini;
    delete artini;
    for( i = 0; i < unitpool.size(); i++ ) {
        delete unitpool[i];
    }
    for( i = 0; i < unittypepool.size(); i++ ) {
        unitpair = unit_prereqs.equal_range(unittypepool[i]);
        for (Iu l = unitpair.first; l != unitpair.second ; ++l) {
            delete l->second;
        }
        delete unittypepool[i];
    }
    for( i = 0; i < structurepool.size(); i++ ) {
        delete structurepool[i];
    }
    for( i = 0; i < structuretypepool.size(); i++ ) {
        structpair = struct_prereqs.equal_range(structuretypepool[i]);
        for (Is l = structpair.first; l != structpair.second ; ++l) {
            delete l->second;
        }
        delete structuretypepool[i];
    }

    It = talkbackpool.begin();
    while (It != talkbackpool.end()) {
        delete It->second;
        ++It;
    }
    delete p::weappool;
}



/** @brief Retrieve the imagenumber for the structure at the specified position
 * @param cellpos the position we want to know the structure of.
 * @param inumbers pointer to array of image numbers
 * @param xoffsets pointer to array of x offsets
 * @param yoffsets pointer to array of y offsets
 * @returns the imagenumber.  */
Uint8 UnitAndStructurePool::getUnitOrStructureNum(Uint16 cellpos, Uint32 **inumbers,
        Sint8 **xoffsets, Sint8 **yoffsets, bool* showpips)
{
    Structure *st;
    Unit *un;

    Uint8 layers;

    if( !(unitandstructmat[cellpos]&US_LOWER_RIGHT) )
        return 0;
    if( unitandstructmat[cellpos]&US_IS_UNIT ) {
        un = unitpool[(unitandstructmat[cellpos])&0xffff];
        if( ((UnitType *)un->getType())->isInfantry() ) {
            *showpips = false;
            return un->getInfantryGroup()->getImageNums(inumbers, xoffsets, yoffsets);
        }
        return un->getImageNums(inumbers, xoffsets, yoffsets,showpips);
    } else if( unitandstructmat[cellpos]&(US_IS_STRUCTURE|US_IS_WALL) ) {
        st = structurepool[(unitandstructmat[cellpos])&0xffff];
        layers = st->getImageNums(inumbers,xoffsets,yoffsets,showpips);
        return layers;
    }
    return 0;
}


bool UnitAndStructurePool::getUnitOrStructureLimAt(Uint32 curpos, float* width,
        float* height, Uint32* cellpos, Uint8* igroup, Uint8* owner,
        Uint8* pcol, bool* blocked)
{
    Uint32 cval = unitandstructmat[curpos];
    if (cval & US_IS_UNIT) {
        Unit* un = unitpool[cval&0xffff];
        *width   = 0.75f;
        *height  = 0.75f;
        *owner   = un->getOwner();
        *pcol    = p::ppool->getPlayer(*owner)->getStructpalNum();
        *igroup  = 0;
        *cellpos = un->getPos();
        *blocked = true;
        if (un->getType()->isInfantry()) {
            InfantryGroup* igrp = un->getInfantryGroup();
            for (int i=0;i<5;++i)
                if (igrp->unitAt(i) != NULL)
                    *igroup |= 1<<i;
        }
        return true;
    } else if ((cval & US_IS_STRUCTURE) || (cval & US_IS_WALL)) {
        Structure* st = structurepool[cval&0xffff];
        *width   = 1.0f;
        *height  = 1.0f;
        *owner   = st->getOwner();
        *pcol    = p::ppool->getPlayer(*owner)->getStructpalNum();
        *igroup  = 0;
        *cellpos = st->getBPos(curpos);
        *blocked = true;
        return true;
    }
    return false;
}

/// Gets a list of all flying stuff in the current tile
Uint8 UnitAndStructurePool::getL2overlays(Uint16 pos, Uint32 **inumbers, Sint8 **xoffset, Sint8 **yoffset)
{
    multimap<Uint16, L2Overlay*>::iterator entry;
    //multimap<Uint16, L2Overlay*>::iterator lastentry;
    L2Overlay *curl2;
    Uint8 numentries, i,j;
    if( !(unitandstructmat[pos]&US_HAS_L2OVERLAY) ) {
        return 0;
    }
    entry = l2pool.find(pos);
    numentries = numl2images[pos];
    *inumbers = new Uint32[numentries];
    *xoffset = new Sint8[numentries];
    *yoffset = new Sint8[numentries];
    for( i = 0; i < numentries; i++ ) {
        curl2 = entry->second;
        for (j=0;j<(curl2->numimages);++j) {
            (*inumbers)[i+j] = curl2->imagenums[j];
            (*xoffset)[i+j] = curl2->xoffsets[j];
            (*yoffset)[i+j] = curl2->yoffsets[j];
        }
        entry++;
    }
    return numentries;
}

multimap<Uint16, L2Overlay*>::iterator UnitAndStructurePool::addL2overlay(Uint16 cellpos, L2Overlay *ov)
{
    multimap<Uint16, L2Overlay*>::iterator entry;
    l2pool.insert(make_pair(cellpos, ov));
    unitandstructmat[cellpos]|=US_HAS_L2OVERLAY;
    numl2images[cellpos] += ov->numimages;
    entry = l2pool.find(cellpos);
    while(entry->second != ov) {
        entry++;
    }
    return entry;
}

void UnitAndStructurePool::removeL2overlay(multimap<Uint16, L2Overlay*>::iterator entry)
{
    Uint32 cellpos = entry->first;
    numl2images[cellpos] -= entry->second->numimages;
    l2pool.erase(entry);
    if( l2pool.find(cellpos) == l2pool.end() ) {
        unitandstructmat[cellpos]&=(~US_HAS_L2OVERLAY);
    }
}

/// Creates a structure
void UnitAndStructurePool::createStructure(const char *typen, Uint16 cellpos,
        Uint8 owner, Uint16 health, Uint8 facing, bool makeanim)
{
    int x, y, curpos;
    bool setlr, ok;
    Uint32 structnum;
    Uint8 count;
    StructureType *type;
    Structure *st;

    if (cellpos > (p::ccmap->getWidth() * p::ccmap->getHeight())) {
        logger->warning("Attempted to create a structure at %i, outside map\n",cellpos);
        return;
    }
    if( numdeletedstruct > 0 ) {
        for( structnum = 0; structnum < structurepool.size(); structnum++ ) {
            if(structurepool[structnum] == NULL)
                break;
        }
    } else {
        structnum = structurepool.size();
    }

    type = getStructureTypeByName(typen);
    if (type == NULL) {
        logger->warning("Invalid type %s\n",typen);
        return;
    }
    //   unitandstructmat[cellpos] = structurepool.size();
    if( type->isWall() ) {
        // walls will always be one cell
        if (getStructureAt(cellpos) == NULL) {
            unitandstructmat[cellpos] = (US_LOWER_RIGHT|US_IS_WALL)|structnum;
        }
    } else {
        curpos = cellpos+p::ccmap->getWidth()*type->getYsize();
        setlr = false;
        ok = true;
        count = 0;
        for( y = type->getYsize()-1; y>=0; y-- ) {
            curpos -= p::ccmap->getWidth();
            for(x = type->getXsize()-1; x>=0; x--) {
                if( ok && type->isBlocked(y*type->getXsize()+x) ) {
                    if (getStructureAt(curpos+x) != NULL) {
                        logger->warning("Structure already exists at (%i,%i) [%i]\n",x,y,curpos+x);
                        ok = false;
                    }
                    if (ok) {
                        for (Uint8 p = 0; p < 5; ++p) {
                            if (getUnitAt(curpos+x,p) != NULL) {
                                logger->warning("Unit already exists at cell %i subpos %i\n",curpos+x,p);
                                ok = false;
                                break;
                            }
                        }
                    }
                    if (!ok) {
                        if (count > 0) {
                            // undo setting cell flags
                            curpos = cellpos+p::ccmap->getWidth()*type->getYsize();
                            for( y = type->getYsize()-1; y>=0; y-- ) {
                                curpos -= p::ccmap->getWidth();
                                for(x = type->getXsize()-1; x>=0; x--) {
                                    if ((count) && type->isBlocked(y*type->getXsize()+x)) {
                                        unitandstructmat[curpos+x] &= ~(US_IS_STRUCTURE|structnum);
                                        if( setlr ) {
                                            unitandstructmat[curpos+x] &= ~US_LOWER_RIGHT;
                                        }
                                        --count;
                                    }
                                }
                            }
                        }
                        return;
                    }
                    unitandstructmat[curpos+x] = US_IS_STRUCTURE|structnum;
                    if( !setlr ) {
                        unitandstructmat[curpos+x] |= US_LOWER_RIGHT;
                        setlr=true;
                    }
                    ++count;
                }
            }
        }
    }
    st = new Structure(type, cellpos, owner, health, facing);
    st->setStructnum(structnum);
    if( structnum == structurepool.size() ) {
        structurepool.push_back(st);
    } else {
        structurepool[structnum] = st;
        numdeletedstruct--;
    }

    // update the wall-images
    if( type->isWall() ) {
        updateWalls(st,true);
    } else {
        if (makeanim) {
            st->runAnim(0);
        } else {
            if( (type->getAnimInfo().animtype == 1) || (type->getAnimInfo().animtype == 4) ) {
                st->runAnim(1);
            }
        }
    }
}


/// Creates a unit
void UnitAndStructurePool::createUnit(const char *typen, Uint16 cellpos,
        Uint8 subpos, Uint8 owner, Uint16 health, Uint8 facing)
{
    Uint32 unitnum;

    UnitType *type;
    Unit *un;
    if (cellpos > (p::ccmap->getWidth() * p::ccmap->getHeight())) {
        logger->warning("Attempted to create a unit at %i, outside map\n",cellpos);
        return;
    }
    if (getStructureAt(cellpos) != NULL) {
        //fprintf(stderr,"cell already occupied by structure\n");
        return;
    }
    if (getUnitAt(cellpos,subpos) != NULL) {
        //fprintf(stderr,"cell/subpos already occupied!\n");
        return;
    }
    //   int x, y, curpos;
    if( numdeletedunit > 0 ) {
        for( unitnum = 0; unitnum < unitpool.size(); unitnum++ ) {
            if(unitpool[unitnum] == NULL)
                break;
        }
    } else {
        unitnum = unitpool.size();
    }

    InfantryGroup *group;

    type = getUnitTypeByName(typen);
    if (type == NULL) {
        logger->warning("Could not create unit of type \"%s\".\n",typen);
        return;
    }

    group = NULL;
    if( type->isInfantry() ) {
        if(unitandstructmat[cellpos]&US_IS_UNIT)
            group = unitpool[unitandstructmat[cellpos]&0xffff]->getInfantryGroup();
        else
            group = new InfantryGroup();
    }

    un = new Unit(type, cellpos, subpos, group, owner, health, facing);
    unitandstructmat[cellpos] = unitnum;
    unitandstructmat[cellpos] |= US_LOWER_RIGHT|US_IS_UNIT;
    /* curpos = cellpos;
     for( y = 0; y < type->getSize(); y++ ){
      for(x = 0; x < type->getSize(); x++){
       unitandstructmat[curpos+x] |= US_IS_UNIT;
      }
      curpos += mapwidth;
     }*/
    un->setUnitnum(unitnum);
    if( unitnum == unitpool.size() ) {
        unitpool.push_back(un);
    } else {
        unitpool[unitnum] = un;
        numdeletedunit--;
    }
}

Unit *UnitAndStructurePool::getUnitAt(Uint32 cell, Uint8 subcell)
{
    Unit *un;
    if( !(unitandstructmat[cell] & US_IS_UNIT) )
        return NULL;
    un = unitpool[unitandstructmat[cell]&0xffff];
    if( ((UnitType *)un->getType())->isInfantry() )
        return un->getInfantryGroup()->unitAt(subcell);
    return un;

}

Unit* UnitAndStructurePool::getUnit(Uint32 num)
{
    return unitpool[num];
}

/** @brief wrapper function that assumes that walls are not wanted
 * @param cell the cell to be examined for structures 
 * @returns a pointer to the Unit if found */
Structure* UnitAndStructurePool::getStructureAt(Uint32 cell)
{
    return getStructureAt(cell,false);
}

/** @brief retrieves the structure at a given cell (NULL if there is no structure).
 * @param cell the cell to be examined for structures
 * @param wall if false, will return NULL if a wall is found in the cell 
 * @returns a pointer to the Structure if found */
Structure *UnitAndStructurePool::getStructureAt(Uint32 cell, bool wall)
{
    if( !(unitandstructmat[cell] & US_IS_STRUCTURE) &&
            !(wall && (unitandstructmat[cell]&US_IS_WALL)))
        return NULL;
    return structurepool[unitandstructmat[cell]&0xffff];

}

Structure* UnitAndStructurePool::getStructure(Uint32 num)
{
    return structurepool[num];
}

/** @brief retrieves units or structures from a given cell like getUnitAt and
 * getStructureAt except works for either.
 * @returns a pointer to the UnitOrStructure at cell.
 * @param cell The cell to be examined.
 * @param subcell The subposition of the cell to be examined (only valid for infantry).  Bitwise or with 128 to get the nearest infantry unit to that subpos.
 * @param wall Whether or not to check for walls as well.
 */
UnitOrStructure* UnitAndStructurePool::getUnitOrStructureAt(Uint32 cell, Uint8 subcell, bool wall)
{
    Unit* un;

    Uint32 cval = unitandstructmat[cell];
    if (cval & US_IS_UNIT) {
        un = unitpool[unitandstructmat[cell]&0xffff];
        if (((UnitType *)un->getType())->isInfantry()) {
            UnitOrStructure* tmp = (UnitOrStructure*)un->getInfantryGroup()->unitAt(subcell&(0xf));
            if (tmp == 0 && subcell & 0x80) {
                return un->getInfantryGroup()->getNearest(subcell&(0xf));
            }
            return tmp;
        }
        return (UnitOrStructure*)un;
    } else if (cval & (US_IS_STRUCTURE|(wall?US_IS_WALL:0)) ) {
        return (UnitOrStructure*)structurepool[unitandstructmat[cell]&0xffff];
    } else {
        return NULL;
    }
}

/// retrieves infantry group from a given cell
InfantryGroup* UnitAndStructurePool::getInfantryGroupAt(Uint32 cell)
{
    Unit* un;
    if (unitandstructmat[cell] & US_IS_UNIT) {
        un = unitpool[unitandstructmat[cell]&0xffff];
        if( ((UnitType *)un->getType())->isInfantry() ) {
            return un->getInfantryGroup();
        }
    }
    return NULL;
}

Uint16 UnitAndStructurePool::getSelected(Uint32 pos)
{
    int i;
    Uint16 selected;

    if( !(unitandstructmat[pos]&US_LOWER_RIGHT) )
        return 0;
    if( unitandstructmat[pos]&US_IS_STRUCTURE ) {
        if(structurepool[unitandstructmat[pos]&0xffff]->isSelected())
            return (((StructureType *)structurepool[unitandstructmat[pos]&0xffff]->getType())->getXsize()<<8) | 1;
    } else if( unitandstructmat[pos]&US_IS_UNIT ) {
        if(((UnitType *)unitpool[unitandstructmat[pos]&0xffff]->getType())->isInfantry()) {
            selected = 0xff00;
            for( i = 0; i < 5; i++ )
                if( !unitpool[unitandstructmat[pos]&0xffff]->getInfantryGroup()->isClear(i) )
                    if(unitpool[unitandstructmat[pos]&0xffff]->getInfantryGroup()->unitAt(i)->isSelected() )
                        selected |= 1<<i;

            return selected;
        } else if(unitpool[unitandstructmat[pos]&0xffff]->isSelected())
            return (1<<8) | 1;
    }
    return 0;
}

/** called by MoveAnimEvent before moving to set up the unitandstructure matrix
 * @param un the unit about to move
 * @param dir the direction in which the unit is to move
 * @param xmod the modifier used to adjust the xoffset (set in this function)
 * @param ymod the modifier used to adjust the yoffset (set in this function)
 * @returns the cell of the new position */
Uint16 UnitAndStructurePool::preMove(Unit *un, Uint8 dir, Sint8 *xmod, Sint8 *ymod)
{
    Uint16 newpos;
    Sint8 unitmod = ((UnitType *)un->getType())->getMoveMod();

    switch(dir) {
    case Unit::m_up:
        newpos = un->getPos()-p::ccmap->getWidth();
        *xmod = 0;
        *ymod = -unitmod;
        break;
    case Unit::m_upright:
        newpos = un->getPos()-p::ccmap->getWidth()+1;
        *xmod = unitmod;
        *ymod = -unitmod;
        break;
    case Unit::m_right:
        newpos = un->getPos()+1;
        *xmod = unitmod;
        *ymod = 0;
        break;
    case Unit::m_downright:
        newpos = un->getPos()+p::ccmap->getWidth()+1;
        *xmod = unitmod;
        *ymod = unitmod;
        break;
    case Unit::m_down:
        newpos = un->getPos()+p::ccmap->getWidth();
        *xmod = 0;
        *ymod = unitmod;
        break;
    case Unit::m_downleft:
        newpos = un->getPos()+p::ccmap->getWidth()-1;
        *xmod = -unitmod;
        *ymod = unitmod;
        break;
    case Unit::m_left:
        newpos = un->getPos()-1;
        *xmod = -unitmod;
        *ymod = 0;
        break;
    case Unit::m_upleft:
        newpos = un->getPos()-p::ccmap->getWidth()-1;
        *xmod = -unitmod;
        *ymod = -unitmod;
        break;
    default:
        return POS_INVALID;
        break;
    }
    // this is needed since tiles in fog have cost 1 in the pathfinder
    if( p::ccmap->getCost(newpos, un) > 0xf000 ) {
        return POS_INVALID;
    }
    /* if an infantry's position got updated */
    if( unitandstructmat[newpos]&(US_IS_WALL|US_IS_STRUCTURE|US_IS_UNIT|US_MOVING_HERE) ) {
        if( !((UnitType *)un->getType())->isInfantry() ) {
            // remove this later when code for moving over walls and infantry is done
            return POS_INVALID;
        }
        if (unitandstructmat[newpos]&(US_IS_WALL)) {
            /** @todo check for tracked and wall type to allow over running some walls
             */
            return POS_INVALID;
        }
        if (unitandstructmat[newpos]&(US_IS_STRUCTURE)) {
            return POS_INVALID;
        }
        if( unitandstructmat[newpos]&US_IS_UNIT ) {
            if( !((UnitType *)unitpool[unitandstructmat[newpos]&0xffff]->getType())->isInfantry() ) {
                /** @todo infantry squishing, check moving unit's type for tracked and accept
                 *  this cell as valid if so.
                 **/
                return POS_INVALID;
            }
            if ( ((Unit* )unitpool[unitandstructmat[newpos]&0xffff])->getOwner() != un->getOwner()) {
                // do not allow units of different sides to occupy same cell
                // this is not allowed because apart from looking weird,
                // area of effect weapons (e.g. flame thrower) will take
                // themselves with their target(s).
                return POS_INVALID;
            }
            if( (((unitandstructmat[newpos]&US_MOVING_HERE)>>24) +
                    unitpool[unitandstructmat[newpos]&0xffff]->getInfantryGroup()->getNumInfantry()) >= 5)
                return POS_INVALID;

        } else if(((unitandstructmat[newpos]&US_MOVING_HERE)>>24) >= 5)
            // more than five infantry in current cell
            return POS_INVALID;

    }

    if( ((UnitType *)un->getType())->isInfantry() )
        unitandstructmat[newpos] += 0x1000000;
    else
        unitandstructmat[newpos] |= US_MOVING_HERE;

    return newpos;
}

/** Called when a unit has moved into a new cell
 * @param un the unit
 * @param newpos the cell into which the unit has moved 
 * @returns the new sub position for the unit (only needed for infantry) */
Uint8 UnitAndStructurePool::postMove(Unit *un, Uint16 newpos)
{
    Uint8 subpos = 0;

    subpos = unhideUnit(un,newpos,false);

    p::ppool->getPlayer(un->getOwner())->movedUnit(un->getPos(), newpos, un->getType()->getSight());

    if( ((UnitType *)un->getType())->isInfantry() ) {
        unitandstructmat[newpos] -= 0x1000000;
    } else {
        // clear values from old position
        unitandstructmat[un->getPos()] &= ~(US_LOWER_RIGHT|US_IS_UNIT);
    }
    return subpos;
}

Uint8 UnitAndStructurePool::unhideUnit(Unit* un, Uint16 newpos, bool unload)
{
    Uint8 subpos = 0;
    Uint8 i;
    InfantryGroup *ig;

    if( ((UnitType *)un->getType())->isInfantry() ) {
        subpos = un->getSubpos();
        ig = un->getInfantryGroup();
        if (!unload) {
            if( ig->getNumInfantry() == 1 ) {
                // old cell is now empty
                unitandstructmat[un->getPos()] &= ~(US_IS_UNIT|US_LOWER_RIGHT);
                delete un->getInfantryGroup();
            } else {
                ig->removeInfantry(subpos);
                if( un->getNum() == (unitandstructmat[un->getPos()]&0xffff) ) {
                    unitandstructmat[un->getPos()]&=0xffff0000;
                    for( i = 0; i < 5; i++ )
                        if( !ig->isClear(i) )
                            break;
                    unitandstructmat[un->getPos()]|=ig->unitAt(i)->getNum();
                }
            }
        }
        /* check indirectly for infantry group
         if the new cell has the US_IS_UNIT flag set, it is assumed
         that there is an infantry group for that cell */
        if( unitandstructmat[newpos] & US_IS_UNIT ) {
            ig = unitpool[unitandstructmat[newpos]&0xffff]->getInfantryGroup();
            // search for an empty sub position for the unit
            for( subpos = 0; subpos < 5; subpos++ )
                if( ig->isClear(subpos) )
                    break;
        } else { // infantry group does not exist for this cell yet
            ig = new InfantryGroup();
            unitandstructmat[newpos] &= 0xffff0000;
            unitandstructmat[newpos] |= un->getNum();
            // note that the subpos variable stays zero (from start of function)
        }
        ig->addInfantry(un, subpos);
        un->setInfantryGroup(ig);
        unitandstructmat[newpos] |= US_LOWER_RIGHT|US_IS_UNIT;
    } else {
        /* easier to assign directly than bitwise AND the compliment of US_MOVING_HERE
         then bitwise OR this value. */
        unitandstructmat[newpos] = US_LOWER_RIGHT|US_IS_UNIT|un->getNum();
    }

    return subpos;
}

void UnitAndStructurePool::hideUnit(Unit* un)
{
    InfantryGroup* ig;
    if ( ((UnitType*)un->getType())->isInfantry() ) {
        ig = un->getInfantryGroup();
        if (ig->getNumInfantry() == 1) {
            // old cell is now empty
            unitandstructmat[un->getPos()] &= ~(US_IS_UNIT|US_LOWER_RIGHT);
            delete un->getInfantryGroup();
        } else {
            ig->removeInfantry(un->getSubpos());
        }
    } else {
        unitandstructmat[un->getPos()] &= ~(US_IS_UNIT|US_LOWER_RIGHT);
    }
}

/** resets the US_MOVING_HERE flag of a cell when the unit stops
 *  before it reaches its destination.
 *  @param un the unit stopping.
 *  @param pos the position to which the unit was moving before stopping.
 **/

void UnitAndStructurePool::abortMove(Unit* un, Uint32 pos)
{
    //fprintf(stderr,"Aborting move by %p to %u\n",un,pos);
    if( !(unitandstructmat[pos] & US_MOVING_HERE) ) {
        return;
    }
    if (((UnitType *)un->getType())->isInfantry()) {
        //fprintf(stderr,"abMi x == %x\n",((unitandstructmat[pos] & 0x7000000) >> 24));
        unitandstructmat[pos] -= 0x1000000;
    } else {
        //if (!(unitandstructmat[pos] & US_MOVING_HERE)) {
        //fprintf(stderr,"Doing nothing\n");
        //} else {
        unitandstructmat[pos] &= ~(US_MOVING_HERE);
        //fprintf(stderr,"reset: %x\n",unitandstructmat[pos]);
        //}
    }
}

Uint16 UnitAndStructurePool::getTileCost(Uint16 pos, Unit* excpUn = 0) const
{
    Unit *un;
    if (unitandstructmat[pos] & (US_IS_WALL|US_IS_STRUCTURE) )
        return 0xfff0;
    if( unitandstructmat[pos] & US_MOVING_HERE ) {
        return 2;
    }
    if( unitandstructmat[pos] & US_IS_UNIT ) {
        un = unitpool[unitandstructmat[pos]&0xffff];
        if (un == excpUn)
            return 0;
        if( un->getOwner() == costcalcowner )
            return 2;
        return 10;
    }
    return 0;
}

bool UnitAndStructurePool::tileAboutToBeUsed(Uint16 pos) const
{
    return (unitandstructmat[pos] & US_MOVING_HERE)!=0;
}

/*
Uint16 *UnitAndStructurePool::getCostMatrix(Uint8 unittype)
{
   Uint16 lastpos = p::ccmap->getWidth()*p::ccmap->getHeight();
   Uint16 i;
   Uint16 *cost = new Uint16[lastpos];
   
   for( i = 0; i < lastpos; i++ ){
      if( unitandstructmat[i] & (US_IS_WALL|US_IS_UNIT|US_IS_STRUCTURE) )
 cost[i] = 0xffff;
      else
 cost[i] = 1;
 //cost = p::ccmap->getCost(i);
   }
   return cost;
}
*/

/** @brief searches the UnitType pool for a unit type with a given name.
 *  if the type can not be found, it is read in from units.ini
 * @param unitname the name of the unit to retrieve 
 * @returns pointer to the UnitType value */
UnitType* UnitAndStructurePool::getUnitTypeByName(const char* unitname)
{
    map<string, Uint16>::iterator typeentry;
    UnitType* type;
    Uint16 typenum;
    string uname = (string)unitname;
    transform(uname.begin(),uname.end(), uname.begin(), toupper);

    typeentry = unitname2typenum.find(uname);

    if( typeentry != unitname2typenum.end() ) {
        typenum = typeentry->second;
        type = unittypepool[typenum];
    } else {
        typenum = unittypepool.size();
        type = new UnitType(uname.c_str(), unitini);
        unittypepool.push_back(type);
        unitname2typenum[uname] = typenum;
    }
    if (type->isValid()) {
        return type;
    }
    return NULL;
}

/** @brief same as getUnitTypeByName but for structures (and the ini file is structure.ini)
 * @param structname the name of the structure to retrieve (e.g. FACT or PROC)
 * @returns pointer to the StructureType value */
StructureType *UnitAndStructurePool::getStructureTypeByName(const char* structname)
{
    Uint16 typenum;
    map<string, Uint16>::iterator typeentry;
    string sname = (string)structname;
    transform(sname.begin(),sname.end(),sname.begin(),toupper);

    StructureType *type;

    typeentry = structname2typenum.find(sname);

    if( typeentry != structname2typenum.end() ) {
        typenum = typeentry->second;
        type = structuretypepool[typenum];
    } else {
        typenum = structuretypepool.size();
        type = new StructureType(structname, structini, artini, theaterext);
        structuretypepool.push_back(type);
        structname2typenum[sname] = typenum;
    }
    if (type->isValid()) {
        return type;
    }
    return NULL;
}

UnitOrStructureType* UnitAndStructurePool::getTypeByName(const char* typen)
{
    UnitOrStructureType* retval;
    /// @BUG: This gives an error message
    retval = getUnitTypeByName(typen);
    if (retval == NULL) {
        return getStructureTypeByName(typen);
    }
    return retval;
}

/// removes a unit from the map
void UnitAndStructurePool::removeUnit(Unit *un)
{
    int i;
    InfantryGroup *infgrp;
    unitpool[un->getNum()] = NULL;
    if( ((UnitType *)un->getType())->isInfantry() ) {
        infgrp = un->getInfantryGroup();
        infgrp->removeInfantry(un->getSubpos());
        if( infgrp->getNumInfantry() == 0 ) {
            delete infgrp;
            unitandstructmat[un->getPos()] &= ~(US_LOWER_RIGHT|US_IS_UNIT);
        } else if( (unitandstructmat[un->getPos()]&0xffff) == un->getNum() ) {
            for( i = 0; i < 5; i++ ) {
                if(!infgrp->isClear(i)) {
                    unitandstructmat[un->getPos()]&=~0xffff;
                    unitandstructmat[un->getPos()]|=infgrp->unitAt(i)->getNum();
                    break;
                }
            }
        }
        un->setInfantryGroup(NULL);
    } else {
        unitandstructmat[un->getPos()] &= ~(US_LOWER_RIGHT|US_IS_UNIT);
    }
    numdeletedunit++;
    deleted_unitorstruct = true;
    un->remove();
    //if numdeletedunit > some_value then pack the unitpool
}

/// removes a structure from the map
void UnitAndStructurePool::removeStructure(Structure *st)
{
    Uint16 curpos, x, y;

    // unitandstructmat[st->getPos()] &= ~(US_LOWER_RIGHT|US_IS_STRUCTURE);
    structurepool[st->getNum()] = NULL;
    curpos = st->getPos();
    if (((StructureType*)st->getType())->isWall()) {
        updateWalls(st,false);
        unitandstructmat[curpos] &= ~(US_LOWER_RIGHT|US_IS_WALL);
    } else {
        for( y = 0; y<((StructureType *)st->getType())->getYsize(); y++ ) {
            for(x = 0; x<((StructureType *)st->getType())->getXsize(); x++) {
                if( ((StructureType *)st->getType())->isBlocked(y*((StructureType *)st->getType())->getXsize()+x) ) {
                    unitandstructmat[curpos+x] &= ~(US_LOWER_RIGHT|US_IS_STRUCTURE);
                }
            }
            curpos += p::ccmap->getWidth();
        }
        numdeletedstruct++; // don't count walls
    }
    deleted_unitorstruct = true;
    st->remove();
    //if numdeletedstruct > some_value then pack the structurepool
}

/** @brief scans neighbouring cells of a wall for walls and updates their
 * layer zero image
 * @param st pointer to the wall to scan around
 * @param add if true, the wall has been added, if false, the wall has been
 * removed */
void UnitAndStructurePool::updateWalls(Structure* st, bool add
                                          )
{
    Structure* neighbour;
    StructureType* type;
    int cellpos;

    cellpos = st->getPos();
    type = ((StructureType*)st->getType());
    // left
    if(cellpos%p::ccmap->getWidth() > 0) {
        if( unitandstructmat[cellpos -1] & US_IS_WALL ) {
            // check if same type
            neighbour = structurepool[unitandstructmat[cellpos -1]&0xffff];
            if( neighbour->getType() == type ) {
                if (add
                   ) {
                    neighbour->changeImage(0, 2);
                    st->changeImage(0, 8);
                }
                else {
                    neighbour->changeImage(0,-2);
                }
            }
        }
    }
    // right
    if(cellpos%p::ccmap->getWidth() < p::ccmap->getWidth() - 1) {
        if( unitandstructmat[cellpos +1] & US_IS_WALL ) {
            // check if same type
            neighbour = structurepool[unitandstructmat[cellpos +1]&0xffff];
            if( neighbour->getType() == type ) {
                if (add
                   ) {
                    neighbour->changeImage(0, 8);
                    st->changeImage(0, 2);
                }
                else {
                    neighbour->changeImage(0,-8);
                }
            }
        }
    }
    // up
    if(cellpos/p::ccmap->getWidth() > 0) {
        if( unitandstructmat[cellpos -p::ccmap->getWidth()] & US_IS_WALL ) {
            // check if same type
            neighbour = structurepool[unitandstructmat[cellpos -p::ccmap->getWidth()]&0xffff];
            if( neighbour->getType() == type ) {
                if (add
                   ) {
                    neighbour->changeImage(0, 4);
                    st->changeImage(0, 1);
                }
                else {
                    neighbour->changeImage(0,-4);
                }
            }
        }
    }
    // down
    if(cellpos/p::ccmap->getWidth() < p::ccmap->getHeight() - 1) {
        if( unitandstructmat[cellpos +p::ccmap->getWidth()] & US_IS_WALL ) {
            // check if same type
            neighbour = structurepool[unitandstructmat[cellpos +p::ccmap->getWidth()]&0xffff];
            if( neighbour->getType() == type ) {
                if (add
                   ) {
                    neighbour->changeImage(0, 1);
                    st->changeImage(0, 4);
                }
                else {
                    neighbour->changeImage(0,-1);
                }
            }
        }
    }
}

/// for debugging the movement code
void UnitAndStructurePool::showMoves()
{
    Uint32 x;
    logger->note("Current cells have US_MOVING_HERE set:\n"
                 "cell\tvalue (US_MOVING_HERE == %u/%x)\n",US_MOVING_HERE,US_MOVING_HERE);
    for (x=0;x < (Uint32)p::ccmap->getWidth()*p::ccmap->getHeight();++x) {
        if (unitandstructmat[x]&US_MOVING_HERE)
            logger->note("%i\t%u/%x\n",x,unitandstructmat[x],unitandstructmat[x]);
    }
}

void UnitAndStructurePool::addPrerequisites(UnitType* unittype)
{
    vector<StructureType*>* type_prereqs;
    if (unittype == NULL)
        return;
    vector<char*> prereqs = unittype->getPrereqs();

    if (prereqs.empty()) {
        logger->warning("No prerequisites for unit \"%s\"\n",unittype->getTName());
        return;
    }
    if (strcasecmp(prereqs[0],"none") == 0) {
        return;
    }
    for (Uint16 x=0;x<prereqs.size();++x) {
        type_prereqs = new vector<StructureType*>;
        splitORPreReqs(prereqs[x],type_prereqs);
        unit_prereqs.insert(make_pair(unittype,type_prereqs));
    }
}

void UnitAndStructurePool::addPrerequisites(StructureType* structtype)
{
    vector<StructureType*>* type_prereqs;
    if (structtype == NULL)
        return;
    vector<char*> prereqs = structtype->getPrereqs();

    if (prereqs.empty()) {
        logger->warning("No prerequisites for structure \"%s\".\n"
                        "Use \"none\" if this intended.\n",structtype->getTName());
        return;
    }
    if (strcasecmp(prereqs[0],"none") == 0) {
        return;
    }
    for (Uint16 x=0;x<prereqs.size();++x) {
        type_prereqs = new vector<StructureType*>;
        splitORPreReqs(prereqs[x],type_prereqs);
        struct_prereqs.insert(make_pair(structtype,type_prereqs));
    }
}
void UnitAndStructurePool::splitORPreReqs(const char* prereqs, vector<StructureType*>* type_prereqs)
{
    char tmp[16];
    Uint32 i, i2;
    if (strcasecmp("none",prereqs) == 0) {
        return;
    }
    memset(tmp,0,16);
    for (i=0,i2=0;prereqs[i]!=0x0;++i) {
        if ( (i2>=1024) || (tmp != NULL && (prereqs[i] == '|')) ) {
            type_prereqs->push_back(getStructureTypeByName(tmp));
            memset(tmp,0,16);
            i2 = 0;
        } else {
            tmp[i2] = toupper(prereqs[i]);
            ++i2;
        }
    }
    type_prereqs->push_back(getStructureTypeByName(tmp));
}

void UnitAndStructurePool::preloadUnitAndStructures(Uint8 techlevel)
{
    //char* line;
    string secname;
    Uint8 ltech;
    Uint32 secnum;
    //unitini->pushLocation();
    //unitini->seekStart();
    //line = unitini->getLine(true);

    try {
        for(secnum = 0;;secnum++) {
            secname = unitini->readSection(secnum);
            ltech = unitini->readInt(secname.c_str(),"techlevel",100);
            if (ltech == 100) {
//                logger->warning("No techlevel defined for unit \"%s\"\n",secname.c_str());
            } else {
                if (p::ccmap->getGameMode() == 0) {
                    if (ltech <= techlevel) {
                        addPrerequisites(getUnitTypeByName(secname.c_str()));
                    }
                } else {
                    if (ltech < 99) {
                        addPrerequisites(getUnitTypeByName(secname.c_str()));
                    }
                }
            }
            //delete[] line;
            //line = unitini->getLine(true);
        }
    } catch(int) {}
    //unitini->popLocation();

    //structini->pushLocation();
    //structini->seekStart();
    //line = structini->getLine(true);
    try {
        for (secnum = 0;;secnum++) {
            secname = structini->readSection(secnum);
            ltech = structini->readInt(secname.c_str(),"techlevel",100);
            if (ltech == 100) {
//                logger->warning("No techlevel defined for structure \"%s\"\n",secname.c_str());
            } else {
                if (p::ccmap->getGameMode() == 0) {
                    if (ltech <= techlevel) {
                        addPrerequisites(getStructureTypeByName(secname.c_str()));
                    }
                } else {
                    if (ltech < 99) {
                        addPrerequisites(getStructureTypeByName(secname.c_str()));
                    }
                }
            }
            //delete[] line;
            //line = structini->getLine(true);
        }
    } catch(int) {
    }
    //structini->popLocation();
}

void UnitAndStructurePool::generateProductionGroups() {
    for (vector<UnitType*>::iterator ut = unittypepool.begin(); ut != unittypepool.end(); ++ut) {
        vector<StructureType*> options;
        vector<char*> nopts = (*ut)->getPrereqs();
        splitORPreReqs(nopts[0], &options);
        if (0 == options.size()) {
            continue;
        }
        Uint32 ptype = 1+(*ut)->getType();
        (*ut)->setPType(ptype);
        for (vector<StructureType*>::iterator st = options.begin();
                st != options.end(); ++st) {
            (*st)->setPType(ptype);
        }
    }
}

vector<const char*> UnitAndStructurePool::getBuildableUnits(Player* pl)
{
    vector<const char*> retval;
    vector<StructureType*> prereqs;
    typedef multimap<UnitType*, vector<StructureType*>* >::const_iterator I;
    pair<I,I> b;
    Uint32 x,y;
    UnitType* utype;
    bool ivalid, ovalid, buildall;

    buildall = pl->canBuildAll();
    for (x=0;x<unittypepool.size();++x) {
        utype = unittypepool[x];
        if (!utype->isValid())
            continue;
        b = unit_prereqs.equal_range(utype);
        ovalid = true;
        if (buildall) {
            if (strlen(utype->getTName()) < 5) {
                retval.push_back(utype->getTName());
            }
            continue;
        }
        if ( ( (utype->getBuildlevel() < 99) && (p::ccmap->getGameMode() != 0)) ||
                (utype->getBuildlevel() <= p::ccmap->getMissionData().buildlevel) ) {
            for (I i = b.first; i != b.second; ++i) {
                // need all of these
                prereqs = *(i->second);
                ivalid = false;
                for (y = 0;y < prereqs.size() ; ++y) {
                    // need just one of these
                    if (pl->ownsStructure(prereqs[y])) {
                        ivalid = true;
                        break;
                    }
                }
                if (ovalid) {
                    ovalid = ivalid;
                }
            }
            if (ovalid) {
                int localPlayerSide, curside;
                char* tmpname;
                localPlayerSide = p::ppool->getLPlayer()->getSide();
                localPlayerSide = PS_BAD;
                for (y=0;y<utype->getOwners().size();++y) {
                    tmpname = utype->getOwners()[y];
                    // note: should avoid hardcoded side names
                    if (strcasecmp(tmpname,"gdi") == 0) {
                        curside = PS_GOOD;
                    } else if (strcasecmp(tmpname,"nod") == 0) {
                        curside = PS_BAD;
                    } else {
                        curside = PS_NEUTRAL;
                    }
                    if (curside == (localPlayerSide&~PS_MULTI)) {
                        retval.push_back(utype->getTName());
                        break;
                    }
                }
            }
        }
    }
    return retval;
}

vector<const char*> UnitAndStructurePool::getBuildableStructures(Player* pl)
{
    vector<const char*> retval;
    vector<StructureType*> prereqs;
    typedef multimap<StructureType*, vector<StructureType*>* >::const_iterator I;
    pair<I,I> b;
    Uint32 x,y;
    bool ivalid, ovalid, buildall;
    StructureType* stype;

    buildall = pl->canBuildAll();
    for (x=0;x<structuretypepool.size();++x) {
        stype = structuretypepool[x];
        if (!stype->isValid())
            continue;
        b = struct_prereqs.equal_range(stype);
        ovalid = true;
        if (buildall) {
            if (strlen(stype->getTName()) < 5) {
                retval.push_back(stype->getTName());
            }
            continue;
        }
        if ( ( (stype->getBuildlevel() < 99) && (p::ccmap->getGameMode() != 0)) ||
                (stype->getBuildlevel() <= p::ccmap->getMissionData().buildlevel)) {
            for (I i = b.first; i != b.second; ++i) {
                // need all of these
                prereqs = *(i->second);
                ivalid = false;
                for (y = 0;y < prereqs.size() ; ++y) {
                    // need just one of these
                    if (pl->ownsStructure(prereqs[y])) {
                        ivalid = true;
                        break;
                    }
                }
                if (ovalid) {
                    ovalid = ivalid;
                }
            }
            if (ovalid) {
                int localPlayerSide, curside;
                char* tmpname;
//                localPlayerSide = (p::ppool->getLPlayer()->getSide())&~PS_MULTI;
                localPlayerSide = PS_BAD;
                for (y=0;y<stype->getOwners().size();++y) {
                    tmpname = stype->getOwners()[y];
                    // note: should avoid hardcoded side names
                    if (strcasecmp(tmpname,"gdi") == 0) {
                        curside = PS_GOOD;
                    } else if (strcasecmp(tmpname,"nod") == 0) {
                        curside = PS_BAD;
                    } else {
                        curside = PS_NEUTRAL;
                    }
                    if (curside == localPlayerSide) {
                        retval.push_back(stype->getTName());
                        break;
                    }
                }
            }
        }
    }
    return retval;
}

Talkback* UnitAndStructurePool::getTalkback(const char* talkback)
{
    Talkback* tb;
    map<string, Talkback*>::iterator typeentry;
    string tname = (string)talkback;
    string::iterator p = tname.begin();
    while (p!=tname.end()) {
        *p = toupper(*p);
        ++p;
    }
    typeentry = talkbackpool.find(tname);

    if( typeentry != talkbackpool.end() ) {
        tb = typeentry->second;
    } else {
        tb = new Talkback(tname.c_str(), tbackini);
        talkbackpool[tname] = tb;
    }
    return tb;
}

/* L2Overlay code */

L2Overlay::L2Overlay(Uint8 numimages)
{
    this->numimages = numimages;
    imagenums.resize(numimages);
    xoffsets.resize(numimages);
    yoffsets.resize(numimages);
}

Uint8 L2Overlay::getImages(Uint32** images, Sint8** xoffs, Sint8** yoffs)
{
    Uint8 i;
    *images = new Uint32[numimages];
    *xoffs = new Sint8[numimages];
    *yoffs = new Sint8[numimages];
    for (i=0;i<numimages;++i) {
        (*images)[i] = imagenums[i];
        (*xoffs)[i] = xoffsets[i];
        (*yoffs)[i] = yoffsets[i];
    }
    return numimages;
}
