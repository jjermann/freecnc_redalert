#include <cstdlib>
#include <cstring>
#include "config.h"
#include "ccmap.h"
#include "common.h"
#include "inifile.h"
#include "logger.h"
#include "playerpool.h"
#include "shpimage.h"
#include "soundengine.h"
#include "structure.h"
#include "unitqueue.h"
#include "unitandstructurepool.h"
#include "weaponspool.h"

StructureType::StructureType(const char* typeName, INIFile *structini, 
        INIFile* artini, const char* thext) : UnitOrStructureType()
{
    SHPImage *shpimage, *makeimage;
    Uint32 i;
    char shpname[13], imagename[8];
    char* tmp;
    char blocktest[128];
    Uint32 size;
    char* miscnames;
    // for reading in what units can be used as passengers
    std::vector<char*> tmpallow;
    UnitType* tmpspecallow;

    // Ensure that there is a section in the ini file
    try {
        structini->readKeyValue(typeName,0);
    } catch (int) {
        shpnums = NULL;
        blocked = NULL;
        shptnum = NULL;
        name    = NULL;
        return;
    }

    is_wall = false;

    memset(this->tname,0x0,8);
    strncpy(this->tname,typeName,8);
    name = structini->readString(tname,"name");
    //prereqs = structini->splitList(tname,"prerequisites",',');
    tmp = structini->readString(tname, "prerequisites");
    if( tmp != NULL ) {
        prereqs = splitList(tmp, ',');
        delete[] tmp;
    }
    //owners = structini->splitList(tname,"owners",',');
    tmp = structini->readString(tname, "owners");
    if(  tmp != NULL ) {
        owners = splitList(tmp, ',');
        delete[] tmp;
    }
    if (owners.empty()) {
        logger->warning("%s has no owners\n",tname);
    }

    if( !strcasecmp(tname, "BRIK") || !strcasecmp(tname, "SBAG") ||
            !strcasecmp(tname, "BARB") || !strcasecmp(tname, "WOOD") ||
            !strcasecmp(tname, "CYCL") || !strcasecmp(tname, "FENC") )
        is_wall = true;

    /* the size of the structure in tiles */
    xsize = artini->readInt(tname, "xsize",1);
    ysize = artini->readInt(tname, "ysize",1);

    blckoff = 0;
    blocked = new Uint8[xsize*ysize];
    for( i = 0; i < (Uint32)xsize*ysize; i++ ) {
        sprintf(blocktest, "notblocked%d", i);
        size = artini->readInt(tname, blocktest);
        if(size == INIERROR) {
            blocked[i] = 1;
            xoffset = -(i%xsize)*24;
            yoffset = -(i/xsize)*24;
            if (blocked[blckoff] == 0) {
                blckoff = i;
            }
        } else {
            blocked[i] = 0;
        }
    }

    memset(shpname,0,13);
    numshps = structini->readInt(tname, "layers",1);

    shpnums = new Uint16[(is_wall?numshps:numshps+1)];
    shptnum = new Uint16[numshps];

    buildlevel = structini->readInt(tname,"buildlevel",100);
    techlevel = structini->readInt(tname,"techlevel",99);
    if (buildlevel == 100) {
        logger->warning("%s does not have a buildlevel\n",tname);
    }

    powerinfo.power = structini->readInt(tname,"power",0);
    powerinfo.drain = structini->readInt(tname,"drain",0);
    powerinfo.powered = structini->readInt(tname,"powered",0) != 0;
    maxhealth = structini->readInt(tname,"health",100);
    maxstorage = structini->readInt(tname,"storage",0);

    for( i = 0; i < numshps; i++ ) {
        sprintf(imagename, "image%d", i+1);
        tmp = structini->readString(tname, imagename);
        if( tmp == NULL ) {
            strncpy(shpname, tname, 13);
            strncat(shpname, ".SHP", 13);
        } else {
            strncpy(shpname, tmp, 13);
            delete[] tmp;
        }
        try {
            shpimage = new SHPImage(shpname, mapscaleq);
        } catch (ImageNotFound&) {
            strncpy(shpname, tname, 13);
            strncat(shpname, thext, 13);
            try {
                shpimage = new SHPImage(shpname, mapscaleq);
            } catch (ImageNotFound&) {
                logger->warning("Image not found: \"%s\"\n", shpname);
                numshps = 0;
                return;
            }
        }
        shpnums[i] = pc::imagepool->size();
        shptnum[i] = shpimage->getNumImg();
        pc::imagepool->push_back(shpimage);
    }
    if (!is_wall) {
        numwalllevels = 0;
        animinfo.loopend = structini->readInt(tname,"loopend",0);
        animinfo.loopend2 = structini->readInt(tname,"loopend2",0);

        animinfo.animspeed = structini->readInt(tname,"animspeed", 3);
        animinfo.animspeed = abs(animinfo.animspeed);
        animinfo.animspeed = (animinfo.animspeed>1?animinfo.animspeed:2);
        animinfo.animdelay = structini->readInt(tname,"delay",0);

        animinfo.animtype = structini->readInt(tname, "animtype", 0);
        animinfo.sectype  = structini->readInt(tname, "sectype", 0);

        animinfo.dmgoff   = structini->readInt(tname, "dmgoff", ((shptnum[0]-1)>>1));
        if (numshps == 2)
            animinfo.dmgoff2 = structini->readInt(tname, "dmgoff2", (shptnum[1]>>1));
        else
            animinfo.dmgoff2 = 0;

        defaultface       = structini->readInt(tname, "defaultface", 0);

        if (strlen(tname) <= 4) {
            strncpy(shpname, tname, 13);
            strncat(shpname, "make.shp", 13);
        } else
            logger->warning("%s is nonstandard!\n",tname);
        try {
            makeimage = new SHPImage(shpname, mapscaleq);
            makeimg = pc::imagepool->size();
            animinfo.makenum = makeimage->getNumImg();
            pc::imagepool->push_back(makeimage);
        } catch (ImageNotFound&) {
            makeimg = 0;
            animinfo.makenum = 0;
        }

        miscnames = structini->readString(tname, "primary_weapon");
        if( miscnames == NULL ) {
            primary_weapon = NULL;
        } else {
            primary_weapon = p::weappool->getWeapon(miscnames);
            delete[] miscnames;
        }
        miscnames = structini->readString(tname, "secondary_weapon");
        if( miscnames == NULL ) {
            secondary_weapon = NULL;
        } else {
            secondary_weapon = p::weappool->getWeapon(miscnames);
            delete[] miscnames;
        }
        turret = (structini->readInt(tname,"turret",0) != 0);
        if (turret) {
            turnspeed = structini->readInt(tname,"rot",3);
        }
    } else {
        numwalllevels = structini->readInt(tname,"levels",1);
        turret = 0;
        primary_weapon = NULL;
        secondary_weapon = NULL;
    }
    cost = structini->readInt(tname, "cost", 0);

    miscnames = structini->readString(tname,"movetype");
    movetype=MT_build;
    if (miscnames == NULL) {
      // do nothing
    } else {
      if (strcasecmp(miscnames,"build") == 0)
        movetype=MT_build;
      else if (strcasecmp(miscnames,"build_water") == 0)
        movetype=MT_build_water;
    }

    miscnames = structini->readString(tname,"armour","none");
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

    primarysettable = (structini->readInt(tname,"primary",0) != 0);
    // we use a fixed number of passengers for structures
    maxpassengers = 5;
    tmp = structini->readString(tname, "passengersallow");
    if( tmp != NULL ) {
        tmpallow = splitList(tmp, ',');
        delete[] tmp;
    }
    //tmpallow = structini->splitList(tname,"passengersallow",',');
    for (i=0;i<tmpallow.size();++i) {
        if (tmpallow[i][0] < '4') {
            passengerAllow.push_back(atoi(tmpallow[i]));
        } else {
            tmpspecallow = p::uspool->getUnitTypeByName(tmpallow[i]);
            if (tmpspecallow != NULL) {
                specificTypeAllow.push_back(tmpspecallow);
            } else {
                logger->warning("Invalid type name: %s (in %s)\n",tmpallow[i],tname);
            }
        }
        delete[] tmpallow[i];
    }
    valid = true;
}


