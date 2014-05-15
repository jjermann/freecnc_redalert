// mode: -*- C++ -*-
#ifndef PLAYERPOOL_H
#define PLAYERPOOL_H

#include <list>
#include <map>
#include <queue>
#include <vector>
#include "SDL_types.h"
#include "common.h"
#include "actioneventqueue.h"

#define BQUEUE_NOMONEY   1  /* not enough money */
#define BQUEUE_CONSTRUCT 2  /* construction started */
#define BQUEUE_STOPPED   3  /* stopped */
#define BQUEUE_FROZEN    4  /* frozen for missing money */
#define BQUEUE_ABORTED   5
#define BQUEUE_FULL      6  /* queue is full */
#define BQUEUE_READY     7  /* unit ready */
#define BQUEUE_PLACING   8  /* placing a struct */

namespace AI {
    class AIPluginData;
}
class INIFile;
class PlayerPool;
class CnCMap;
class Unit;
class UnitType;
class Structure;
class StructureType;
class UnitOrStructure;
class UnitOrStructureType;
class UnitAndStructurePool;
class Player;

class BuildQueue
{  
      friend class CompBuild;
public:
      BuildQueue(Player* player, Uint8 max, Uint8 queuetype);
      ~BuildQueue();
      int click(const char *name, Uint8 button);
      void update(Uint32 ticks);
      /* outputs the current status for a button by name */
      void getStatus(const char *name, Uint8 *num, bool *grayed, bool *stopped, bool *frozen, bool *ready, double *angle);
      bool updateSidebar();
      int add(const char *name);
      int stop();
      int cancel();
      int resume();
      void done(); 
      bool isFull();
      bool isTop(const char *name);
      int queueType()
      {
        return qtype;
      }
      int buildspeed;
private:
      void start();
      Uint32 maxunits; /* max units in queue */
      Uint32 t_start, t_end;
      Uint32 cost;     /* cost of the top item */
      Uint32 money;    /* money spent on top item until now */
      bool stopped;    /* top item has been stopped */
      bool frozen;     /* top item has been frozen */
      bool ready;      /* top item is done */
      bool upd;        /* when sidebar should be updated */
      int qtype;   /* is a unit queue */
      Player *player;
      std::priority_queue<char*, std::vector<char*>, CompBuild> *queue;
      std::map<std::string, Uint8> queuecounts;
};
  
class CompBuild
{
public:
    bool operator()(char *x, char *y) {
       return 0;
    }
};   

class Player
{
friend class BuildQueue;
public:
    explicit Player(const char *pname, INIFile *mapini);
    ~Player();
    void setPlayerNum(Uint8 num)
    {
        playernum = num;
    }
    void setMultiColour(const char* colour);
    void setSettings(const char* nick, const char* colour, const char* mside);
    Uint8 getPlayerNum()
    {
        return playernum;
    }
    const char* getName()
    {
        return playername;
    }
    int getSide()
    {
        return playerside;
    }
    void increaseMoney(Uint32 change)
    {
        money += change;
    }
    bool decreaseMoney(Uint32 change);
    Uint32 getMoney()
    {
        return money;
    }
    void setMoney(Uint32 newmoney)
    {
        if (newmoney<money) {
           if ((money-newmoney)<storage) storage-=(money-newmoney);
           else storage=0;
        }
        money=newmoney;
    }

    void builtUnit(Unit* un);
    void lostUnit(Unit* un, bool wasDeployed);
    void movedUnit(Uint32 oldpos, Uint32 newpos, Uint8 sight);
    Uint16 getNumUnits()
    {
        return unitpool.size();
    }
    void builtStruct(Structure* str);
    void lostStruct(Structure* str);
    Structure*& getPrimary(UnitOrStructureType* uostype);
    Structure*& getPrimary(Uint32 ptype);
    Uint8 getMSide()
    {
        return multiside;
    }
    Uint16 getNumStructs()
    {
        return structurepool.size();
    }
    Uint8 getStructpalNum()
    {
        return structpalnum;
    }
    Uint8 getUnitpalNum()
    {
        return unitpalnum;
    }
    bool isAllied(Player* pl);
    Uint16 getNumAllies()
    {
        return allies.size() - unallycalls;
    }
    Uint32 getPower()
    {
        return powerGenerated;
    }
    Uint32 getPowerUsed()
    {
        return powerUsed;
    }
    Uint16 getPlayerStart()
    {
        return playerstart;
    }
    void createAI();
    void runAI();

    void updateOwner(Uint8 newnum);
    std::vector<Unit*> getUnits()
    {
        return unitpool;
    }
    std::vector<Structure*> getStructures()
    {
        return structurepool;
    }
    std::list<Structure*> getStructures(StructureType* stype)
    {
        return structures_owned[stype];
    }
    bool isOwnedStructure(Structure* str,StructureType* stype);
    bool isDefeated()
    {
        return defeated;
    }
    bool allyWithPlayer(Player* pl);
    void didAlly(Player* pl);
    bool unallyWithPlayer(Player* pl);
    void didUnally(Player* pl);
    void setAlliances();
    void clearAlliances();
    void addUnitKill()
    {
        ++unitkills;
    }
    void addStructureKill()
    {
        ++structurekills;
    }
    Uint16 getUnitKills()
    {
        return unitkills;
    }
    Uint16 getUnitLosses()
    {
        return unitlosses;
    }
    Uint16 getStructureKills()
    {
        return structurekills;
    }
    Uint16 getStructureLosses()
    {
        return structurelosses;
    }
    void placeMultiUnits();
    Uint32 ownsStructure(StructureType* stype)
    {
        return structures_owned[stype].size();
    }
    void setPrimary(Structure* str);
    std::vector<bool>& getMapVis()
    {
        return mapVis;
    }

