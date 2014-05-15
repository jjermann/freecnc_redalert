// mode: -*- C++ -*-
#ifndef UNIT_H
#define UNIT_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif
#define BAIL_SIZE 4

#include <map>
#include <vector>
#include <stack>
#include "SDL_types.h"
#include "common.h"
#include "unitanimations.h"
#include "unitorstructure.h"
class INIFile;
class InfantryGroup;
class UnitQueue;
class Weapon;
class WeaponsPool;
class StructureType;
class Talkback;
class L2Overlay;

enum bail_t {bail_empty = 0, bail_ore = 25, bail_gem = 150};
struct Bail {
  bail_t type[BAIL_SIZE];
};
struct Harvested {
  Uint8 fillnum;
  Uint8 bailnum;
  Bail* bails;
};

class UnitType : public UnitOrStructureType {
public:
    UnitType(const char *typeName, INIFile *unitini);
    ~UnitType();
    Uint32 *getSHPNums() {
        return shpnums;
    }
    Uint8 getNumLayers() const {
        return numlayers;
    }
    bool isInfantry() const {
        return is_infantry;
    }
    Uint8 getType() const {
        return unittype;
    }
    const char* getTName() const {
        return tname;
    }
    const char* getName() const {
        return name;
    }
    std::vector<char*> getPrereqs() const {
        return prereqs;
    }
    std::vector<char*> getOwners() const {
        return owners;
    }
    Uint8 getOffset() const {
        return offset;
    }
    Uint8 getSpeed() const {
        return speed;
    }
    Uint8 getROT() const {
        return turnspeed;
    }
    Sint8 getMoveMod() const {
        return movemod;
    }
    Uint8 getTurnMod() const {
        return turnmod;
    }
    Uint16 getMaxHealth() const {
        return maxhealth;
    }
    Uint16 getCost() const {
        return cost;
    }
    Uint8 getTurnspeed() const {
        return turnspeed;
    }
    Uint8 getSight() const {
        return sight;
    }
    armour_t getArmour() const {
        return armour;
    }
    move_t getMoveType() const {
        return movetype;
    }
    animinfo_t getAnimInfo() const {
        return animinfo;
    }

    const char* getRandTalk(TalkbackType type) const;
    Weapon *getWeapon(bool primary = true) const {
        return (primary?primary_weapon:secondary_weapon);
    }
    bool isWall() const {
        return false;
    }
    bool canDeploy() const {
        return deployable;
    }
    bool canHarvest() const {
        return (maxbails>0);
    }
    Uint32 getMaxBails() const {
        return maxbails;
    }
    const char* getRefineStruct() const {
        return refinestrname;
    }
    const char* getDeployTarget() const {
        return deploytarget;
    }
    StructureType* getDeployType() const {
        return deploytype;
    }
    Uint8 getBuildlevel() const {
        return buildlevel;
    }
    Uint8 getTechlevel() const {
        return techlevel;
    }
    // what colour pip should be displayed for this unit when being carried
    Uint8 getPipColour() const {
        return pipcolour;
    }
    Uint8 getMaxPassengers() const {
        return maxpassengers;
    }
    std::vector<Uint8> getPassengerAllow() const {
        return passengerAllow;
    }
    std::vector<UnitType*> getSpecificTypeAllow() const {
        return specificTypeAllow;
    }
private:
    Uint32 *shpnums;
    Uint16 cost,maxhealth;
    Uint8 maxbails;
    Uint8 numlayers,speed,turnspeed,turnmod,sight,offset,pipcolour;
    animinfo_t animinfo;
    armour_t armour;
    move_t movetype;
    Uint8 techlevel,buildlevel,unittype;
    Sint8 movemod;

    char tname[8];
    char* name;
    std::vector<char*> prereqs;
    std::vector<char*> owners;

    // Talkback related members
    Talkback* talkback;

    bool is_infantry, deployable;
    char* deploytarget;
    char* refinestrname;
    // this is used to check the unit can deploy
    StructureType* deploytype;
    Uint8 maxpassengers;
    // matches the unit's type value specified in units.ini
    std::vector<Uint8> passengerAllow;
    // matches the unit's type name.
    std::vector<UnitType*> specificTypeAllow;

    Weapon* primary_weapon;
    Weapon* secondary_weapon;
};

