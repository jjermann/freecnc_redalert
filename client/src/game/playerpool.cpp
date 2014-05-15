#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif
#include <cstring>
#include "aiplugman.h"
#include "ccmap.h"
#include "common.h"
#include "config.h"
#include "inifile.h"
#include "logger.h"
#include "netconnection.h"
#include "playerpool.h"
#include "structure.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "soundengine.h"
#include "dispatcher.h"

/**
 * @TODO Make hardcoded side names customisable (Needed to make RA support
 * cleaner)
 *
 * @TODO The colour lookup code needs to ensure duplicates don't happen.
 *
 * @TODO Do something about the player start points in multiplayer: server
 * should take points from map, shuffle them, and then hand out only the start
 * point for each player (possibly include start points for allies if we have
 * map sharing between allies).  This requires some work on the USPool too (see
 * that file for details).
 *
 * @TODO Build queue(s): Just basic housekeeping this end, the main work is done
 * in the sidebar and input code client side and some sanity checking server
 * side.
 */

Player::Player(const char *pname, INIFile *mapini) : aiId(0) {
    Uint32 mapsize;
    playername = cppstrdup(pname);
    multiside = 0;
    unallycalls = 0;
    playerstart = 0;
    defeated = false;
    if( !strcasecmp(playername, "goodguy") ) {
        playerside = PS_GOOD;
        unitpalnum = 0;
        structpalnum = 0;
    } else if( !strcasecmp(playername, "badguy") ) {
        playerside = PS_BAD;
        unitpalnum = 2;
        structpalnum = 1;
    } else if( !strcasecmp(playername, "neutral") ) {
        playerside = PS_NEUTRAL;
        unitpalnum = 0;
        structpalnum = 0;
    } else if( !strcasecmp(playername, "special") ) {
        playerside = PS_SPECIAL;
        unitpalnum = 0;
        structpalnum = 0;
    } else if( !strncasecmp(playername, "multi", 5) ) {
        playerside = PS_MULTI;
        if (playername[5] < 49 || playername[5] > 57) {
            logger->error("Invalid builtin multi name: %s\n",playername);
            /// @TODO Nicer error handling here
            multiside = 9;
        } else {
            multiside = playername[5] - 48;
        }
        if (multiside > 0) {
            unitpalnum = multiside - 1;
            structpalnum = multiside - 1;
        } else {
            unitpalnum = structpalnum = 0;
        }
        playerstart = p::ppool->getAStart();
        if (playerstart != 0) {
            // conversion algorithm from loadmap.cpp
            playerstart = p::ccmap->normaliseCoord(playerstart);
        }
    } else {
        logger->warning("Player Side \"%s\" not recognised, using gdi instead\n",pname);
        playerside = PS_GOOD;
        unitpalnum = 0;
        structpalnum = 0;
    }
    //mapini->seekSection(playername);
    moneyanim = NULL;
//    changeMoney(mapini->readInt(playername, "Credits", 0) * 100);
    changeMoney(10000);
    powerGenerated = 0;
    powerUsed = 0;
    radarstat = 0;
    unitkills = unitlosses = structurekills = structurelosses = 0;

    //mapini->popLocation();

    mapsize = p::ccmap->getWidth()*p::ccmap->getHeight();
    sightMatrix = new Uint8[mapsize];
    buildMatrix = new Uint8[mapsize];
    mapVis.resize(mapsize);
    mapBld.resize(mapsize);
    allmap = false;
    buildall = false;
    buildany = false;
    memset(sightMatrix,0,mapsize);
    memset(buildMatrix,0,mapsize);
    aiData = new AI::AIPluginData;
    for (Uint32 i=0; i<10; i++) {
        buildqueue[i] = NULL;
    }

    maxqueue = getConfig().maxqueue;
}

Player::~Player()
{
    if (aiId != 0) {
        ps::aiplugman->destroyAI(aiId);
    }
    delete aiData;
    delete[] playername;
    delete[] sightMatrix;
    delete[] buildMatrix;
}

void Player::setVisBuild(SOB_update mode, bool x)
{
    Uint32 i, mapsize;
    std::vector<bool>* mapVoB;
    if (mode == SOB_SIGHT) {
        mapVoB = &mapVis;
        allmap = x;
    } else {
        mapVoB = &mapBld;
        buildany = x;
    }

    mapsize = p::ccmap->getWidth()*p::ccmap->getHeight();
    for( i = 0; i < mapsize; i++ ) {
        (*mapVoB)[i] = x;
    }
}

