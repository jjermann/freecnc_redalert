#include <cstdlib>
#include <cstring>
#include <string>
#include "config.h"
#include "ccmap.h"
#include "inifile.h"
#include "logger.h"
#include "playerpool.h"
#include "shpimage.h"
#include "soundengine.h"
#include "structure.h"
#include "snprintf.h"
#include "unit.h"
#include "unitanimations.h"
#include "unitandstructurepool.h"
#include "unitqueue.h"
#include "weaponspool.h"

using std::vector;
using std::string;

const Sint8 InfantryGroup::unitoffset[10] = {
    /* Theses values have been heavily tested, do NOT change them unless you're
     *        _really_ sure of what you are doing */
    /* X value */
    -13, -19, -7, -19, -7,
    /* Y value */
    -3, -7, -7, 1, 1
};

UnitType::UnitType(const char *typeName, INIFile *unitini) : UnitOrStructureType(), shpnums(0), name(0), deploytarget(0)
{
    SHPImage* shpimage;
    Uint32 i;
    string shpname(typeName);
    char* imagename;
    Uint32 shpnum;
    Uint32 tmpspeed;
    char* talkmode;
    char* miscnames;
    char *tmp;

    // for reading in what units can be used as passengers
    vector<char*> tmpallow;
    UnitType* tmpspecallow;

    deploytarget = NULL;
    // Ensure that there is a section in the ini file
    try {
        unitini->readKeyValue(typeName,0);
    } catch (int) {
//        logger->error("Unknown type: %s\n",typeName);
        name = NULL;
        shpnums = NULL;
        return;
    }

    memset(this->tname,0x0,8);
    strncpy(this->tname,typeName,8);
    name = unitini->readString(tname,"name");
    tmp = unitini->readString(tname,"prerequisites");
    if( tmp != NULL ) {
        prereqs = splitList(tmp,',');
        delete[] tmp;
    }

    unittype = unitini->readInt(tname, "unittype",0);

    miscnames = unitini->readString(tname,"movetype");

    movetype=MT_none;
    if (miscnames == NULL)
        switch (unittype) {
        case 0:
          movetype=MT_foot;
          break; 
        case 1:
          movetype=MT_wheel;
          break;
        case 2:
          movetype=MT_float;
          break;
        case 3:
          movetype=MT_float;
          break;
        case 4:
          movetype=MT_air;
          break;
        case 5:
          movetype=MT_air;
          break;
        }
    else {
        if (strcasecmp(miscnames,"foot") == 0)
          movetype=MT_foot;
        else if (strcasecmp(miscnames,"track") == 0)
          movetype=MT_track;
        else if (strcasecmp(miscnames,"wheel") == 0)
          movetype=MT_wheel;
        else if (strcasecmp(miscnames,"float") == 0)
          movetype=MT_float;
        else if (strcasecmp(miscnames,"air") == 0)
          movetype=MT_air;
        delete[] miscnames;
    }

    animinfo.loopend = unitini->readInt(tname,"loopend",31);
    animinfo.loopend2 = unitini->readInt(tname,"loopend2",31);

    numlayers = unitini->readInt(tname, "layers", 1);

    shpnums = new Uint32[numlayers];

//TODO: This is _wrong_ atm!!!! (only true for numlayers<=1)
    for(i = 0; i < numlayers; i++) {
        asprintf(&imagename, "image%d", i+1);
        tmp = unitini->readString(tname, imagename);
        free(imagename);
        if (0 == tmp) {
            shpname=tname;
            shpname+=".SHP";
        } else {
            shpname=tmp;
            delete[] tmp;
        }
        try {
            shpimage = new SHPImage(shpname.c_str(), mapscaleq);
        } catch (ImageNotFound&) {
            logger->warning("Image not found: \"%s\"\n", shpname.c_str());
            numlayers = 0;
            return;
        }
    }
    shpnum = pc::imagepool->size();
    pc::imagepool->push_back(shpimage);
    shpnum <<= 16;
    for( i = 0; i < numlayers; i++ ) {
        /* get layer offsets from inifile */
        shpnums[i] = shpnum;
        if (i==0) {
            shpnum += (animinfo.loopend+1);
        } else if (i==1) {
            shpnum += (animinfo.loopend2+1);            
        } else {
            shpnum += 32;
        }
    }
    is_infantry = false;

    buildlevel = unitini->readInt(tname,"buildlevel",99);
    techlevel = unitini->readInt(tname,"techlevel",99);

    tmp = unitini->readString(tname, "owners");
    if( tmp != NULL ) {
        owners = splitList(tmp,',');
        delete[] tmp;
    }

    maxbails = unitini->readInt(tname, "maxbails", 0);
    refinestrname = unitini->readString(tname, "refinestructure");

    if (unittype == 0 )
        is_infantry = true;

    tmpspeed = unitini->readInt(tname, "speed");
    if (is_infantry) {
        if (tmpspeed == INIERROR) {
            speed = 4; // default for infantry is slower
            movemod = 1;
        } else {
            speed = (tmpspeed>4)?2:(7-tmpspeed);
            movemod = (tmpspeed>4)?(tmpspeed-4):1;
        }
    } else {
        if (tmpspeed == INIERROR) {
            speed = 2;
            movemod = 1;
        } else {
            speed = (tmpspeed>4)?2:(7-tmpspeed);
            movemod = (tmpspeed>4)?(tmpspeed-4):1;
        }
    }
    if (is_infantry) {
        talkmode = unitini->readString(tname, "talkback","Generic");
        sight = unitini->readInt(tname, "sight", 3);
    } else {
        talkmode = unitini->readString(tname, "talkback","Generic-Vehicle");
        sight = unitini->readInt(tname, "sight", 5);
    }
    talkback = p::uspool->getTalkback(talkmode);
    delete[] talkmode;
    maxhealth = unitini->readInt(tname, "health", 50);
    cost = unitini->readInt(tname, "cost", 0);
    tmpspeed = unitini->readInt(tname, "turnspeed");
    if (tmpspeed == INIERROR) { // no default for infantry as do not turn
        turnspeed = 2;
        turnmod = 1;
    } else {
        turnspeed = (tmpspeed>4)?2:(7-tmpspeed);
        turnmod = (tmpspeed>4)?(tmpspeed-4):1;
    }
    if( is_infantry ) {
        //      size = 1;
        offset = 0;
    } else {
        //size = shpimage->getWidth();
        offset = (shpimage->getWidth()-24)>>1;
    }
    miscnames = unitini->readString(tname, "primary_weapon");
    if( miscnames == NULL ) {
        primary_weapon = NULL;
    } else {
        primary_weapon = p::weappool->getWeapon(miscnames);
        delete[] miscnames;
    }
    miscnames = unitini->readString(tname, "secondary_weapon");
    if( miscnames == NULL ) {
        secondary_weapon = NULL;
    } else {
        secondary_weapon = p::weappool->getWeapon(miscnames);
        delete[] miscnames;
    }
    deploytarget = unitini->readString(tname, "deploysto");
    if (deploytarget != NULL) {
        deployable = true;
        deploytype = p::uspool->getStructureTypeByName(deploytarget);
        maxpassengers = 0;
    } else {
        deploytype = NULL;
        maxpassengers = unitini->readInt(tname, "maxpassengers",0);
        if (maxpassengers > 0) {
            deployable = true;
            tmp = unitini->readString(tname, "passengersallow");
            if( tmp != NULL ) {
                tmpallow = splitList(tmp,',');
                delete[] tmp;
            }
            for (i=0;i<tmpallow.size();++i) {
                // this bit depends on there not being any other unit
                // base types.
                if (tmpallow[i][0] < '4') {
                    passengerAllow.push_back(atoi(tmpallow[i]));
                } else {
                    if (strcasecmp(tname,tmpallow[i]) != 0) {
                        tmpspecallow = p::uspool->getUnitTypeByName(tmpallow[i]);
                        if (tmpspecallow != NULL) {
                            specificTypeAllow.push_back(tmpspecallow);
                        } else {
                            logger->warning("Invalid type name: %s (in %s)\n",tmpallow[i],tname);
                        }
                    } else {
                        specificTypeAllow.push_back(this);
                    }
                }
                delete[] tmpallow[i];
            }
            if (passengerAllow.empty() && specificTypeAllow.empty()) {
                logger->warning("Max passengers set, but no types allowed (%s)\n",tname);
            }
        } else {
            deployable = false;
        }
    }
    pipcolour = unitini->readInt(tname,"pipcolour",0);
    miscnames = unitini->readString(tname,"armour");
    if (miscnames == NULL)
        armour = AC_none;
    else {
        if (strncasecmp(miscnames,"none",4) == 0)
            armour = AC_none;
        else if (strncasecmp(miscnames,"wood",4) == 0)
            armour = AC_wood;
        else if (strncasecmp(miscnames,"light",5) == 0)
            armour = AC_light;
        else if (strncasecmp(miscnames,"heavy",5) == 0)
            armour = AC_heavy;
        else if (strncasecmp(miscnames,"concrete",8) == 0)
            armour = AC_concrete;

        delete[] miscnames;
    }
    valid = true;
}