class Unit : public UnitOrStructure {
public:
    friend class UnitAnimEvent;
    friend class MoveAnimEvent;
    friend class HarvestAnimEvent;
    friend class UAttackAnimEvent;
    friend class TurnAnimEvent;
    friend class WalkAnimEvent;
    Unit(UnitType *type, Uint16 cellpos, Uint8 subpos, InfantryGroup *group,
            Uint8 owner, Uint16 rhealth, Uint8 facing);
    ~Unit();
    Uint8 getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets, bool* showpips);
    InfantryGroup *getInfantryGroup() {
        return infgrp;
    }
    void setInfantryGroup(InfantryGroup *ig) {
        infgrp = ig;
    }
    Uint32 getImageNum(Uint8 layer) const {
        return type->getSHPNums()[layer]+imagenumbers[layer];
    }
    void setImageNum(Uint32 num, Uint8 layer);
    Sint8 getXoffset() const; // return xoffset-type->getOffset();
    Sint8 getYoffset() const; // return yoffset-type->getOffset();
    void setXoffset(Sint8 xo);
    void setYoffset(Sint8 yo);
    UnitOrStructureType* getType() {
        return type;
    }
    Uint16 getPos() const {
        return cellpos;
    }
    Uint16 getBPos(Uint16 pos) const {
        return cellpos;
    }
    Uint16 getSubpos() const {
        return subpos;
    }
    Uint32 getNum() const {
        return unitnum;
    }
    void setUnitnum(Uint32 unum) {
        unitnum = unum;
    }
    Uint16 getHealth() const {
        return health;
    }
    void move(Uint16 dest);
    void harvest(Uint16 dest, bool stop=true);
    void move(Uint16 dest, bool stop);
    bool targetinRange();
    bool targetinRange(Uint16 cellpos);
    void attack(UnitOrStructure* target);
    void attack(UnitOrStructure* target, bool stop);
    void turn(Uint8 facing, Uint8 layer);
    void stop();

    Uint8 getOwner() const {
        return owner;
    }
    void setOwner(Uint8 newowner) {
        owner = newowner;
    }
    void remove();
    void applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker);
    bool canAttack() {
        return type->getWeapon()!=NULL;
    }
    void doRandTalk(TalkbackType ttype);
    void deploy(void);
    bool canDeploy();
    bool checkDeployTarget(Uint32 pos);
    Uint32 calcDeployPos() const;
    Uint32 getExitCell() const {
        return calcDeployPos();
    }
    double getRatio() const {
        return ratio;
    }
    bool pushPassenger(Unit* un);
    Unit* removePassenger(Unit* un);
    std::vector<Unit*> getPassengers() {
        return passengers;
    }
    bool canLoad(Unit* un);
    void unloadUnits();
    void loadUnits(std::vector<Unit*> units);
    UnitQueue* getUnitQueue() {
        return unitqueue;
    }
    bool isLoaded() const {
        return (container != NULL);
    }
    void cancelLoad(Unit* un);
    void enterUnitOrStructure(UnitOrStructure* dest);
    void exitUnitOrStructure(UnitOrStructure* source);
    LOADSTATE getLoadState() const {
        return loadstate;
    }
    void resetLoadState();
    Uint16 getDist(Uint16 pos);
    Uint16 getTargetCell();
    bool addHarvest(bail_t type);
    Uint16 getHarvested() {
        Uint16 x=0;
        for (Uint8 i=0; i<=harvested.bailnum; i++) {
           for (Uint8 j=0; j<BAIL_SIZE; j++) {
             x+=harvested.bails[i].type[j];
           }
        }
        return x;
    }
    bool bailsFilled() {
        if (((Uint32)(harvested.bailnum+1)>=type->getMaxBails())
         && (harvested.fillnum+1>=BAIL_SIZE)
         && (harvested.bails[harvested.bailnum].type[harvested.fillnum]!=bail_empty)) {
          return true;
        } else {
          return false;
        }
    }
    void clearHarvest() {
        harvested.bailnum=0;
        harvested.fillnum=0;
        for(Uint8 i=0;i<type->getMaxBails();i++) {
          for(Uint8 j=0;j<BAIL_SIZE;j++) {
            harvested.bails[i].type[j]=bail_empty;
          }
        }
    }
    Structure* getRefineStr() {
        return refinestr;
    }
    bool setNewRefineStr();
    enum movement {m_up = 0, m_upright = 1, m_right = 2, m_downright = 3,
        m_down = 4, m_downleft = 5, m_left = 6, m_upleft = 7};