void Player::setSettings(const char* nick, const char* colour, const char* mside)
{
    if (mside == NULL || strlen(mside) == 0) {
        playerside |= PS_GOOD;
    } else {
        if (strcasecmp(mside,"gdi") == 0) {
            playerside |= PS_GOOD;
        } else if (strcasecmp(mside,"nod") == 0) {
            playerside |= PS_BAD;
        }
    }
    if (colour != NULL && strlen(colour) != 0) {
        setMultiColour(colour);
    }
    if (nick != NULL && strlen(nick) != 0) {
        if (strlen(nick) < strlen(playername)) {
            strncpy(playername, nick, strlen(playername));
        } else {
            delete[] playername;
            playername = cppstrdup(nick);
        }
    }
}

void Player::setAlliances()
{
    INIFile* mapini = p::ppool->getMapINI();
    std::vector<char*> allies_n;
    char *tmp;

    //mapini->pushLocation();
    //mapini->seekSection(playername);
    tmp = mapini->readString(playername, "Allies");
    if( tmp != NULL ) {
        allies_n = splitList(tmp,',');
        delete[] tmp;
    }

    // always allied to self
    allyWithPlayer(this);
    // no initial allies for multiplayer
    if (multiside == 0) {
        for (Uint16 i=0;i<allies_n.size();++i) {
            if (strcasecmp(allies_n[i],playername)) {
                allyWithPlayer(p::ppool->getPlayerByName(allies_n[i]));
            }
            delete[] allies_n[i];
        }
    } else {
        for (Uint16 i=0;i<allies_n.size();++i) {
            delete[] allies_n[i];
        }
    }
    //mapini->popLocation();
    // since this part of the initialisation is deferred until after map
    // is loaded check for no units or structures
    if (unitpool.empty() && structurepool.empty()) {
        defeated = true;
        clearAlliances();
        return;
    }
}

void Player::clearAlliances()
{
    Uint32 i;
    Player* tmp;
    for (i = 0; i < allies.size() ; ++i) {
        if (allies[i] != NULL) {
            tmp = allies[i];
            unallyWithPlayer(tmp);
        }
    }
    for (i = 0; i < non_reciproc_allies.size() ; ++i) {
        if (non_reciproc_allies[i] != NULL) {
            non_reciproc_allies[i]->unallyWithPlayer(this);
        }
    }
}

// todo: check that we aren't duplicating colours
// ideally, the player information should be fed in from the server.
void Player::setMultiColour(const char* colour)
{
    if (strcasecmp(colour,"yellow") == 0) {
        unitpalnum = structpalnum = 0;
        return;
    } else if (strcasecmp(colour,"red") == 0) {
        unitpalnum = structpalnum = 1;
        return;
    } else if (strcasecmp(colour,"blue") == 0) {
        unitpalnum = structpalnum = 2;
        return;
    } else if (strcasecmp(colour,"orange") == 0) {
        unitpalnum = structpalnum = 3;
        return;
    } else if (strcasecmp(colour,"green") == 0) {
        unitpalnum = structpalnum = 4;
        return;
    } else if (strcasecmp(colour,"turquoise") == 0) {
        unitpalnum = structpalnum = 5;
        return;
    }
}

bool Player::decreaseMoney(Uint32 change)
{
    if( money < change ) {
        return false;
    }
    money -= change;
    return true;
}

void Player::builtUnit(Unit* un)
{
    unitpool.push_back(un);

    addSoB(un->getPos(), 1, 1, un->getType()->getSight(),SOB_SIGHT);

    if (defeated) {
        defeated = false;
        p::ppool->playerUndefeated(this);
    }
}

void Player::lostUnit(Unit* un, bool wasDeployed)
{
    Uint32 i;

    removeSoB(un->getPos(), 1, 1, un->getType()->getSight(), SOB_SIGHT);

    if (!wasDeployed) {
        logger->gameMsg("%s has %d structs and %d units", playername, (Uint32)structurepool.size(), (Uint32)unitpool.size()-1);
        if (aiId != 0) {
            // Add stuff to AIPluginData
        }
        ++unitlosses;
    }
    if( unitpool.size() <= 1 && structurepool.empty() && !wasDeployed) {
        logger->gameMsg("Player \"%s\" defeated", playername);
        defeated = true;
        p::ppool->playerDefeated(this);
    } else {
        for (i=0;i<unitpool.size();++i) {
            if (unitpool[i] == un) {
                break;
            }
        }
        for (i=i+1;i<unitpool.size();++i) {
            unitpool[i-1] = unitpool[i];
        }
        unitpool.resize(unitpool.size()-1);
    }
}