//////
const char* UnitType::getRandTalk(TalkbackType type) const
{
    if (talkback != NULL) {
        return talkback->getRandTalk(type);
    }
    return NULL;
}

UnitType::~UnitType()
{
    Uint16 i;
    delete[] name;
    delete[] shpnums;
    delete[] deploytarget;
    for (i=0;i<owners.size();++i)
        delete[] owners[i];
    for (i=0;i<prereqs.size();++i)
        delete[] prereqs[i];
}

/* note to self, pass owner, cellpos, facing and health to this
 (maybe subcellpos)*/
Unit::Unit(UnitType *type, Uint16 cellpos, Uint8 subpos, InfantryGroup *group, 
        Uint8 owner, Uint16 rhealth, Uint8 facing) : UnitOrStructure()
{
    targetCell = cellpos;
    Uint32 i;
    this->type = type;
    imagenumbers = new Uint16[type->getNumLayers()];
    for( i = 0; i < type->getNumLayers(); i++ ) {
        imagenumbers[i] = facing;
        if( owner != 0xff ) {
            if (type->getDeployTarget() != NULL) {
                palettenum = (p::ppool->getStructpalNum(owner)<<11);
            } else {
                palettenum = (p::ppool->getUnitpalNum(owner)<<11);
            }
            imagenumbers[i] |= palettenum;
        }
    }
    this->owner = owner;
    this->cellpos = cellpos;
    this->subpos = subpos;

    harvested.fillnum=0;
    harvested.bailnum=0;
    if (type->canHarvest()) {
      harvested.bails = new Bail[type->getMaxBails()];
      for (Uint8 i=0; i<type->getMaxBails(); i++) {
        for (Uint8 j=0; j<BAIL_SIZE; j++) {
          harvested.bails[i].type[j]=bail_empty;
        }
      }
    } else {
      harvested.bails = NULL;
    }

    l2o = NULL;
    xoffset = 0;
    yoffset = 0;
    ratio = (double)rhealth/256.0f;
    health = (Uint16)(ratio * type->getMaxHealth());
    infgrp = group;

    setNewRefineStr();

    if( infgrp != NULL ) {
        if( infgrp->isClear(subpos) ) { /* else select another subpos */
            infgrp->addInfantry(this, subpos);
        }
    }
    moveanim = NULL;
    attackanim = NULL;
    harvestanim = NULL;
    walkanim = NULL;
    turnanim1 = turnanim2 = NULL;
    unitqueue = NULL;
    container = NULL;
    deployed = false;
    loadstate = PASSENGER_NONE;
    if (type->getMaxPassengers() > 0) {
        //passengers.reserve(type->getMaxPassengers());
    }
    p::ppool->getPlayer(owner)->builtUnit(this);
}

