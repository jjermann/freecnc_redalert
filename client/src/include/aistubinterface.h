#ifndef AISTUBINTERFACE_H
#define AISTUBINTERFACE_H

#include "SDL_types.h"

class Unit;
class Structure;
class UnitType;
class StructureType;

namespace AI {

struct Order {
    // ENTERU means enter a unit, ENTERS means enter a structure
    enum {MOVE, ATTACK, DEPLOY, ENTERU, ENTERS} type;
    /** @note Meaning of param:
     * move: dest cell
     * attack: id of target
     * deploy: ignored
     * enteru: id of target
     * enters: id of target */
    Uint32 param;
    bool immed;
};

// this will probably contain a trigger type and a union containing relevant data
class Trigger;

struct AIUnitType {
    Uint32 firepower;
    Uint32 speed;
    Uint32 visRange;
    Uint32 range;
    Uint32 cost;
    Uint32 armour;
    Uint32 maxHealth;

    bool attacksAir;
    bool canHarvest;
    Uint8 moveType; //walk/wheels/tracks/air/water
    Uint8 possibleVisStatus;

    UnitType *typeID;
};

struct AIStructureType {
    Uint32 firepower;
    Uint32 visRange;
    Uint32 range;
    Uint32 cost;
    Uint32 armour;
    Uint32 powerConsumption;
    Uint32 maxHealth;

    bool attacksAir;
    Uint8 canBeEntered; // including which types can enter it
    Uint8 possibleVisStatus;

    StructureType *typeID;
};

struct AIUnit {
    Uint32 pos;
    Uint32 health;

    Uint8 visStatus;
    Uint8 owner;

    AIUnitType *type;
    Order *curOrder;

    Unit *unitID;
};

struct AIStructure {
    Uint32 pos;
    Uint32 health;

    Uint8 visStatus;
    Uint8 owner;

    AIUnitType* type;
    Order* curOrder;

    Structure* structureID;
};

class AIStubInterface {
public:
    virtual ~AIStubInterface(){};

    /* Functions to register a trigger and get the activated triggers
     * one by one until there are no more activated triggers and NULL is 
     * returned */
    virtual void registerTrigger(Trigger *trig) = 0;
    virtual Trigger *getActivatedTrigger() = 0;

    /* Functions to get the layout of the map. The layout array consists of
     * terrain type (land, water, blocked) occupied + unit/structure + enemy/friendly
     * and contais tiberium */
    virtual Uint16 getMapWidth() = 0;
    virtual Uint16 getMapHeight() = 0;
    virtual Uint8 *getMapLayout() = 0;

    /* Stores a list of the units/structures specified by mask in a list
     * allocated in *units/structs and returns the number of units/structures in
     * that list. Contents
     * undefined */
    virtual Uint32 getUnit(Uint8 mask, AIUnit **units) = 0;
    virtual Uint32 getStructure(Uint8 mask, AIUnit **structs) = 0;
    /* Same as the above functions but instead of all units/structures matching
     * the mask only those at most radius tiles away from pos is returned. To
     * get your unit at pos 3 you would call: getUnitAt(AIUS_OWNED, 3, 0,
     * &unitptr); meaning get all my units which are 0 or less tiles from
     * position 3. This can be up to 3 units if they are infantry. */
    virtual Uint32 getUnitAt(Uint8 mask, Uint32 pos, Uint16 radius, AIUnit **units) = 0;
    virtual Uint32 getStructureAt(Uint8 mask, Uint32 pos, Uint16 radius, AIUnit **structs) = 0;

    /* Used to build units and structures. The tech tree might have to be
     * accessed to, but that's a
     * later problem */
    virtual void buildUnit(AIUnitType *type) = 0;
    virtual void buildStructure(AIStructureType *type) = 0;
    virtual void placeStructure(AIStructure *stru, Uint32 pos) = 0;
    virtual Uint32 getBuildableUnits(AIUnitType **types) = 0;
    virtual Uint32 getBuildableStructures(AIStructureType **types) = 0;

    // Number of units/structures the player the AI is playing has
    virtual Uint32 getNumUnits() = 0;
    virtual Uint32 getNumStructures() = 0;

    // Order the unit/structure to do something
    virtual void giveUnitOrder(AIUnit *un, Order *order) = 0;
    virtual void giveStructureOrder(AIStructure *st, Order *order) = 0;

    // calculate the cost of the path, be careful with this one, it will be slow
    virtual Uint32 getPathCost(Uint32 start, Uint32 end) = 0;

    // How much money/power the player has
    virtual Uint32 getMoney() = 0;
    virtual Uint32 getPower() = 0;

    virtual const char* getName() = 0;
    virtual Uint32 getTime() = 0;

    virtual void sayAll(const char*) = 0;
};

/* these are the functions the plugin must support. They should be called:
 * void* initAI(const char *options, AIStubInterface *stub)
 * void shutdownAI(void *AI)
 * void runAI(void *AI)
 * where the void* returned from init and passed to the other funcs is a
 * identifier for one instance of the ai */
typedef void* ( *PFDLINITAI )( const char*, AIStubInterface* );
typedef void ( *PFDLSHUTDOWNAI )( void* );
typedef void ( *PFDLRUNAI )( void* );

/* Defines so you don't have to specify the bits yourself =) */
//AI Terrain Type
static const Uint32 AITT_UNKNOWN = 0xff;
static const Uint32 AITT_GROUND = 0x80;
static const Uint32 AITT_WATER = 0x40;
static const Uint32 AITT_BLOCKED = 0x20;
static const Uint32 AITT_HASRESOURCE = 0x10;

//Tells you if this tile has a unit or structure
static const Uint32 AIMAP_HASUNIT = 0x8;
static const Uint32 AIMAP_HASSTRUCTURE = 0x4;

//Tells you if the unit/structure on this tile is a enemy
static const Uint32 AIMAP_ENEMYUS = 0x2;
static const Uint32 AIMAP_ALLYUS = 0x1;

/* defines for the general getUnit/Structure stuff */
// enemy, owned and allies can be combined in any way..
static const Uint32 AIUS_ENEMY = 0x80;
static const Uint32 AIUS_OWNED = 0x40;
static const Uint32 AIUS_ALLIES = 0x20;
// not that idle only works in combination with own units
static const Uint32 AIUS_IDLE = 0x10;

}

#endif