void Player::movedUnit(Uint32 oldpos, Uint32 newpos, Uint8 sight)
{
    removeSoB(oldpos, 1, 1, sight, SOB_SIGHT);
    addSoB(newpos, 1, 1, sight, SOB_SIGHT);
}

void Player::builtStruct(Structure* str)
{
    StructureType* st = (StructureType*)str->getType();
    structurepool.push_back(str);
    addSoB(str->getPos(), st->getXsize(), st->getYsize(), 2, SOB_SIGHT);//str->getType()->getSight());
    addSoB(str->getPos(), st->getXsize(), st->getYsize(), 1, SOB_BUILD);
    powerinfo_t newpower = st->getPowerInfo();
    powerGenerated += newpower.power;
    powerUsed += newpower.drain;
    maxstorage += ((StructureType*)str->getType())->getMaxStorage();
    structures_owned[st].push_back(str);
    if (st->primarySettable() && (0 != st->getPType())) {
        production_groups[st->getPType()].push_back(str);
        if ((production_groups[st->getPType()].size() == 1) ||
                (0 == getPrimary(str->getType()))) {
            setPrimary(str);
        }
    }
    if (buildqueue[st->getPType()] == NULL) {
        if (st->getPType()==0 && strcasecmp(((StructureType*)str->getType())->getTName(),"fact") == 0) {
            buildqueue[0] = new BuildQueue(this, 1, 0);
        } else {
            buildqueue[st->getPType()] = new BuildQueue(this, maxqueue,st->getPType());
        }
    }

    if (defeated) {
        defeated = false;
        p::ppool->playerUndefeated(this);
    }
    if (playernum == p::ppool->getLPlayerNum()) {
        p::ppool->updateSidebar();
        p::ppool->updatePowerbar();

        if (radarstat == 0) {
            if ( (strcasecmp(((StructureType*)str->getType())->getTName(),"eye") == 0) ||
                    (strcasecmp(((StructureType*)str->getType())->getTName(),"hq") == 0) ||
		    (strcasecmp(((StructureType*)str->getType())->getTName(),"dome") == 0) ) {
                p::ppool->updateRadar(1);
                radarstat = 1;
            }
        }
    }
}


inline Structure*& Player::getPrimary(UnitOrStructureType* uostype) {
    return primary_structure[uostype->getPType()];
}

inline Structure*& Player::getPrimary(Uint32 ptype) {
    return primary_structure[ptype];
}

bool Player::isOwnedStructure(Structure* str,StructureType* stype) 
{
    std::list<Structure*>::iterator it = std::find(structures_owned[stype].begin(),structures_owned[stype].end(), str);
    if (it==structures_owned[stype].end()) return false;
    else return true;
}

void Player::setPrimary(Structure* str)
{
    StructureType* st = (StructureType*)str->getType();
    if (st->primarySettable()) {
        Structure*& os = getPrimary(st);
        if (0 != os) {
            os->setPrimary(false);
        }
        os = str; // This works because os is a reference.

        str->setPrimary(true);
    }
}