Unit::~Unit()
{
    if (harvested.bails) delete[] harvested.bails;
    delete[] imagenumbers;
    if (attackanim != NULL && target != NULL) {
        target->unrefer();
    }
    if( l2o != NULL ) {
        p::uspool->removeL2overlay(l2entry);
        delete l2o;
    }
    if( type->isInfantry() && infgrp != NULL ) {
        infgrp->removeInfantry(subpos);
        if( infgrp->getNumInfantry() == 0 )
            delete infgrp;
    }
    if (deployed) {
        /** @todo This is a client thing. Will dispatch a "play these sounds"
         * event when the time comes.
         */
        if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("build5.aud");
        else pc::sfxeng->queueSound("constru2.aud");
        if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("hvydoor1.aud");
        else pc::sfxeng->queueSound("placbldg1.aud");
        p::uspool->createStructure(type->getDeployTarget(),calcDeployPos(),owner,(Uint16)(ratio*256.0f),0,true);
    }
    delete unitqueue;
}

void Unit::remove() {
    p::ppool->getPlayer(owner)->lostUnit(this,deployed);
    if (unitqueue != NULL) {
        unitqueue->purge();
    }
    UnitOrStructure::remove();
}

bool Unit::setNewRefineStr() {
    if ((type->getRefineStruct() != NULL) && !p::ppool->getPlayer(owner)->getStructures(p::uspool->getStructureTypeByName(type->getRefineStruct())).empty()) {
        refinestr=p::ppool->getPlayer(owner)->getStructures(p::uspool->getStructureTypeByName(type->getRefineStruct())).front();
        return true;
    } else {
        refinestr=NULL;
        return false;
    }
}

