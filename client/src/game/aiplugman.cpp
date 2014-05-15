#include <string>
#include <map>
#include "actioneventqueue.h"
#include "aiplugman.h"
#include "ccmap.h"
#include "dllibrary.h"
#include "logger.h"
#include "message.h"
#include "playerpool.h"
#include "unitandstructurepool.h"

namespace AI {

AIPlugMan::AILib::AILib() {
    lib = 0;
}

AIPlugMan::AILib::~AILib() {
    delete lib;
}

AIPlugMan::AIStateData::AIStateData() {
    instance = 0;
    player = 0;
}

AIPlugMan::AIPlugMan(const char* bp) : nextid(1) {
    this->binpath = bp + std::string("/");
    //VFS_Scan("aip");
}

Uint32 AIPlugMan::createAI(const std::string& name, const std::string& options, Player* player) {
    std::string libname;
    std::map<std::string, AILib*>::iterator curLib;
    AILib* lib;
    AIStateData* state;

    if (name == "any") {
        libname = binpath+"test.aip";
    } else {
        libname = binpath+name;
    }
    curLib = loadedLibs.find(libname);

    if (curLib == loadedLibs.end()) {
        lib = new AILib();
        // load the library
        try {
            lib->lib = new DLLibrary(libname.c_str());
        } catch(int) {
            logger->error("Unable to open library \"%s\"\n", libname.c_str());
            return 0;
        }
        lib->init = (PFDLINITAI)lib->lib->getFunction("initAI");
        lib->close = (PFDLSHUTDOWNAI)lib->lib->getFunction("shutdownAI");
        lib->run = (PFDLRUNAI)lib->lib->getFunction("runAI");
        if (lib->init == 0 || lib->close == 0 || lib->run == 0) {
            logger->error("Invalid AI plugin\n");
            return 0;
        }
        lib->refcount = 0;
        loadedLibs[libname] = lib;
    } else {
        lib = curLib->second;
    }
    lib->refcount++;
    state = new AIStateData();
    curplayer = player;
    state->id = nextid++;
    idStateLut[state->id] = state;
    idLibLut[state->id]   = lib;
    state->instance       = lib->init(options.c_str(), this);
    state->player         = player;
    return state->id;
}

AIPlugMan::~AIPlugMan() {
    std::map<std::string, AILib*>::iterator curLib;
    for (curLib = loadedLibs.begin(); curLib != loadedLibs.end(); ++curLib) {
        delete curLib->second;
    }
}

void AIPlugMan::destroyAI(Uint32 id) {
    AILib* lib;
    if (id == 0) {
        // Zero is an invalid id
        return;
    }
    lib = idLibLut[id];
    lib->close(idStateLut[id]->instance);
    delete idStateLut[id];
    lib->refcount--;
}

void AIPlugMan::runAI(Uint32 id, const AIPluginData* data) {
    if (id == 0) {
        return;
    }
    curdata   = data;
    curstate  = idStateLut[id];
    curplayer = curstate->player;
    idLibLut[id]->run(curstate->instance);
}


// Theses are the functions used to interact with the plugin
void AIPlugMan::registerTrigger(Trigger *trig) {
}

Trigger *AIPlugMan::getActivatedTrigger() {
    return NULL;
}

Uint16 AIPlugMan::getMapWidth(){
    return p::ccmap->getWidth();
}

Uint16 AIPlugMan::getMapHeight(){
    return p::ccmap->getHeight();
}

Uint8 *AIPlugMan::getMapLayout(){
    return curdata->mapLayout;
}

Uint32 AIPlugMan::getUnit(Uint8 mask, AIUnit **units){
    return 0;
}
Uint32 AIPlugMan::getStructure(Uint8 mask, AIUnit **structs){
    return 0;
}

Uint32 AIPlugMan::getUnitAt(Uint8 mask, Uint32 pos, Uint16 radius, AIUnit **units){
    return 0;
}
Uint32 AIPlugMan::getStructureAt(Uint8 mask, Uint32 pos, Uint16 radius, AIUnit **structs){
    return 0;
}

void AIPlugMan::buildUnit(AIUnitType* type) {
}

void AIPlugMan::buildStructure(AIStructureType* type){
}

void AIPlugMan::placeStructure(AIStructure* str, Uint32 pos){
}
Uint32 AIPlugMan::getBuildableUnits(AIUnitType** types){
    return 0;
}
Uint32 AIPlugMan::getBuildableStructures(AIStructureType **types){
    return 0;
}

Uint32 AIPlugMan::getNumUnits(){
    return 0;
}
Uint32 AIPlugMan::getNumStructures(){
    return 0;
}

void AIPlugMan::giveUnitOrder(AIUnit *un, Order *order){
}
void AIPlugMan::giveStructureOrder(AIStructure *st, Order *order){
}

Uint32 AIPlugMan::getPathCost(Uint32 start, Uint32 end){
    return 0;
}

Uint32 AIPlugMan::getMoney(){
    return 0;
}
Uint32 AIPlugMan::getPower(){
    return 0;
}

const char* AIPlugMan::getName() {
    return curplayer->getName();
}

Uint32 AIPlugMan::getTime() {
    return p::aequeue->getCurtick();
}

void AIPlugMan::sayAll(const char* txt) {
    pc::msg->postMessage(txt);
}

// Close AI namespace
}