StructureType::~StructureType()
{
    Uint16 i;
    for (i=0;i<owners.size();++i)
        delete[] owners[i];
    for (i=0;i<prereqs.size();++i)
        delete[] prereqs[i];
    delete[] shpnums;
    delete[] blocked;
    delete[] shptnum;
    delete[] name;
}

Structure::Structure(StructureType *type, Uint16 cellpos, Uint8 owner,
        Uint16 rhealth, Uint8 facing) : UnitOrStructure()
{
    Uint32 i;
    targetCell = cellpos;
    this->type = type;
    imagenumbers = new Uint16[type->getNumLayers()];
    if (!type->hasTurret()) {
        facing = 0;
    }
    for(i=0;i<type->getNumLayers();i++) {
        imagenumbers[i] = facing;
        if( owner != 0xff && !type->isWall() ) {
            imagenumbers[i] |= (p::ppool->getStructpalNum(owner)<<11);
        }
    }
    exploding = false;
    this->owner = owner;
    this->cellpos = cellpos;
    bcellpos = cellpos+(type->getBlckOff() % type->getXsize()) +
               ((type->getBlckOff()/type->getXsize())*p::ccmap->getWidth());
    animating = false;
    usemakeimgs = false;
    primary = false;
    buildAnim = NULL;
    repairAnim = NULL;
    attackAnim = NULL;
    unitqueue = NULL;
    loadstate = PASSENGER_NONE;
    //passengers.reserve(type->getMaxPassengers());
    health = (Uint16)((double)rhealth/256.0f * (double)type->getMaxHealth());
    damaged = checkdamage();
    if( !type->isWall() ) {
        p::ppool->getPlayer(owner)->builtStruct(this);
    }
}