bool Unit::addHarvest(bail_t amount) {
    if (bailsFilled()) {
      return false;
    } else {
      harvested.bails[harvested.bailnum].type[harvested.fillnum]=amount;
      //hack to fill the last bail completely too...
      if ((Uint32)(harvested.bailnum+1)>=type->getMaxBails()
       && (harvested.fillnum+1)>=BAIL_SIZE) return true;

      if ((harvested.fillnum+1)>=BAIL_SIZE) {
        harvested.fillnum=0;
        harvested.bailnum++;
      } else {
        harvested.fillnum++;
      }
      return true;
    }
}

Uint8 Unit::getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets, bool* showpips)
{
    int i;
    Uint32 *shpnums;

    *showpips = (selected && (passengers.size() > 0) &&
                 (owner == p::ppool->getLPlayerNum()));
    shpnums = type->getSHPNums();

    *inums = new Uint32[type->getNumLayers()+(*showpips)];
    *xoffsets = new Sint8[type->getNumLayers()+(*showpips)];
    *yoffsets = new Sint8[type->getNumLayers()+(*showpips)];
    for(i = 0; i < type->getNumLayers(); i++ ) {
        (*inums)[i] = shpnums[i]+imagenumbers[i];
        (*xoffsets)[i] = xoffset-type->getOffset();
        (*yoffsets)[i] = yoffset-type->getOffset();
    }
    if (*showpips) {
        (*inums)[i] = p::ccmap->getPipsNum()+(passengers.size() > 0);
        (*xoffsets)[i] = xoffset-type->getOffset();
        (*yoffsets)[i] = yoffset-type->getOffset()+24;
        return type->getNumLayers()+1;
    } else {
        return type->getNumLayers();
    }
}

void Unit::move(Uint16 dest)
{
    move(dest,true);
}

void Unit::harvest(Uint16 dest, bool stop)
{
    if (moveanim != NULL) {
        moveanim->stop();
    }
    //shouldn't happen...
    if (attackanim != NULL) {
        attackanim->stop();
        if (target != NULL) {
          target->unrefer();
          target = NULL;
        }
    }

    targetCell = dest;
    if (harvestanim == NULL) {
        harvestanim = new HarvestAnimEvent(0, this);
        p::aequeue->scheduleEvent(harvestanim);
    } else {
        harvestanim->update();
    }
}

void Unit::move(Uint16 dest, bool stop)
{
    targetCell = dest;
    if (stop && (attackanim != NULL)) {
        attackanim->stop();
        if (target != NULL) {
            target->unrefer();
            target = NULL;
        }
    }
    if (stop && (harvestanim != NULL)) {
        harvestanim->stop();
    }
    if (moveanim == NULL) {
        moveanim = new MoveAnimEvent(type->getSpeed(), this);
        p::aequeue->scheduleEvent(moveanim);
    } else {
        moveanim->update();
    }
}

bool Unit::targetinRange()
{
    if (!canAttack()) return false;
    Uint32 distance;
    Sint32 xtiles = cellpos % p::ccmap->getWidth() - getTargetCell() % p::ccmap->getWidth();
    Sint32 ytiles = cellpos / p::ccmap->getWidth() - getTargetCell() / p::ccmap->getWidth();
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);
    return (distance <= type->getWeapon()->getRange());
}

bool Unit::targetinRange(Uint16 tarpos)
{
    if (!canAttack()) return false;
    Uint32 distance;
    Sint32 xtiles = cellpos % p::ccmap->getWidth() - tarpos % p::ccmap->getWidth();
    Sint32 ytiles = cellpos / p::ccmap->getWidth() - tarpos / p::ccmap->getWidth();
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles); 
    return (distance <= type->getWeapon()->getRange()); 
}

void Unit::attack(UnitOrStructure* target)
{
    attack(target, true);
}

