// mode: -*- C++ -*-
#ifndef STRUCTURE_H
#define STRUCTURE_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <vector>
#include "SDL_types.h"
#include "common.h"
#include "unitorstructure.h"

class UnitType;
class UnitQueue;
class Weapon;
class WeaponsPool;
class INIFile;

class StructureType : public UnitOrStructureType {
public:
    StructureType(const char* typeName, INIFile* structini, INIFile* artini, 
                  const char* thext);
    ~StructureType();
    Uint16 *getSHPNums() {
        return shpnums;
    }
    Uint16 *getSHPTNum() {
        return shptnum;
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
    Uint8 getNumLayers() const {
        return numshps;
    }

    Uint16 getMakeImg() const {
        return makeimg;
    }
    bool isWall() const {
        return is_wall;
    }

    Uint8 getXsize() const {
        return xsize;
    }
    Uint8 getYsize() const {
        return ysize;
    }

    Uint8 isBlocked(Uint16 tile) const {
        return blocked[tile];
    }

    Sint8 getXoffset() const {
        return xoffset;
    }
    Sint8 getYoffset() const {
        return yoffset;
    }
    Uint8 getOffset() const {
        return 0;
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
    Uint8 getSpeed() const {
        return 0;
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

    powerinfo_t getPowerInfo() const {
        return powerinfo;
    }
    Uint16 getMaxHealth() const {
        return maxhealth;
    }
    Uint16 getMaxStorage() const {
        return maxstorage;
    }
    Weapon *getWeapon(bool primary = true) const {
        return (primary?primary_weapon:secondary_weapon);
    }
    bool hasTurret() const {
        return turret;
    }
    Uint16 getBlckOff() const {
        return blckoff;
    }
    bool isInfantry() const {
        return false;
    }
    Uint8 getNumWallLevels() const {
        return numwalllevels;
    }
    Uint8 getDefaultFace() const {
        return defaultface;
    }
    Uint8 getBuildlevel() const {
        return buildlevel;
    }
    Uint8 getTechlevel() const {
        return techlevel;
    }
    bool primarySettable() const {
        return primarysettable;
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
    Uint16* shpnums, *shptnum;
    Uint16 cost,maxhealth,makeimg,blckoff,maxstorage;
    Sint8 xoffset,yoffset;
    armour_t armour;
    move_t movetype;
    Uint8 turnspeed,sight,xsize,ysize,numshps,numwalllevels,defaultface;
    Uint8 techlevel,buildlevel,maxpassengers;
    Uint8 *blocked;
    char tname[8];
    char* name;
    std::vector<char*> owners;
    std::vector<char*> prereqs;

    Weapon* primary_weapon;
    Weapon* secondary_weapon;

    animinfo_t animinfo;
    powerinfo_t powerinfo;

    bool is_wall,turret, primarysettable;

    // matches the unit's type value specified in units.ini
    std::vector<Uint8> passengerAllow;
    // matches the unit's type name.
    std::vector<UnitType*> specificTypeAllow;
};

class BAttackAnimEvent;

class Structure : public UnitOrStructure
{
public:
    friend class BuildingAnimEvent;
    friend class BAttackAnimEvent;
    friend class RepairAnimEvent;

    Structure(StructureType *type, Uint16 cellpos, Uint8 owner,
            Uint16 rhealth, Uint8 facing);
    ~Structure();
    Uint8 getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets, bool* showpips);
    Uint16* getImageNums() const {
        return imagenumbers;
    }
    void changeImage(Uint8 layer, Sint16 imagechange) {
        imagenumbers[layer]+=imagechange;
    }
    Uint32 getImageNum(Uint8 layer) const {
        return type->getSHPNums()[layer]+imagenumbers[layer];
    }
    void setImageNum(Uint32 num, Uint8 layer);
    UnitOrStructureType* getType() {
        return type;
    }
    void setStructnum(Uint32 stn) {
        structnum = stn;
    }
    Uint32 getNum() const {
        return structnum;
    }
    Uint16 getPos() const {
        return cellpos;
    }
    Uint16 getBPos(Uint16 curpos) const;
    void remove();
    Uint16 getSubpos() const {
        return 0;
    }
    Uint16 getFreePos(UnitType* un);
    void applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker);
    void applyDamage(Sint16 amount);
    void runAnim(Uint32 mode, bool back=false, Uint16 options=0);
    void runSecAnim(Uint32 param);
    void stopAnim();
    void stop();
    Uint8 getOwner() const {
        return owner;
    }
    void setOwner(Uint8 newowner) {
        owner = newowner;
    }
    bool canAttack() const {
        return type->getWeapon()!=NULL;
    }
    bool targetinRange();
    bool targetinRange(Uint16 cellpos);
    void attack(UnitOrStructure* target);
    Uint16 getHealth() const {
        return health;
    }
    Sint8 getXoffset() const {
        return type->getXoffset();
    }
    Sint8 getYoffset() const {
        return type->getYoffset();
    }
    bool isWall() const {
        return type->isWall();
    }
    double getRatio() const {
        return ratio;
    }
    bool isPrimary() const {
        return primary;
    }
    void setPrimary(bool pri) {
        primary = pri;
    }
    std::vector<Unit*> getPassengers() {
        return passengers;
    }
    bool pushPassenger(Unit* un);
    Unit* removePassenger(Unit* un);
    bool canLoad(Unit* un);
    void unloadUnits();
    void loadUnits(std::vector<Unit*> units);
    void cancelLoad(Unit* un);
    bool isLoaded() const {
        return false;
    }
    UnitQueue* getUnitQueue() {
        return unitqueue;
    }
    Uint32 getExitCell() const;
    LOADSTATE getLoadState() const {
        return loadstate;
    }
    void resetLoadState(bool runsec, Uint32 param);
    Uint8 checkdamage();
    Uint16 getTargetCell() const;
    bool repair(bool change);
private:
    StructureType *type;
    Uint32 structnum;
    Uint16 *imagenumbers;
    Uint16 cellpos,bcellpos,health;
    Uint8 owner,references,damaged;
    bool animating,usemakeimgs,exploding,primary;
    double ratio; // health/maxhealth

    BuildingAnimEvent* buildAnim;
    BAttackAnimEvent* attackAnim;
    RepairAnimEvent* repairAnim;
    // used to store units that have been built and are about to leave
    // or units that have entered the building.
    // passengers is used for consistency with the Unit class.
    std::vector<Unit*> passengers;
    UnitQueue* unitqueue;
    LOADSTATE loadstate;
};

#include "structureanims.h"

#endif