void Player::lostStruct(Structure* str)
{
    StructureType* st = (StructureType*)str->getType();
    std::list<Structure*>& sto = structures_owned[st];
    Uint32 i;
    removeSoB(str->getPos(), ((StructureType*)str->getType())->getXsize(), ((StructureType*)str->getType())->getYsize(), 1, SOB_BUILD);
    powerinfo_t newpower = ((StructureType*)str->getType())->getPowerInfo();
    powerGenerated -= newpower.power;
    powerUsed -= newpower.drain;
    maxstorage -= ((StructureType*)str->getType())->getMaxStorage();
logger->gameMsg("%s has %d structs and %d units", playername, (Uint32)structurepool.size()-1, (Uint32)unitpool.size());

    std::list<Structure*>::iterator it = std::find(sto.begin(), sto.end(), str);
    if (sto.end() == it) {
        logger->error("structures_owned[st] empty already!  This shouldn't happen!\n");
    } else {
        sto.erase(it);
    }
    if (st->primarySettable() && (0 != st->getPType())) {
        std::list<Structure*>& prg = production_groups[st->getPType()];
        it = std::find(prg.begin(), prg.end(), str);
        if (prg.end() == it) {
            logger->error("production_groups[ptype] is already empty! This shouldn't happen!\n");
        } else {
            prg.erase(it);
        }
        if (getPrimary(str->getType()) == str) {
            if (prg.empty()) {
                getPrimary(str->getType()) = 0;
            } else {
                setPrimary(prg.front());
            }
        }
    }
    if (playernum == p::ppool->getLPlayerNum()) {
        p::ppool->updateSidebar();
        p::ppool->updatePowerbar();
         
        if (st->getPType() && (production_groups[st->getPType()].size() == 0)) {
            delete buildqueue[st->getPType()];
            buildqueue[st->getPType()]=NULL;
        }

        if (radarstat == 1) {
            if ((structures_owned[p::uspool->getStructureTypeByName("eye")].empty()) &&
                (structures_owned[p::uspool->getStructureTypeByName("hq")].empty())  &&
                (structures_owned[p::uspool->getStructureTypeByName("dome")].empty())) {
                p::ppool->updateRadar(2);
                radarstat = 0;
            }
        }
    }
    ++structurelosses;
    if (aiId != 0) {
        // Store something in AIPluginData
    }
    if( unitpool.empty() && structurepool.size() <= 1 ) {
        logger->gameMsg("Player \"%s\" defeated", playername);
        defeated = true;
        p::ppool->playerDefeated(this);
    } else {
        for (i=0;i<structurepool.size();++i) {
            if (structurepool[i] == str) {
                break;
            }
        }
        for (i=i+1;i<structurepool.size();++i) {
            structurepool[i-1] = structurepool[i];
        }
        structurepool.resize(structurepool.size()-1);
    }
}

bool Player::isAllied(Player* pl)
{
    for (Uint16 i = 0; i < allies.size() ; ++i) {
        if (allies[i] == pl)
            return true;
    }
    return false;
}

void Player::createAI() {
    aiId = ps::aiplugman->createAI("any","",this);
}

void Player::runAI() {
    if (aiId)
        ps::aiplugman->runAI(aiId, aiData);
}

void Player::updateOwner(Uint8 newnum)
{
    Uint32 i;
    for (i=0;i<unitpool.size();++i)
        unitpool[i]->setOwner(newnum);
    for (i=0;i<structurepool.size();++i)
        structurepool[i]->setOwner(newnum);
}

bool Player::allyWithPlayer(Player* pl)
{
    Uint16 i;
    if (isAllied(pl)) {
        return false;
    }
    if (unallycalls == 0) {
        allies.push_back(pl);
        pl->didAlly(this);
        return true;
    } else {
        for (i=0;i<allies.size();++i) {
            if (allies[i] == NULL) {
                allies[i] = pl;
                pl->didAlly(this);
                --unallycalls;
                return true;
            }
        }
    }
    // shouldn't get here, but in case unallycalls becomes invalid
    allies.push_back(pl);
    pl->didAlly(this);
    unallycalls = 0;
    return true;
}

bool Player::unallyWithPlayer(Player* pl)
{
    Uint16 i;
    if (pl == this) {
        return false;
    }
    for (i=0;i<allies.size();++i) {
        if (allies[i] == pl) {
            allies[i] = NULL;
            pl->didUnally(this);
            ++unallycalls;
            return true;
        }
    }
    return true;
}

void Player::didAlly(Player* pl)
{
    Uint32 i;
    if (isAllied(pl)) {
        return;
    }
    for (i=0;i<non_reciproc_allies.size();++i) {
        if (non_reciproc_allies[i] == pl) {
            // player has reciprocated alliance, remove from one-sided ally list
            non_reciproc_allies[i] = NULL;
            return;
        }
    }
    non_reciproc_allies.push_back(pl);
}

void Player::didUnally(Player* pl)
{
    if (isAllied(pl)) {
        unallyWithPlayer(pl);
    }
}

void Player::placeMultiUnits()
{
    p::uspool->createUnit("MCV",playerstart,0,playernum, 256, 0);
}