void Unit::attack(UnitOrStructure* target, bool stop)
{
    if (stop && (moveanim != NULL)) {
        moveanim->stop();
    }
    if (stop && (harvestanim != NULL)) {
        harvestanim->stop();
    }
    if (this->target != NULL) {
        this->target->unrefer();
    }
    this->target = target;
    target->referTo();
    targetCell = target->getBPos(cellpos);
    if (attackanim == NULL) {
        attackanim = new UAttackAnimEvent(0, this);
        p::aequeue->scheduleEvent(attackanim);
    } else {
        attackanim->update();
    }
}

void Unit::turn(Uint8 facing, Uint8 layer)
{
    TurnAnimEvent** t;
    switch (layer) {
    case 0:
        t = &turnanim1;
        break;
    case 1:
        t = &turnanim2;
        break;
    default:
        logger->error("invalid arg of %i to Unit::turn\n",layer);
        return;
        break;
    }
    if (*t == NULL) {
        *t = new TurnAnimEvent(type->getROT()*100/p::ccmap->getMovePercentage(cellpos,this), this, facing, layer);
        p::aequeue->scheduleEvent(*t);
    } else {
        (*t)->changedir(facing);
    }

}

void Unit::stop()
{
    if (moveanim != NULL) {
        moveanim->stop();
    }
    if (attackanim != NULL) {
        attackanim->stop();
    }
    if (harvestanim != NULL) {
        harvestanim->stop();
    }
}

void Unit::applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker)
{
    //fprintf(stderr,"%i * %f = ",amount,weap->getVersus(type->getArmour()));
    amount = (Sint16)((double)amount * weap->getVersus(type->getArmour()));
    //fprintf(stderr,"%i (a == %i)\n",amount,type->getArmour());
    if ((health-amount) <= 0) {
        doRandTalk(TB_die);
        p::ppool->getPlayer(attacker->getOwner())->addUnitKill();
        // todo: add infantry death animation
        p::uspool->removeUnit(this);
        return;
    } else if ((health-amount) > type->getMaxHealth()) {
        health = type->getMaxHealth();
    } else {
        health -= amount;
    }
    ratio = (double)health / (double)type->getMaxHealth();
}

void Unit::doRandTalk(TalkbackType ttype)
{
    const char* sname;
    sname = type->getRandTalk(ttype);
    if (sname != NULL) {
        pc::sfxeng->queueSound(sname);
    }
}

bool Unit::canDeploy()
{
    if (type->canDeploy()) {
        if (type->getDeployTarget() != NULL) {
            if (!deployed)
                return checkDeployTarget(calcDeployPos());
            return false;
        } else {
            return (checkDeployTarget(calcDeployPos())&&(passengers.size() > 0));
        }
    }
    return false;
}

void Unit::deploy()
{
    if (canDeploy()) { // error catching
        if (type->getDeployTarget() != NULL) {
            deployed = true;
            p::uspool->removeUnit(this);
        } else {
            unloadUnits();
        }
    }
}

bool Unit::checkDeployTarget(Uint32 pos)
{
    static Uint32 mapwidth = p::ccmap->getWidth();
    static Uint32 mapheight = p::ccmap->getHeight();
    Uint8 placexpos, placeypos;
    Uint32 curpos;
    Uint8 typewidth, typeheight;
    if (pos == (Uint32)(-1)) {
        return false;
    }
    if (type->getDeployType() == NULL) {
        //return (p::ccmap->getCost(pos,this)<=1);
        return false;
    }
    typewidth = type->getDeployType()->getXsize();
    typeheight = type->getDeployType()->getYsize();
    if ((pos%mapwidth)+typewidth > mapwidth) {
        return false;
    }
    if ((pos/mapwidth)+typeheight > mapheight) {
        return false;
    }
    for( placeypos = 0; placeypos < typeheight; ++placeypos) {
        for( placexpos = 0; placexpos < typewidth; ++placexpos) {
            curpos = pos+placeypos*mapwidth+placexpos;
            if( type->getDeployType()->isBlocked(placeypos*typewidth+placexpos) ) {
                if (!p::ccmap->isBuildableAt(curpos,this)) {
                    return false;
                }
            }
        }
    }
    return true;
}