Structure::~Structure()
{
    delete[] imagenumbers;
}

Uint16 Structure::getBPos(Uint16 pos) const
{
    Uint16 x,y,dx,dy,t,retpos,bpos,sc;
    Sint16 dw;
    Uint32 mwid = p::ccmap->getWidth();
    x = cellpos%mwid;
    dx = 0;
    if ((pos%mwid) > x) {
        dx = min((unsigned)(type->getXsize()-1),(unsigned)((pos%mwid)-x)-1);
    }
    y = cellpos/mwid;
    dy = 0;
    if ((pos/mwid) > y) {
        dy = min((unsigned)(type->getYsize()-1),(unsigned)((pos/mwid)-y)-1);
    }
    retpos = (x+dx)+(y+dy)*mwid;
    // just makes the bpos calculation cleaner
    sc = x+y*mwid;
    dw = type->getXsize() - mwid;
    bpos   = retpos - sc + dy*dw;
    while (!type->isBlocked(dx+dy*type->getXsize())) {
        /* This happens in this situation (P is position of attacker,
         * X is a blocked cell and _ is an unblocked cell)
         * P   P
         *  _X_
         *  XXX
         *  _X_
         * P   P
         */
        if (dx == type->getXsize()-1) {
            for (t=dx;t>0;--t) {
                retpos = (x+t)+(y+dy)*mwid;
                bpos   = retpos - sc + dy*dw;
                if (type->isBlocked(bpos)) {
                    return retpos;
                }
            }
        } else {
            for (t=dx;t<type->getXsize();++t) {
                retpos = (x+t)+(y+dy)*mwid;
                bpos   = retpos - sc + dy*dw;
                if (type->isBlocked(bpos)) {
                    return retpos;
                }
            }
        }
        ++dy;
        if (dy >= type->getYsize()) {
            logger->error("ERROR: could not find anywhere to shoot at %s!\n",type->getTName());
        }
        retpos = (x+dx)+(y+dy)*mwid;
    }
    return retpos;
}