void Player::addSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode)
{
    Uint32 curpos, xsize, ysize, cpos;
    Sint32 xstart, ystart;
    std::vector<bool>* mapVoB = NULL;
    static Uint8 brad = getConfig().buildable_radius;

    if (mode == SOB_SIGHT) {
        mapVoB = &mapVis;
    } else if (mode == SOB_BUILD) {
        mapVoB = &mapBld;
        sight  = brad;
    } else {
        logger->error("Can't5~5~ happen? mode: %i\n",mode);
    }
    xstart = pos%p::ccmap->getWidth() - sight;
    xsize = 2*sight+width;
    if( xstart < 0 ) {
        xsize += xstart;
        xstart = 0;
    }
    if( xstart+xsize > p::ccmap->getWidth() ) {
        xsize = p::ccmap->getWidth()-xstart;
    }
    ystart = pos/p::ccmap->getWidth() - sight;
    ysize = 2*sight+height;
    if( ystart < 0 ) {
        ysize += ystart;
        ystart = 0;
    }
    if( ystart+ysize > p::ccmap->getHeight() ) {
        ysize = p::ccmap->getHeight()-ystart;
    }
    curpos = ystart*p::ccmap->getWidth()+xstart;
    for( cpos = 0; cpos < xsize*ysize; cpos++ ) {
        sightMatrix[curpos] += (mode == SOB_SIGHT);
        buildMatrix[curpos] += (mode == SOB_BUILD);
        (*mapVoB)[curpos] = true;
        curpos++;
        if (cpos%xsize == xsize-1)
            curpos += p::ccmap->getWidth()-xsize;
    }
}

void Player::removeSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode)
{
    Uint32 curpos, xsize, ysize, cpos;
    Sint32 xstart, ystart;
    static Uint16 mwid = p::ccmap->getWidth();
    static Uint8 brad = getConfig().buildable_radius;

    if (mode == SOB_BUILD) {
        sight = brad;
    }

    xstart = pos%mwid - sight;
    xsize = 2*sight+width;
    if( xstart < 0 ) {
        xsize += xstart;
        xstart = 0;
    }
    if( xstart+xsize > mwid ) {
        xsize = mwid-xstart;
    }
    ystart = pos/mwid - sight;
    ysize = 2*sight+height;
    if( ystart < 0 ) {
        ysize += ystart;
        ystart = 0;
    }
    if( ystart+ysize > p::ccmap->getHeight() ) {
        ysize = p::ccmap->getHeight()-ystart;
    }
    curpos = ystart*mwid+xstart;
    // I've done it this way to make each loop more efficient. (zx64)
    if (mode == SOB_SIGHT) {
        for( cpos = 0; cpos < xsize*ysize; cpos++ ) {
            // sightMatrix[curpos] will never be < 1 here
            sightMatrix[curpos]--;
            curpos++;
            if (cpos%xsize == xsize-1)
                curpos += mwid-xsize;
        }
    } else if (mode == SOB_BUILD && !buildany) {
        for( cpos = 0; cpos < xsize*ysize; cpos++ ) {
            if (buildMatrix[curpos] <= 1) {
                mapBld[curpos] = false;
                buildMatrix[curpos] = 0;
            } else {
                --buildMatrix[curpos];
            }
            curpos++;
            if (cpos%xsize == xsize-1)
                curpos += mwid-xsize;
        }
    }
}

void Player::changeMoney(Sint32 change)
{
    if (!moneyanim) {
        moneyanim = new MoneyAnimEvent(this,change);
    } else {
        moneyanim->updateMoney(change);
    }
}

Player::MoneyAnimEvent::MoneyAnimEvent(Player* p, Sint32 inimoney) : ActionEvent(1)
{
    player=p;
    newmoney = inimoney;
    ticks = SDL_GetTicks();
    moneyspeed=getConfig().moneyspeed;
    p::aequeue->scheduleEvent(this);
}
 
void Player::MoneyAnimEvent::run()
{
    Uint32 newticks=SDL_GetTicks();
    Uint32 tmpmoney=(newticks-ticks)*100/moneyspeed;
    if (newmoney) ticks=newticks;

    if (newmoney>=(Sint32)tmpmoney) {
        player->setMoney(player->getMoney()+tmpmoney);
        newmoney-=tmpmoney;
    } else if (newmoney<=(-(Sint32)tmpmoney)) {
        if (player->getMoney()>=tmpmoney) {
            player->setMoney(player->getMoney()-tmpmoney);
            newmoney += tmpmoney;
        } else {
            player->setMoney(0);
            newmoney=0;
        }
    } else if (newmoney>0) {
        player->setMoney(player->getMoney()+newmoney);
        newmoney=0;
    } else if (newmoney<0) {
        if (player->getMoney()>=(Uint32)(-newmoney)) {
            player->setMoney(player->getMoney()+newmoney);
        } else {
            player->setMoney(0);
        }
        newmoney=0;
    } else if ((newticks-ticks) > 1000) {
        delete this;
        player->moneyanim=NULL;
        return;
    }
    p::aequeue->scheduleEvent(this);
    p::ppool->updateSidebar();
}

