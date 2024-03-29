#include <algorithm>
#include <cctype>
#include "common.h"
#include "inifile.h"
#include "logger.h"
#include "projectileanim.h"
#include "shpimage.h"
#include "soundengine.h"
#include "structure.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "unitorstructure.h"
#include "weaponspool.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::map;
using std::string;
#endif

Warhead::Warhead(const char *whname, INIFile *weapini)
{
    char *tmpname;
    SHPImage* temp;

    spread = weapini->readInt(whname, "spread", 0);
    tmpname = weapini->readString(whname, "explosionimage");
    explosionimage = 0;
    if( tmpname != NULL ) {
        explosionimage = pc::imagepool->size()<<16;
        try {
            temp = new SHPImage(tmpname, mapscaleq);
        } catch (ImageNotFound&) {
            throw 0;
        }
        pc::imagepool->push_back(temp);
        delete[] tmpname;
    }
    explosionanimsteps = temp->getNumImg();
    explosionsound = weapini->readString(whname, "explosionsound");
    infantrydeath = 0;
    walls = (weapini->readInt(whname, "walls",0) != 0);
    trees = false;
    //blastradius = 0;
    tmpname = weapini->readString(whname, "versus");
    versus[0] = 100;
    versus[1] = 100;
    versus[2] = 100;
    versus[3] = 100;
    versus[4] = 100;
    if (tmpname != NULL) {
        sscanf(tmpname,"%u,%u,%u,%u,%u",&versus[0],&versus[1],
               &versus[2],&versus[3],&versus[4]);
    }
    delete[] tmpname;
    //for (int i=0;i<5;++i)
    //    fprintf(stderr,"%s\t%i\t%i\n",whname,i,versus[i]);
}

Warhead::~Warhead()
{
    if( explosionsound != NULL ) {
        delete[] explosionsound;
    }
}

Projectile::Projectile(const char *pname, INIFile *weapini)
{
    char *iname = weapini->readString(pname, "image");
    SHPImage* temp;
    imagenum = 0;
    rotates = false;
    rotationimgs = 0;
    if( iname != NULL ) {
        imagenum = pc::imagepool->size()<<16;
        try {
            temp = new SHPImage(iname, mapscaleq);
        } catch (ImageNotFound&) {
            throw 0;
        }
        //  printf("Projectile %s has %s which has %i\n",pname,iname,temp->getNumImg());
        pc::imagepool->push_back(temp);
        delete[] iname;

        if (weapini->readInt(pname,"rotates",0) != 0) {
            rotationimgs = temp->getNumImg();
            rotates = true;
        }
    }
    AA = false;
    AG = true;
    high = false;
    inacurate = false;
}

Projectile::~Projectile()
{}

Weapon::Weapon(const char* wname) : name(wname)
{
    char *pname, *whname, *faname, *faimage;
    map<string, Projectile*>::iterator projentry;
    map<string, Warhead*>::iterator wheadentry;
    INIFile *weapini = p::weappool->getWeaponsINI();
    SHPImage* fireanimtemp;
    Uint8 additional, i;
    string projname, warheadname;
    string weapname = (string)wname;
    string::iterator p = weapname.begin();
    while (p!=weapname.end())
        *p++ = toupper(*p);

    pname = weapini->readString(wname, "projectile");
    if( pname == NULL ) {
        logger->warning("Unable to find projectile for weapon \"%s\" in inifile..\n", wname);
        throw 0;
    }
    projname = (string)pname;
    p = projname.begin();
    while (p!=projname.end())
        *p++ = toupper(*p);

    projentry = p::weappool->projectilepool.find(projname);
    if( projentry == p::weappool->projectilepool.end() ) {
        try {
            projectile = new Projectile(pname, weapini);
        } catch(int) {
            logger->warning("Unable to find projectile \"%s\" used for weapon \"%s\".\nUnit using this weapon will be unarmed\n", pname, wname);
            delete[] pname;
            throw 0;
        }
        p::weappool->projectilepool[projname] = projectile;
    } else {
        projectile = projentry->second;
    }
    delete[] pname;

    whname = weapini->readString(wname, "warhead");
    if( whname == NULL ) {
        logger->warning("Unable to find warhead for weapon \"%s\" in inifile..\n", wname);
        throw 0;
    }
    warheadname = (string)whname;
    transform(warheadname.begin(),warheadname.end(), warheadname.begin(), toupper);
    wheadentry = p::weappool->warheadpool.find(warheadname);
    if( wheadentry == p::weappool->warheadpool.end() ) {
        try {
            whead = new Warhead(whname, weapini);
        } catch (int) {
            logger->warning("Unable to find Warhead \"%s\" used for weapon \"%s\".\nUnit using this weapon will be unarmed\n", whname, wname);
            delete[] whname;
            throw 0;
        }
        p::weappool->warheadpool[warheadname] = whead;
    } else {
        whead = wheadentry->second;
    }
    delete[] whname;

    speed      = weapini->readInt(wname, "speed", 100);
    range      = weapini->readInt(wname, "range", 1);
    reloadtime = weapini->readInt(wname, "reloadtime", 5);
    damage     = weapini->readInt(wname, "damage", 10);
    burst      = weapini->readInt(wname, "burst", 1);
    heatseek   = (weapini->readInt(wname, "heatseek", 0) != 0);
    fireimage  = pc::imagepool->size()<<16;
    // pc::imagepool->push_back(new SHPImage("minigun.shp", mapscaleq));
    firesound  = weapini->readString(wname, "firesound");
    chargesound = weapini->readString(wname, "chargesound");
    fuel       = weapini->readInt(wname, "fuel", 0);
    seekfuel   = weapini->readInt(wname, "seekfuel", 0);

    faname = weapini->readString(wname, "fireimage", "none");
    if (strcasecmp(faname,"none") == 0) {
        delete[] faname;
        numfireimages = 0;
        numfiredirections = 1;
        fireimage = 0;
    } else {
        additional = (Uint8)weapini->readInt(faname,"additional",0);
        faimage = weapini->readString(faname, "image", "minigun.shp");
        try {
            fireanimtemp = new SHPImage(faimage, mapscaleq);
        } catch (ImageNotFound&) {
            throw 0;
        }
        delete[] faimage;
        faimage = NULL;
        numfireimages = fireanimtemp->getNumImg();
        numfiredirections = weapini->readInt(faname, "directions", 1);
        if (numfiredirections == 0) {
            numfiredirections = 1;
        }
        fireimages = new Uint32[numfiredirections];
        fireimages[0] = fireimage;
        pc::imagepool->push_back(fireanimtemp);
        if (additional != 0) {
            char* tmpname = new char[12];
            for (i=2;i<=additional;++i) {
                sprintf(tmpname,"image%i",i);
                faimage = weapini->readString(faname, tmpname, "");
                if (strcasecmp(faimage,"") != 0) {
                    try {
                        fireanimtemp = new SHPImage(faimage, mapscaleq);
                    } catch (ImageNotFound&) {
                        throw 0;
                    }
                    fireimages[i-1]=(pc::imagepool->size()<<16);
                    numfireimages += fireanimtemp->getNumImg();
                    pc::imagepool->push_back(fireanimtemp);
                } else {
                    fireimages[i] = 0;
                    logger->warning("%s was empty in [%s]\n",tmpname,faname);
                }
                delete[] faimage;
                faimage = NULL;
            }
            delete[] tmpname;
        } else if (numfiredirections != 1) {
            for (i=1;i<numfiredirections;++i) {
                fireimages[i] = fireimage+i*(numfireimages/numfiredirections);
            }
        }
        delete[] faname;
    }
}