Uint32 Unit::calcDeployPos() const
{
    Uint32 deploypos;
    Uint32 mapwidth = p::ccmap->getWidth();
    Uint8 w,h;

    if (type->getDeployType() == NULL) {
        if (cellpos%mapwidth == mapwidth) {
            return (Uint32)-1;
        }
        deploypos = cellpos+1;
    } else {
        w = type->getDeployType()->getXsize();
        h = type->getDeployType()->getYsize();

        deploypos = cellpos;
        if ((Uint32)(w >> 1) > deploypos)
            return (Uint32)-1; // large number
        else
            deploypos -= w >> 1;
        if ((mapwidth*(h >> 1)) > deploypos)
            return (Uint32)-1;
        else
            deploypos -= mapwidth*(h >> 1);
    }
    return deploypos;
}

void Unit::setImageNum(Uint32 num, Uint8 layer)
{
    imagenumbers[layer] = num | palettenum;
}

Sint8 Unit::getXoffset() const
{
    if (l2o != NULL) {
        return l2o->xoffsets[0];
    } else {
        return xoffset-type->getOffset();
    }
}

Sint8 Unit::getYoffset() const
{
    if (l2o != NULL) {
        return l2o->yoffsets[0];
    } else {
        return yoffset-type->getOffset();
    }
}

void Unit::setXoffset(Sint8 xo)
{
    if (l2o != NULL) {
        l2o->xoffsets[0] = xo;
    } else {
        xoffset = xo;
    }
}

void Unit::setYoffset(Sint8 yo)
{
    if (l2o != NULL) {
        l2o->yoffsets[0] = yo;
    } else {
        yoffset = yo;
    }
}

bool Unit::pushPassenger(Unit* un)
{
    if (canLoad(un)) {
        passengers.push_back(un);
        un->referTo();
    } else {
        return false;
    }
    return true;
}

Unit* Unit::removePassenger(Unit* un)
{
    Unit* tmp;
    Uint16 i;
    tmp = NULL;
    for (i=0;i<passengers.size();++i) {
        tmp = passengers[i];
        if (tmp == un) {
            un->unrefer();
            passengers[i] = NULL;
            break;
        }
    }
    if (tmp != NULL) {
        for (i=i+1;i<passengers.size();++i) {
            passengers[i-1] = passengers[i];
        }
        passengers[i] = NULL;
        passengers.resize(passengers.size()-1);
    }
    return tmp;
}

bool Unit::canLoad(Unit* un)
{
    Uint32 i;
    if (passengers.size() > type->getMaxPassengers()) {
        return false;
    }
    for (i=0;i<(type->getPassengerAllow()).size();++i) {
        if (un->type->getType() == (type->getPassengerAllow())[i]) {
            return true;
        }
    }
    for (i=0;i<type->getSpecificTypeAllow().size();++i) {
        if (un->type == (type->getSpecificTypeAllow())[i]) {
            return true;
        }
    }
    return false;
}

void Unit::unloadUnits()
{}

void Unit::loadUnits(vector<Unit*> units)
{}

void Unit::cancelLoad(Unit* un)
{
    unitqueue->addCancel(un);
}

void Unit::enterUnitOrStructure(UnitOrStructure* dest)
{}

void Unit::exitUnitOrStructure(UnitOrStructure* source)
{}

void Unit::resetLoadState()
{}

Uint16 Unit::getDist(Uint16 pos)
{
    Uint16 x, y, nx, ny, xdiff, ydiff;
    x = cellpos%p::ccmap->getWidth();
    y = cellpos/p::ccmap->getWidth();
    nx = pos%p::ccmap->getWidth();
    ny = pos/p::ccmap->getWidth();

    xdiff = abs(x-nx);
    ydiff = abs(y-ny);
    return min(xdiff,ydiff)+abs(xdiff-ydiff);
}

Uint16 Unit::getTargetCell()
{
    if (attackanim != NULL && target != NULL) {
        return target->getBPos(cellpos);
    }
    return targetCell;
}


Talkback::Talkback(const char* talkback, INIFile* tbini)
{
    char section[1024];
    char *first;
    vector<char*>* vecptr;
    Uint32 keynum;
    INIKey key;

    strcpy(section, talkback);
    try {
        tbini->readKeyValue(section, 0);
    } catch(int) {
        logger->warning("Could not find talkback \"%s\", reverting to default\n",talkback);
        strcpy(section, "Generic");
    }

    try {
        for (keynum=0;;++keynum) {
            key=tbini->readKeyValue(section, keynum);
            first = stripNumbers(key->first.c_str());
            if (strcasecmp(first,"include") == 0) {
                if (strcasecmp(key->second.c_str(),talkback) != 0) {
                    merge(p::uspool->getTalkback(key->second.c_str()));
                } else {
                    logger->warning("skipping self-referential include in %s\n",talkback);
                }
            } else {
                vecptr = NULL;
                if ( strcasecmp(first,"delete") == 0 ) {
                    vecptr = getTypeVector(getTypeNum(key->second.c_str()),true);
                    (*vecptr).clear();
                    vecptr = NULL;
                } else {
                    vecptr = getTypeVector(getTypeNum(first),true);
                }
                if (vecptr != NULL) {
                    (*vecptr).push_back(cppstrdup(key->second.c_str()));
                }
            }
            delete[] first;
        }
    } catch(int) {}
}