    // SOB == Sight or Build.
    enum SOB_update {
        SOB_SIGHT = 1, SOB_BUILD = 2
    };

    void setVisBuild(SOB_update mode, bool x);
    std::vector<bool>& getMapBuildable()
    {
        return mapBld;
    }
    /// Turns on a block of cells in either the sight or buildable matrix
    void addSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode);
    /// Turns off a block of cells in either the sight or buildable matrix
    void removeSoB(Uint32 pos, Uint8 width, Uint8 height, Uint8 sight, SOB_update mode);
    bool canBuildAll()
    {
        return buildall;
    }
    bool canBuildAny()
    {
        return buildany;
    }
    bool canSeeAll()
    {
        return allmap;
    }
    void enableBuildAll()
    {
        buildall = true;
        /* ugly: we must have the queues to build all */
        if(!buildqueue[0]) buildqueue[0] = new BuildQueue(this, 1, 0);
        for (Uint8 i=1; i<10; i++) {
            if(!buildqueue[i]) buildqueue[i] = new BuildQueue(this, 11, i);
        }
    }
    BuildQueue *getBuildQueue(const char *name, Uint8 mode);
    bool updateBuildQueues();
    void changeMoney(Sint32 newmoney);
    Uint32 getStorage() {
        return storage;
    }
    void setStorage(Uint32 amount) {
        storage=amount;
    }   
    Uint32 getMaxStorage() {
        return maxstorage;
    }
    void setMaxStorage(Uint32 amount) {
        maxstorage=amount;
    }   
private:
    // Do not want player being constructed using default constructor
    Player() {};
    Player(const Player&) {};
        
    class MoneyAnimEvent : public ActionEvent {
    public: 
        MoneyAnimEvent(Player* p, Sint32 inimoney);
        void run();
        void updateMoney(Sint32 change) {
          newmoney+=change;
        }
    private:
        int moneyspeed;
        Uint32 ticks;
        Sint32 newmoney;
        Player* player;
    };
    friend class MoneyAnimEvent;
    MoneyAnimEvent* moneyanim;
    
    Uint32 aiId;
    bool defeated;
    char *playername, *nickname;
    Uint8 playerside, multiside, playernum, radarstat, unitpalnum, structpalnum;

    // See the alliance code in the .cpp file
    Uint8 unallycalls;

    Uint32 money;
    Uint32 powerGenerated, powerUsed;

    Uint32 maxstorage,storage;
    
    Uint16 unitkills, unitlosses, structurekills, structurelosses;

    Uint16 playerstart;

    // All of these pointers are owned elsewhere.
    std::vector<Unit*> unitpool;
    std::vector<Structure*> structurepool;
    std::map<StructureType*, std::list<Structure*> > structures_owned;
    std::map<Uint32, std::list<Structure*> > production_groups;
    std::map<Uint32, Structure*> primary_structure;


    std::vector<Player*> allies;
    // players that have allied with this player, but this player
    // has not allied in return.  Used to force an unally when player
    // is defeated.
    std::vector<Player*> non_reciproc_allies;

    Uint8* sightMatrix;
    Uint8* buildMatrix;

    std::vector<bool> mapVis;
    std::vector<bool> mapBld;
    // cheat/debug flags: allmap (reveal all map), buildany (remove
    // proximity check) and buildall (disable prerequisites).
    bool allmap, buildany, buildall;

    AI::AIPluginData* aiData;

    BuildQueue* buildqueue[10]; /* build queue */
    int maxqueue;
};

/** @todo Currently the player starts are shuffled randomly without any way of
 * accepting a preshuffled list.
 */
class PlayerPool
{
public:
    explicit PlayerPool(INIFile* inifile, Uint8 gamemode);
    ~PlayerPool();
    Uint8 getNumPlayers() const {
        return playerpool.size();
    }
    Uint8 getLPlayerNum() const {
        return localPlayer;
    }
    Player *getLPlayer()
    {
        return playerpool[localPlayer];
    }
    void setLPlayer(const char *pname);
    void setLPlayer(Uint8 number, const char* nick, const char* colour, const char* mside);
    Player *getPlayer(Uint8 player)
    {
        return playerpool[player];
    }
    Uint8 getPlayerNum(const char *pname);
    Player* getPlayerByName(const char* pname);

    Uint8 getUnitpalNum(Uint8 player) const {
        return playerpool[player]->getUnitpalNum();
    }
    Uint8 getStructpalNum(Uint8 player) const {
        return playerpool[player]->getStructpalNum();
    }
    std::vector<Player*> getOpponents(Player* pl);
    void playerDefeated(Player *pl);
    void playerUndefeated(Player *pl);
    bool hasWon() const {
        return won;
    }
    bool hasLost() const {
        return lost;
    }
    void setAlliances();
    void placeMultiUnits();
    INIFile* getMapINI();
    Uint16 getAStart();
    void setWaypoints(std::vector<Uint16> wps);

    /// Called by input to see if sidebar needs updating
    bool pollSidebar();
    /// Called by the local player when sidebar is to be updated
    void updateSidebar();
    /// Called by input to see if powerbar needs updating
    bool pollPowerbar();
    /// Called by the local player when powerbar is to be updated
    void updatePowerbar();

    /// Called by input to see if radar status has changed.
    Uint8 statRadar();

    /// Called by the local player to update the radar status
    void updateRadar(Uint8 status);

    /// Called by game
    void runAIs();

    /// Called by game
    void setupAIs();
private:
    PlayerPool();
    PlayerPool(const PlayerPool&);

    std::vector<Player *> playerpool;
    std::vector<Uint16> player_starts;
    Uint8 localPlayer, gamemode, radarstatus;
    bool won, lost, updatesidebar, updatepowerbar;
    INIFile* mapini;
};

#endif