private:
    UnitType *type;
    Uint32 unitnum;
    Uint16 *imagenumbers;
    Uint16 cellpos,health,palettenum;
    Harvested harvested;
    Uint8 owner,references,subpos;
    Sint8 xoffset,yoffset;
    bool deployed;
    double ratio;

    L2Overlay* l2o;
    std::multimap<Uint16, L2Overlay*>::iterator l2entry;

    Structure* refinestr;
    UnitOrStructure* container;
    std::vector<Unit*> passengers;
    UnitQueue* unitqueue;
    LOADSTATE loadstate;

    InfantryGroup *infgrp;

    MoveAnimEvent* moveanim;
    HarvestAnimEvent* harvestanim;
    UAttackAnimEvent* attackanim;
    WalkAnimEvent* walkanim;
    TurnAnimEvent* turnanim1, *turnanim2;
};

/*
 * This should be a member of unit for infatry. When a infatry unit walks in
 * to a new cell a new group is created, if someone is already there that 
 * group is used. We need one more bit in the unit/structure matrix to tell if
 * a infantry is in that cell (faster). The groups should be destroyed 
 * (or reused by last unit if he travels to a empty cell) when the last
 * infantry leaves. */

class InfantryGroup {
public:
    InfantryGroup() {
        int i;
        numinfantry = 0;
        for(i=0;i<5;i++)
            positions[i] = NULL;
    }
    void addInfantry(Unit *inf, Uint8 subpos) {
        positions[subpos] = inf;
        numinfantry++;
    }
    void removeInfantry(Uint8 subpos) {
        positions[subpos]=NULL;
        numinfantry--;
    }
    bool isClear(Uint8 subpos) {
        return (positions[subpos] == NULL);
    }
    Uint8 getNumInfantry() {
        return numinfantry;
    }
    Unit *unitAt(Uint8 subpos) {
        return positions[subpos];
    }
    Uint8 getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets) {
        int i, j;

        (*inums) = new Uint32[numinfantry];
        (*xoffsets) = new Sint8[numinfantry];
        (*yoffsets) = new Sint8[numinfantry];
        j = 0;
        for( i = 0; i < 5; i++ ) {
            if( positions[i] != NULL ) {
                (*inums)[j]=positions[i]->getImageNum(0);
                (*xoffsets)[j]=positions[i]->getXoffset()+unitoffset[i];
                (*yoffsets)[j]=positions[i]->getYoffset()+unitoffset[i+5];
                j++;
            }
        }
        return numinfantry;
    }
    void getSubposOffsets(Uint8 oldsp, Uint8 newsp, Sint8* xoffs, Sint8* yoffs) {
        *xoffs = unitoffset[oldsp]-unitoffset[newsp];
        *yoffs = unitoffset[oldsp+5]-unitoffset[newsp+5];
    }
    static const Sint8* getunitoffsets() {
        return unitoffset;
    }
    Unit* getNearest(Uint8 subpos);
private:
    Unit* positions[5];
    Uint8 numinfantry;
    static const Sint8 unitoffset[];
};

inline Unit* InfantryGroup::getNearest(Uint8 subpos)
{
    static const Uint8 lut[20] = {
        1,2,3,4,
        3,0,2,4,
        1,0,4,3,
        1,0,4,2,
        3,0,2,1
    };
    Uint8 x;
    /* The compiler will optimise this nicely with -funroll-loops,
     * leaving it like this to keep it readable.
     */
    for (x=0;x<4;++x)
        if (positions[lut[x+subpos*4]] != NULL)
            return positions[lut[x+subpos*4]];
    return NULL;
}

class Talkback {
public:
    Talkback(const char* talkback, INIFile* tbini);
    ~Talkback();
    const char* getRandTalk(TalkbackType type);
private:
    void merge(Talkback* mergee);
    std::vector<char*>* getTypeVector(TalkbackType type, bool ignoresize);
    TalkbackType getTypeNum(const char* name);
    std::vector<char*> tbReport;
    std::vector<char*> tbAck;
    std::vector<char*> tbDie;
    std::vector<char*> tbPostkill;
    std::vector<char*> tbAtkUn;
    std::vector<char*> tbAtkSt;
};

#endif