Uint16 Structure::getFreePos(UnitType* un)
{
    Uint8 i,xsize,ysize;
    Uint16 x,y,curpos;

    xsize=((StructureType*)this->getType())->getXsize();
    ysize=((StructureType*)this->getType())->getYsize();

    curpos=cellpos;
    p::ccmap->translateFromPos(curpos, &x, &y);
    y+=ysize; //bottom left of building
    curpos=p::ccmap->translateToPos(x,y);

    if (curpos!=POS_INVALID && !p::ccmap->isBuildableAt(curpos,un))
        curpos=POS_INVALID;

    for (i=0;(i<xsize && curpos==POS_INVALID);i++) {
        ++x;
        curpos=p::ccmap->translateToPos(x,y);
        if (!p::ccmap->isBuildableAt(curpos,un))
            curpos=POS_INVALID;
    }
    //ugly: I assume that the first blocks are noblocked
    for (i=0;(i<ysize && curpos==POS_INVALID);i++) {
        --y;
        curpos=p::ccmap->translateToPos(x,y);
        if (!p::ccmap->isBuildableAt(curpos,un))
            curpos=POS_INVALID;
    }
    for (i=0;(i<(xsize+1) && curpos==POS_INVALID);i++) {
        --x;
        curpos=p::ccmap->translateToPos(x,y);
        if (!p::ccmap->isBuildableAt(curpos,un))
            curpos=POS_INVALID;
    }
    for (i=0;(i<ysize && curpos==POS_INVALID);i++) {
        ++y;
        curpos=p::ccmap->translateToPos(x,y);
        if (!p::ccmap->isBuildableAt(curpos,un))
            curpos=POS_INVALID;
    }
    return curpos;
}


void Structure::remove() {
    if (!type->isWall()) {
        p::ppool->getPlayer(owner)->lostStruct(this);
    }
    UnitOrStructure::remove();
}

/** Method to get a list of imagenumbers which the renderer will draw. */
Uint8 Structure::getImageNums(Uint32 **inums, Sint8 **xoffsets, Sint8 **yoffsets, bool* showpips) {
    Uint16 *shps;
    int i;

    *showpips = (primary && selected &&
                 (owner == p::ppool->getLPlayerNum()));
    shps = type->getSHPNums();

    if (usemakeimgs && (!type->isWall()) && (type->getMakeImg() != 0)) {
        *inums = new Uint32[1];
        *xoffsets = new Sint8[1];
        *yoffsets = new Sint8[1];
        (*inums)[0] = (type->getMakeImg()<<16)|imagenumbers[0];
        (*xoffsets)[0] = type->getXoffset();
        (*yoffsets)[0] = type->getYoffset();
        return 1;
    } else {
        *inums = new Uint32[type->getNumLayers()+(*showpips)];
        *xoffsets = new Sint8[type->getNumLayers()+(*showpips)];
        *yoffsets = new Sint8[type->getNumLayers()+(*showpips)];
        for(i = 0; i < type->getNumLayers(); i++ ) {
            (*inums)[i] = (shps[i]<<16)|imagenumbers[i];
            (*xoffsets)[i] = type->getXoffset();
            (*yoffsets)[i] = type->getYoffset();
        }
        if (*showpips) {
            (*inums)[i] = p::ccmap->getPipsNum()+2;
            (*xoffsets)[i] = type->getXoffset();
            (*yoffsets)[i] = type->getYoffset() + (type->getYsize())*24;
            return type->getNumLayers()+1;
        } else {
            return type->getNumLayers();
        }
    }
}

void Structure::runAnim(Uint32 mode, bool back, Uint16 options)
{
    Uint32 speed;
    if (!animating) {
        animating = true;
        if (mode == 0) { // run build anim at const speed
            usemakeimgs = true;
            buildAnim = new BuildAnimEvent(3,this,back);
        } else {
            speed = type->getAnimInfo().animspeed;
            switch (mode&0xf) {
            case 1:
                if (type->getAnimInfo().animtype == 4) {
                    buildAnim = new ProcAnimEvent(speed,this);
                } else {
                    buildAnim = new LoopAnimEvent(speed,this);
                }
                break;
            case 2:
                buildAnim = new BTurnAnimEvent(speed,this,(mode>>4));
                break;
            case 5:
                buildAnim = new DoorAnimEvent(2,this,back);
                break;
            case 7:
                buildAnim = new RefineAnimEvent(speed,this,options);
                break;
            case 8:
                buildAnim = new ChargeAnimEvent(speed,this);
                break;
            default:
                buildAnim = NULL;
                animating = false;
                break;
            }
        }
        if (buildAnim != NULL) {
            p::aequeue->scheduleEvent(buildAnim);
        }
//TODO: do this properly
    } else if (back && mode==0) {
        if (buildAnim) {
            buildAnim->stop();
        }
        stopAnim();
        usemakeimgs = true;
        buildAnim=new BuildAnimEvent(3,this,back);
        p::aequeue->scheduleEvent(buildAnim);
    }
}

