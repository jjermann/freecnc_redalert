// mode: -*- C++ -*-
#ifndef WEAPONSPOOL_H
#define WEAPONSPOOL_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <string>
#include <vector>
#include <map>
#include "SDL.h"
#include "common.h"
#include "structureanims.h"

class INIFile;
class Sound;
class SHPImage;
class Unit;
class UnitOrStructure;
class Structure;
class UnitAndStructurePool;

class WeaponsPool;

class Warhead
{
public:
    Warhead(const char *whname, INIFile *weapini);
    ~Warhead();
    //void getExplosion(Uint32 &image, Uint8 &steps){image = explosionimage; steps = explosionanimsteps;}
    Uint32 getEImage()
    {
        return explosionimage;
    }
    Uint8 getESteps()
    {
        return explosionanimsteps;
    }
    const char *getExplosionsound()
    {
        return explosionsound;
    }
    bool getWall()
    {
        return walls;
    }
    Uint8 getVersus(armour_t armour)
    {
        return versus[(Uint8)armour];
    }
    Uint8 getSpread() const
    {
        return spread;
    }

private:
    // spread - percentage to multiply the damage after going further one more
    //          cell away from the center of the explosion to a maximum cell
    //          range or minimal damage. (default 0: accurate)
            
    //Uint8 explosiontype;
    Uint32 explosionimage;
    Uint8 explosionanimsteps;
    char *explosionsound;
    Uint8 infantrydeath;
    Uint8 blastradius, spread;
    unsigned int versus[5];
    bool walls;
    bool trees;
    //Uint16 damage;
}
;

class Projectile
{
public:
    Projectile(const char *pname, INIFile *weapini);
    ~Projectile();
    Uint32 getImageNum()
    {
        return imagenum;
    }
    //Uint8 getSpeed(){return speed;}
    bool doesRotate()
    {
        return rotates;
    }
private:
    Uint32 imagenum;
    Uint8 rotationimgs;
    bool AA;
    bool AG;
    //Uint8 speed;
    bool high, inacurate, rotates;
};

class Weapon
{
public:
    Weapon(const char* wname);
    ~Weapon();
    Uint16 getReloadTime() const
    {
        return reloadtime;
    }
    Uint16 getRange() const
    {
        return range;
    }
    Uint8 getSpeed() const
    {
        return speed;
    }
    Sint16 getDamage() const
    {
        return damage;
    }
    bool getWall() const
    {
        return whead->getWall();
    }
    Projectile *getProjectile()
    {
        return projectile;
    }
    Warhead *getWarhead()
    {
        return whead;
    }
    void fire(UnitOrStructure* owner, Uint16 target, Uint8 subtarget);
    //Uint32 tmppif;
    bool isHeatseek() const
    {
        return heatseek;
    }
    double getVersus(armour_t armour) const
    {
        return (whead->getVersus(armour))/(double)100.0;
    }
    Uint16 getFuel() const
    {
        return fuel;
    }
    Uint16 getSeekFuel() const
    {
        return seekfuel;
    }
    const char* getName() const
    {
        return name.c_str();
    }
    //HACK: instead of introducing yet another boolean variable charges,
    //      I check the existence of chargesound
    const char* getCharges() const
    {
        return chargesound;
    }
private:
    // Fuel - how many ticks this projectile can move for until being removed.
    // Seekfuel - how many ticks can this projectile change course to track its
    // target before falling back to flying in a straight line.
    Weapon() {};
    Projectile *projectile;
    Warhead *whead;
    Uint8 speed;
    Uint16 range;
    Uint16 reloadtime;
    Sint16 damage;
    Uint8 burst;
    Uint16 fuel, seekfuel;
    bool heatseek;
    Uint32 fireimage;
    Uint32* fireimages;
    Uint8 numfireimages,numfiredirections;
    char *firesound, *chargesound;
    std::string name;
};

class WeaponsPool
{
public:
    friend class Weapon;
    friend class Projectile;
    friend class Warhead;
    WeaponsPool();
    ~WeaponsPool();
    Weapon *getWeapon(const char *wname);
    INIFile* getWeaponsINI()
    {
        return weapini;
    }
private:
    std::map<std::string, Weapon*> weaponspool;
    std::map<std::string, Projectile*> projectilepool;
    std::map<std::string, Warhead*> warheadpool;
    INIFile* weapini;
};

#endif
