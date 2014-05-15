// mode: -*- C++ -*-
#ifndef PROJECTILEANIM_H
#define PROJECTILEANIM_H

#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif

#define MAX_DMG_RANGE 200

#include <map>
#include "SDL.h"
#include "actioneventqueue.h"
#include "structure.h"

class Weapon;
class Unit;
class Structure;
class UnitOrStructure;
class UnitAndStructurePool;
class L2Overlay;

class ExplosionAnim : public ActionEvent
{
public:
    ExplosionAnim(Uint32 p, Uint16 pos, Uint32 startimage, Uint8 animsteps,
                  Sint8 xoff, Sint8 yoff);
    ~ExplosionAnim();
    void run();
private:
    ExplosionAnim() : ActionEvent(1) {};

    L2Overlay *l2o;
    Uint16 pos;
    Uint8 animsteps;
    std::multimap<Uint16, L2Overlay*>::iterator l2entry;
};

class ProjectileAnim : public ActionEvent
{
public:
    ProjectileAnim(Uint32 p, Weapon *weap, UnitOrStructure* owner, Uint16 dest, Uint8 subdest);
    ~ProjectileAnim();
    void run();
    void applyDmg(Uint16 curpos, Sint16 curdmg);
    void applySpreadDmg();
private:
    Weapon* weap;
    UnitOrStructure* owner;
    UnitOrStructure* target;
    std::vector<Structure *> dmgstructs;
    Uint16 dest;
    Uint8 subdest;
    // spread - percentage to multiply the damage after going further one more
    //          cell away from the center of the explosion to a maximum cell
    //          range or minimal damage. (default 0: accurate)
    // Fuel - how many ticks left until projectile is removed.
    // Seekfuel - how many ticks left until this projectile change course
    // to track its target before falling back to flying in a straight line.
    Uint8 spread;
    Uint16 fuel, seekfuel;
    Sint8 xoffset;
    Sint8 yoffset;
    //Sint32 xmod, ymod;
    L2Overlay *l2o;
    std::multimap<Uint16, L2Overlay*>::iterator l2entry;
    double xdiff, ydiff;
    double xmod, ymod, rxoffs, ryoffs;
    bool heatseek,fuelled;
    Uint8 facing;
};

#endif /* PROJECTILEANIM_H */