BuildQueue *Player::getBuildQueue(const char *name, Uint8 mode)
{
  UnitType *type;
 
  if(!mode) return buildqueue[0];
  
  type = p::uspool->getUnitTypeByName(name);

  return buildqueue[type->getPType()];
}
 
// return true when sidebar should be redrawed
 
bool Player::updateBuildQueues()
{
  Uint32 ticks = SDL_GetTicks();
  bool ret = false;

  for (Uint8 i=0; i<10; i++) {
      if(buildqueue[i]) {
          buildqueue[i]->update(ticks);
          ret |= buildqueue[i]->updateSidebar();
      }
  }
   
  return ret;
}

PlayerPool::PlayerPool(INIFile* inifile, Uint8 gamemode)
{
    lost = false;
    won = false;
    mapini = inifile;
    updatesidebar = false;
    updatepowerbar = false;
    radarstatus = 0;
    this->gamemode = gamemode;
}

PlayerPool::~PlayerPool()
{
    Uint8 i;
    delete mapini;
    for( i = 0; i < playerpool.size(); i++ ) {
        delete playerpool[i];
    }
}

void PlayerPool::setLPlayer(const char* pname)
{
    Uint8 i;
    for( i = 0; i < playerpool.size(); i++ ) {
        if( !strcasecmp(playerpool[i]->getName(), pname) ) {
            localPlayer = i;
            return;
        }
    }
    //logger->warning("Tried to set local player to non-existing player \"%s\"\n", pname);
    playerpool.push_back(new Player(pname, mapini));
    localPlayer = playerpool.size()-1;
    playerpool[localPlayer]->setPlayerNum(localPlayer);
}

void PlayerPool::setLPlayer(Uint8 number, const char* nick, const char* colour, const char* mside)
{
    Uint8 i;
    for( i = 0; i < playerpool.size(); i++ ) {
        if( playerpool[i]->getMSide() == number ) {
            localPlayer = i;
            playerpool[i]->setSettings(nick,colour,mside);
            return;
        }
    }
    //logger->warning("Tried to set local player to non-existing player number %i\n", number);
    /*  playerpool.push_back(new Player("multi", mapini, this));
        localPlayer = playerpool.size()-1;
        playerpool[localPlayer]->setSettings(nick,colour,mside);
    */
}

Uint8 PlayerPool::getPlayerNum(const char *pname)
{
    Uint8 i;
    for( i = 0; i < playerpool.size(); i++ ) {
        if( !strcasecmp(playerpool[i]->getName(), pname) ) {
            return i;
        }
    }
    playerpool.push_back(new Player(pname, mapini));
    playerpool[playerpool.size()-1]->setPlayerNum(playerpool.size()-1);
    return playerpool.size()-1;
}

Player* PlayerPool::getPlayerByName(const char* pname)
{
    return playerpool[getPlayerNum(pname)];
}

std::vector<Player*> PlayerPool::getOpponents(Player* pl)
{
    std::vector<Player*> opps;
    for(Uint8 i = 0; i < playerpool.size(); i++ ) {
        if (!playerpool[i]->isDefeated() && !pl->isAllied(playerpool[i])) {
            opps.push_back(playerpool[i]);
        }
    }
    return opps;
}

void PlayerPool::playerDefeated(Player *pl)
{
    Uint8 i;

    pl->clearAlliances();
    for( i = 0; i < playerpool.size(); i++ ) {
        if( playerpool[i] == pl ) {
            break;
        }
    }
    if( i == localPlayer ) {
        lost = true;
    }
    if (!lost) {
        Uint8 defeated = 0;
        for (i = 0; i < playerpool.size(); ++i) {
            if (playerpool[i]->isDefeated()) {
                ++defeated;
            }
        }
        if (playerpool.size() - defeated == 1) {
            won = true;
        } else if (playerpool.size() - defeated == playerpool[localPlayer]->getNumAllies() ) {
            won = true;
        }
    }
}

void PlayerPool::playerUndefeated(Player* pl)
{
    Uint8 i;

    pl->setAlliances();
    for( i = 0; i < playerpool.size(); i++ ) {
        if( playerpool[i] == pl ) {
            break;
        }
    }
    if( i == localPlayer ) {
        lost = false;
    }
    if (gamemode == 0) {
        if (!lost) {
            won = false;
        }
    }
}

INIFile* PlayerPool::getMapINI()
{
    return mapini;
}

void PlayerPool::setAlliances()
{
    for (Uint16 i=0; i < playerpool.size() ; ++i) {
        playerpool[i]->setAlliances();
    }
}