void Structure::runSecAnim(Uint32 param)
{
    BuildingAnimEvent* sec_anim = NULL;
    Uint8 secmode = type->getAnimInfo().sectype;
    if (secmode != 0) {
        switch (secmode) {
        case 7:
            sec_anim = new RefineAnimEvent(2,this,param);
            break;
        case 5:
            sec_anim = new DoorAnimEvent(2,this,true);
            break;
        case 8:
            //sec_anim = new RepairAnimEvent(3,this);
            break;
        }
        if (animating) {
            buildAnim->setSchedule(sec_anim);
            stopAnim();
        } else {
            buildAnim = sec_anim;
            p::aequeue->scheduleEvent(buildAnim);
            animating = true;
        }
    }
}

void Structure::stopAnim()
{
    buildAnim->stop();
}

void Structure::stop()
{
    if (attackAnim != NULL) {
        attackAnim->stop();
    }
}

void Structure::applyDamage(Sint16 amount, Weapon* weap, UnitOrStructure* attacker)
{
    if (exploding)
        return;
    Uint8 odam = damaged;
    amount = (Sint16)((double)amount * weap->getVersus(type->getArmour()));
    if ((health-amount) <= 0) {
        exploding = true;
        if (type->isWall()) {
            p::uspool->removeStructure(this);
        } else {
            p::ppool->getPlayer(attacker->getOwner())->addStructureKill();
            BExplodeAnimEvent* boom = new BExplodeAnimEvent(1,this);
            if (animating) {
                buildAnim->setSchedule(boom);
                buildAnim->stop();
            } else {
                buildAnim = boom;
                p::aequeue->scheduleEvent(boom);
            }
        }
        return;
    } else if ((health-amount)>type->getMaxHealth()) {
        health = type->getMaxHealth();
    } else {
        health -= amount;
    }
    if (animating) {
        buildAnim->updateDamaged();
    } else {
        if (type->isWall()) {
            if ((damaged = checkdamage())) { // This is correct
                if (odam != damaged) {
                    changeImage(0,16);
                }
            }
        } else {
            if ((damaged = checkdamage())) { // This is correct
                if (odam != damaged) { // only play critical damage sound once
                    if (pc::sfxeng != NULL) {
                        if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("kaboom1.aud");
                        else pc::sfxeng->queueSound("xplobig4.aud");
                    }
                        setImageNum((imagenumbers[0]&~0x800)+type->getAnimInfo().dmgoff+1,0);
                    if (type->getNumLayers() == 2)
                        setImageNum((imagenumbers[1]&~0x800)+type->getAnimInfo().dmgoff2+1,1);
                }
            } else {
                if (odam) {
                        setImageNum((imagenumbers[0]&~0x800)-type->getAnimInfo().dmgoff-1,0);
                    if (type->getNumLayers() == 2)
                        setImageNum((imagenumbers[1]&~0x800)-type->getAnimInfo().dmgoff2-1,1);
                    return;
                }
            }
        }
    }
}

void Structure::applyDamage(Sint16 amount)
{
    if (exploding)
        return;
    Uint8 odam = damaged;
    amount = amount;
    if ((health-amount) <= 0) {
        exploding = true;
        if (type->isWall()) {
            p::uspool->removeStructure(this);
        } else {
            BExplodeAnimEvent* boom = new BExplodeAnimEvent(1,this);
            if (animating) {
                buildAnim->setSchedule(boom);
                buildAnim->stop();
            } else {
                buildAnim = boom;
                p::aequeue->scheduleEvent(boom);
            }
        }
        return;
    } else if ((health-amount)>type->getMaxHealth()) {
        health = type->getMaxHealth();
    } else {
        health -= amount;
    }
    if (animating) {
        buildAnim->updateDamaged();
    } else {
        if (type->isWall()) {
            if ((damaged = checkdamage())) { // This is correct
                if (odam != damaged) {
                    changeImage(0,16);
                }
            }
        } else {
            if ((damaged = checkdamage())) { // This is correct
                if (odam != damaged) { // only play critical damage sound once
                    if (pc::sfxeng != NULL) {
                        if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("kaboom1.aud");
                        else pc::sfxeng->queueSound("xplobig4.aud");
                    }
                        setImageNum((imagenumbers[0]&~0x800)+type->getAnimInfo().dmgoff+1,0);
                    if (type->getNumLayers() == 2)
                        setImageNum((imagenumbers[1]&~0x800)+type->getAnimInfo().dmgoff2+1,1);
                }
            } else {
                if (odam) {
                        setImageNum((imagenumbers[0]&~0x800)-type->getAnimInfo().dmgoff-1,0);
                    if (type->getNumLayers() == 2)
                        setImageNum((imagenumbers[1]&~0x800)-type->getAnimInfo().dmgoff2-1,1);
                    return;
                }
            }
        }
    }
}