Talkback::~Talkback()
{
    Uint16 i;
    for (i=0;i<tbReport.size();++i)
        delete[] tbReport[i];
    for (i=0;i<tbAck.size();++i)
        delete[] tbAck[i];
    for (i=0;i<tbDie.size();++i)
        delete[] tbDie[i];
    for (i=0;i<tbPostkill.size();++i)
        delete[] tbPostkill[i];
    for (i=0;i<tbAtkSt.size();++i)
        delete[] tbAtkSt[i];
    for (i=0;i<tbAtkUn.size();++i)
        delete[] tbAtkUn[i];
}

const char* Talkback::getRandTalk(TalkbackType type)
{
    Uint8 rnd,sze;
    vector<char*>* vecptr;
    vecptr = getTypeVector(type,false);
    if (vecptr != NULL && vecptr->size() > 0) {
        sze = (*vecptr).size();
        rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
        return (*vecptr)[rnd];
    } else {
        return NULL;
    }
}

vector<char*>* Talkback::getTypeVector(TalkbackType type, bool ignoresize)
{
    vector<char*>* vecptr;
    switch (type) {
    case TB_report:
        vecptr = &tbReport;
        break;
    case TB_ack:
        vecptr = &tbAck;
        break;
    case TB_atkun: /* attack unit */
        vecptr = &tbAck;
        if (ignoresize || (tbAtkUn.size() > 0)) {
            vecptr = &tbAtkUn;
        }
        break;
    case TB_atkst: /* attack structure */
        vecptr = &tbAck;
        if (ignoresize || (tbAtkSt.size() > 0)) {
            vecptr = &tbAtkSt;
        }
        break;
    case TB_die:
        if (ignoresize || (tbDie.size() > 0)) {
            vecptr = &tbDie;
        } else {
            vecptr = NULL;
        }
        break;
    case TB_postkill:
        if (ignoresize || (tbPostkill.size() > 0)) {
            vecptr = &tbPostkill;
        } else {
            vecptr = NULL;
        }
        break;
    default:
        logger->warning("Unknown talkback type: %i\n",type);
        vecptr = NULL;
        break;
    }
    return vecptr;
}

void Talkback::merge(Talkback* mergee)
{
    Uint16 i;
    for (i=0;i<mergee->tbReport.size();++i)
        tbReport.push_back(cppstrdup(mergee->tbReport[i]));
    for (i=0;i<mergee->tbAck.size();++i)
        tbAck.push_back(cppstrdup(mergee->tbAck[i]));
    for (i=0;i<mergee->tbDie.size();++i)
        tbDie.push_back(cppstrdup(mergee->tbDie[i]));
    for (i=0;i<mergee->tbPostkill.size();++i)
        tbPostkill.push_back(cppstrdup(mergee->tbPostkill[i]));
    for (i=0;i<mergee->tbAtkSt.size();++i)
        tbAtkSt.push_back(cppstrdup(mergee->tbAtkSt[i]));
    for (i=0;i<mergee->tbAtkUn.size();++i)
        tbAtkUn.push_back(cppstrdup(mergee->tbAtkUn[i]));
}

TalkbackType Talkback::getTypeNum(const char* name)
{
    if (strcasecmp("report",name) == 0)
        return TB_report;
    if (strcasecmp("ack",name) == 0)
        return TB_ack;
    if (strcasecmp("die",name) == 0)
        return TB_die;
    if (strcasecmp("postkill",name) == 0)
        return TB_postkill;
    if (strcasecmp("attackunit",name) == 0)
        return TB_atkun;
    if (strcasecmp("attackstruct",name) == 0)
        return TB_atkst;
    logger->error("Unknown type: %s\n",name);
    return TB_invalid;
}