void PlayerPool::placeMultiUnits()
{
    for (Uint16 i=0; i < playerpool.size() ; ++i) {
        if (playerpool[i]->getPlayerStart() != 0) {
            playerpool[i]->placeMultiUnits();
        }
    }
}

Uint16 PlayerPool::getAStart()
{
    Uint8 rnd,sze;
    Uint16 rv;
    sze = player_starts.size();
    if (sze == 0) {
        return 0;
    }
    for (rv = 0; rv < sze ; ++rv) {
        if (player_starts[rv] != 0) {
            break;
        }
    }
    if (rv == sze) {
        player_starts.resize(0);
        return 0;
    }

    // pick a starting location at random
    rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
    while (player_starts[rnd] == 0) {
        rnd = (int) ((double)sze*rand()/(RAND_MAX+1.0));
    }
    rv  = player_starts[rnd];
    // ensure this starting location is not reused.
    player_starts[rnd] = 0;
    return rv;
}

void PlayerPool::setWaypoints(std::vector<Uint16> wps)
{
    player_starts = wps;
}

bool PlayerPool::pollSidebar()
{
    if (updatesidebar) {
        updatesidebar = false;
        return true;
    }
    return false;
}
bool PlayerPool::pollPowerbar()
{
    if (updatepowerbar) {
        updatepowerbar = false;
        return true;
    }
    return false;
}
void PlayerPool::updateSidebar()
{
    updatesidebar = true;
}
void PlayerPool::updatePowerbar()
{
    updatepowerbar = true;
}

Uint8 PlayerPool::statRadar()
{
    Uint8 tmp = radarstatus;
    radarstatus = 0;
    return tmp;
}

void PlayerPool::updateRadar(Uint8 status)
{
    radarstatus = status;
}

void PlayerPool::setupAIs() {
    if (gamemode == 1 || gamemode == 0) {
        for (Uint32 i = 1; i < playerpool.size(); ++i) {
            playerpool[i]->createAI();
        }
    }
}

void PlayerPool::runAIs() {
    static Uint32 lasttick = 0;
    if (p::aequeue->getCurtick() > lasttick) {
        lasttick = p::aequeue->getCurtick();
        for (Uint32 i = 0; i < playerpool.size(); ++i) {
            playerpool[i]->runAI();
        }
    }
}

BuildQueue::BuildQueue(Player *player, Uint8 max, Uint8 queuetype)
{
    maxunits = max;
    this->player = player;
    ready = false;
    stopped = false;
    frozen = false;
    queue = new std::priority_queue<char*, std::vector<char*>, CompBuild>;
    t_start = 0;
    t_end = 0;  
    cost = 0;   
    upd = false;
    buildspeed = getConfig().buildspeed;
    qtype=queuetype;
}
 
 
void BuildQueue::start()
{
  /* get cost of the unit or structure */
  cost = p::uspool->getTypeByName(queue->top())->getCost();

  /* set up the timer */
  t_start = SDL_GetTicks();;
  t_end = t_start + cost * buildspeed;
  money = 0;
}
 
int BuildQueue::add(const char *name)
{
  if (queue->empty() &&
    p::uspool->getTypeByName(name)->getCost()>player->getMoney()) {
      return BQUEUE_NOMONEY;
  }

  char *item;
  
  item = new char[14];
  strcpy(item, name); 
  
  queue->push(item);
  upd = true;
  if(queue->size() == 1)
    start();
  else
    queuecounts[name]++;
  
  return BQUEUE_CONSTRUCT;
}

