#ifndef AIPLUGINMAN_H
#define AIPLUGINMAN_H

#include "aistubinterface.h"

#include <string>
#include <map>
#include <vector>

class DLLibrary;
class Player;

namespace AI {

struct AIPluginData {
    Uint8* mapLayout;
    std::vector<Trigger*> activatedTriggers;
    std::vector<AIUnit*> visibleUnits;
};

class AIPlugMan : public AIStubInterface{
public:
    AIPlugMan(const char* binpath);
    ~AIPlugMan();

    Uint32 createAI(const std::string& name, const std::string& options, Player* player);
    void destroyAI(Uint32 id);
    void runAI(Uint32 id, const AIPluginData*);

    // Theses are the functions used to interact with the plugin
    void registerTrigger(Trigger *trig);
    Trigger *getActivatedTrigger();

    Uint16 getMapWidth();
    Uint16 getMapHeight();
    Uint8 *getMapLayout();

    Uint32 getUnit(Uint8 mask, AIUnit **units);
    Uint32 getStructure(Uint8 mask, AIUnit **structs);
    Uint32 getUnitAt(Uint8 mask, Uint32 pos, Uint16 radius, AIUnit **units);
    Uint32 getStructureAt(Uint8 mask, Uint32 pos, Uint16 radius, AIUnit **structs);

    void buildUnit(AIUnitType *type);
    void buildStructure(AIStructureType *type);
    void placeStructure(AIStructure *stru, Uint32 pos);
    Uint32 getBuildableUnits(AIUnitType **types);
    Uint32 getBuildableStructures(AIStructureType **types);

    Uint32 getNumUnits();
    Uint32 getNumStructures();

    void giveUnitOrder(AIUnit *un, Order *order);
    void giveStructureOrder(AIStructure *st, Order *order);

    Uint32 getPathCost(Uint32 start, Uint32 end);

    Uint32 getMoney();
    Uint32 getPower();

    const char* getName();
    Uint32 getTime();

    void sayAll(const char*);
private:
    Uint32 nextid;
    std::string binpath;

    struct AILib {
        AILib();
        ~AILib();
        DLLibrary* lib;
        Uint32 refcount;

        PFDLINITAI init;
        PFDLSHUTDOWNAI close;
        PFDLRUNAI run;
    };

    struct AIStateData {
        AIStateData();
        Uint32 id;
        void* instance;
        Player* player;
    };

    std::map<std::string, AILib*> loadedLibs;
    std::map<Uint32, AIStateData*> idStateLut;
    std::map<Uint32, AILib*> idLibLut;

    // These are only valid inside runAI
    Player* curplayer;
    AIStateData* curstate;
    const AIPluginData*  curdata;
};

}

#endif