Weapon::~Weapon()
{
    if( firesound != NULL ) {
        delete[] firesound;
    }
    if( chargesound != NULL ) {
        delete[] chargesound;
    }
    if (fireimage != 0) {
        delete[] fireimages;
    }
}

void Weapon::fire(UnitOrStructure *owner, Uint16 target, Uint8 subtarget) {
    Uint8 loopend=((UnitOrStructureType*)owner->getType())->getAnimInfo().loopend;
    Uint8 loopend2=((UnitOrStructureType*)owner->getType())->getAnimInfo().loopend2;

    if( firesound != NULL ) {
        pc::sfxeng->queueSound(firesound);
    }
    if( fireimage != 0 ) {
        Uint32 length = numfireimages;
        Uint8 facing;
        if (owner->getType()->getNumLayers() == 1) {
            facing = (owner->getImageNum(0))&loopend;
        } else {
            facing = (owner->getImageNum(1))&loopend2;
        }
        if (!owner->getType()->isInfantry()) {
            facing >>= 2;
        }
        length /= numfiredirections;
        if (numfiredirections == 1) {
            facing = 0;
        }
        new ExplosionAnim(1, owner->getPos(),fireimages[facing],
                          (Uint8)length,/*owner->getXoffset()+*/InfantryGroup::getunitoffsets()[owner->getSubpos()],
                          /*owner->getYoffset()+*/InfantryGroup::getunitoffsets()[owner->getSubpos()]);
    }
    p::aequeue->scheduleEvent(new ProjectileAnim(0, this, owner, target, subtarget));
}

WeaponsPool::WeaponsPool()
{
    weapini = new INIFile("weapons.ini");
}

WeaponsPool::~WeaponsPool()
{
    delete weapini;
    map<string, Weapon*>::const_iterator wpclear;
    map<string, Warhead*>::const_iterator whpclear;
    map<string, Projectile*>::const_iterator ppclear;
    wpclear  = weaponspool.begin();
    whpclear = warheadpool.begin();
    ppclear  = projectilepool.begin();

    while (wpclear != weaponspool.end()) {
        delete wpclear->second;
        ++wpclear;
    }
    while (whpclear != warheadpool.end()) {
        delete whpclear->second;
        ++whpclear;
    }
    while (ppclear != projectilepool.end()) {
        delete ppclear->second;
        ++ppclear;
    }
}

Weapon *WeaponsPool::getWeapon(const char *wname)
{
    map<string, Weapon*>::iterator weapentry;
    Weapon *weap;
    string weapname = (string)wname;
    transform(weapname.begin(),weapname.end(), weapname.begin(), toupper);
    weapentry = weaponspool.find(weapname);

    if( weapentry == weaponspool.end() ) {
        try {
            weap = new Weapon(wname);
        } catch( int ) {
            /* if weapon is NULL unit doesn't have a weapon */
            return NULL;
        }
        weaponspool[weapname] = weap;
        return weap;
    } else {
        return weapentry->second;
    }
}