void BuildQueue::update(Uint32 ticks)
{
  Uint32 newmoney;
  Sint32 total;   
  
  total = t_end - t_start;
  frozen = false;

  if(!queue->empty() && !ready) {
      if(!stopped) {
          if (total) newmoney = ((ticks - t_start) * cost) / total;
          else newmoney=money;

          if(newmoney >= cost) newmoney = cost;  

          /* check money */
          if((newmoney-money)>player->getMoney()) {
              money+=player->getMoney();
              player->changeMoney(-player->getMoney());
              frozen = true;
          } else {
              frozen = false;
              if(newmoney == cost) {
                  ready = true;
                  if(qtype)
                    if (getConfig().gamenum == GAME_RA)
                      pc::sfxeng->queueSound("unitrdy1.aud");
                    else pc::sfxeng->queueSound("unitredy.aud");
                  else if (getConfig().gamenum == GAME_RA)
                    pc::sfxeng->queueSound("conscmp1.aud");
                  else pc::sfxeng->queueSound("constru1.aud");
              }
//              pc::sfxeng->queueSound("clock1.aud");
              player->changeMoney(money-newmoney);
              money = newmoney;
          }

          upd = true;
      }

      if (frozen || stopped) {
           /* when build has been stopped/frozen then adjust t_end/t_start */
           t_end = ticks + (cost - money) * buildspeed;
           t_start = t_end - total;
          
           upd = true;
      }
  }
   
  /* below is a temporary ugly early morning hack */
  if(ready && qtype && !stopped) {
      Structure* tmpstruct = player->getPrimary(p::uspool->getUnitTypeByName(queue->top()));
      Uint16 pos = POS_INVALID;
      if (0 != tmpstruct) {
          pos = tmpstruct->getFreePos(p::uspool->getUnitTypeByName(queue->top()));
      } else {
          logger->error("No primary building set for %s\n",queue->top());
      }

      if (pos != POS_INVALID) {
      /// @TODO run weap animation (let unit exit weap)
          //tmpstruct->runAnim(5);
      /// @TODO
          //p::uspool->createUnit(queue->top(), pos, 0,player->getPlayerNum(), 256, 0);
          p::dispatcher->unitCreate(queue->top(),pos, 0, player->getPlayerNum());
          //tmpstruct->runAnim(5,true);
          done();
      } else {
          stopped=true;
          logger->error("No free position for %s\n", queue->top());
      } /// @TODO: give message, set unit to ready in sidebar
  }
}  
   
bool BuildQueue::updateSidebar()
{
  if(upd)
    {
      upd = false;
      return true;
    }
     
  return false;
}
 
bool BuildQueue::isFull()
{
  return !(queue->empty() || queue->size() < maxunits);
}

void BuildQueue::getStatus(const char *name, Uint8 *num, bool *grayed, bool *stopped, bool *frozen, bool *ready, double *angle)
{
  *grayed = false;
  *stopped = false;
  *frozen = false;
  *ready = false;  
  *angle = M_PI * 2;
  std::string sname = (std::string)name;
  
  *num = queuecounts[sname];
  
  if(!queue->empty())
    {
      *grayed = true;
      
      if(strcasecmp(name, queue->top()) == 0)
        {
          if(cost)
            *angle = (double)money * M_PI * 2 / (double)cost;
          else
            *angle = 0.0;
          
          *ready = this->ready;
          *stopped = this->stopped;
          *frozen = this->frozen;
        }
      else
        { 
          *angle = 0.0;
          *ready = false;
          *stopped = false;
          *frozen = false;
        }
    }
}
 
int BuildQueue::stop()
{
  if(!queue->empty())
    {
      stopped = true;
      frozen = false;
      upd = true;
      return BQUEUE_STOPPED;
    }
     
  return 0;
}
 
void BuildQueue::done()
{
  if(!queue->empty())
    {
      delete queue->top();
      queue->pop();
      ready = false;
      stopped = false;
      frozen = false;
      upd = true;
      t_start = 0;
      t_end = 0;  
      money = 0;  
      cost = 0;   
    }
     
  if(!queue->empty())
    {
      if(queuecounts[queue->top()])
        queuecounts[queue->top()]--;
      start();
    }
}
 
int BuildQueue::cancel()
{
  if(!queue->empty())
    {
      player->changeMoney(money);
      done();
      return BQUEUE_ABORTED;
    }
     
  return 0;
}
 
int BuildQueue::resume()
{
  stopped = false;
  if(ready) {
      return BQUEUE_READY;
  } else {
      upd = true;
      return BQUEUE_CONSTRUCT;
  }
}
 
bool BuildQueue::isTop(const char *name)
{
  if(queue->empty()) return false;
  
  return ( strcasecmp(queue->top(), name) == 0 );
}
 
int BuildQueue::click(const char *name, Uint8 button)
{
  std::string sname = (std::string)name;
  
  if(button == SDL_BUTTON_LEFT)
    {
      if(isTop(name))
        {
          if(stopped) return resume();
          if(ready) return BQUEUE_PLACING;
          if(frozen) return BQUEUE_FROZEN;
        }
         
      if(!isFull()) return add(name);
      
      return BQUEUE_FULL;
    }
     
  if(button == SDL_BUTTON_RIGHT)
    {
      if(isTop(name))
        {
          if(stopped || ready) return cancel();
          
          return stop();
        }
    }

  return 0;
}
 
BuildQueue::~BuildQueue()
{
    char *item;
  
    while(!queue->empty()) {
        item = queue->top();
        queue->pop();
        delete item; 
    }
     
   delete queue;
}