bool Structure::targetinRange()
{
    if (!canAttack()) return false;
    Uint32 distance;   
    Sint32 xtiles = cellpos % p::ccmap->getWidth() - getTargetCell() % p::ccmap->getWidth();
    Sint32 ytiles = cellpos / p::ccmap->getWidth() - getTargetCell() / p::ccmap->getWidth();
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);
    return (distance <= type->getWeapon()->getRange());
}

bool Structure::targetinRange(Uint16 tarpos)
{
    if (!canAttack()) return false;
    Uint32 distance;
    Sint32 xtiles = cellpos % p::ccmap->getWidth() - getBPos(tarpos) % p::ccmap->getWidth();
    Sint32 ytiles = cellpos / p::ccmap->getWidth() - getBPos(tarpos) / p::ccmap->getWidth();
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);
    return (distance <= type->getWeapon()->getRange());
}

void Structure::attack(UnitOrStructure* target)
{
    this->target = target;
    targetCell = target->getBPos(cellpos);
    if( attackAnim == NULL ) {
        attackAnim = new BAttackAnimEvent(0, this);
        p::aequeue->scheduleEvent(attackAnim);
    } else {
        attackAnim->update();
    }
}

Uint8 Structure::checkdamage()
{
    ratio = ((double)health)/((double)type->getMaxHealth());
    if (type->isWall()) {
        if ((ratio <= 0.33)&&(type->getNumWallLevels() == 3))
            return 2;
        else
            return ((ratio <= 0.66)&&(type->getNumWallLevels() > 1));
    } else {
        return (ratio <= 0.5);
    }
}

void Structure::setImageNum(Uint32 num, Uint8 layer)
{
    imagenumbers[layer]=(num)|(p::ppool->getStructpalNum(owner)<<11);
}

bool Structure::pushPassenger(Unit* un)
{
    if (canLoad(un)) {
        passengers.push_back(un);
        un->referTo();
    } else {
        return false;
    }
    return true;
}

Unit* Structure::removePassenger(Unit* un)
{
    Unit* tmp;
    Uint16 i;
    tmp = NULL;
    for (i=0;i<passengers.size();++i) {
        tmp = passengers[i];
        if (tmp == un) {
            passengers[i] = NULL;
            un->unrefer();
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

bool Structure::canLoad(Unit* un)
{
    Uint32 i;
    if (passengers.size() >= type->getMaxPassengers()) {
        return false;
    }
    for (i=0;i<(type->getPassengerAllow()).size();++i) {
        if (((UnitType*)un->getType())->getType() == (type->getPassengerAllow())[i]) {
            return true;
        }
    }
    for (i=0;i<type->getSpecificTypeAllow().size();++i) {
        if (un->getType() == (type->getSpecificTypeAllow())[i]) {
            return true;
        }
    }
    return false;
}

void Structure::unloadUnits()
{}

void Structure::loadUnits(std::vector<Unit*> units) {}

void Structure::cancelLoad(Unit* un)
{
    unitqueue->addCancel(un);
}

void Structure::resetLoadState(bool runsec, Uint32 param)
{}

// This should be customisable somehow
Uint32 Structure::getExitCell() const
{
    return cellpos+(type->getYsize()*p::ccmap->getWidth());
}

Uint16 Structure::getTargetCell() const
{
    if (attackAnim != NULL && target != NULL) {
        return target->getBPos(cellpos);
    }
    return targetCell;
}

bool Structure::repair(bool change)
{
    if (repairAnim != NULL && change) {
        repairAnim=NULL;
        delete repairAnim;
        return false;
    } else if (repairAnim==NULL) {
        repairAnim = new RepairAnimEvent(this);
        return true;
    }
    return false;
}

