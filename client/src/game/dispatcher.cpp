#include "actioneventqueue.h"
#include "ccmap.h"
#include "common.h"
#include "config.h"
#include "dispatcher.h"
#include "logger.h"
#include "playerpool.h"
#include "structure.h"
#include "unit.h"
#include "unitorstructure.h"
#include "unitandstructurepool.h"
#include "vfs.h"

using std::string;

/** NOTE: I've stripped out the sections related to logging and playback as that
 * part isn't as stable as the rest (basically need to fix a horrible synch
 * issue with the playback)
 */

Dispatcher::Dispatcher() :
    logstate(Disp::NORMAL),
    localPlayer(p::ppool->getLPlayerNum())
{
}

Dispatcher::~Dispatcher()
{
    switch (logstate) {
    case Disp::RECORDING:
        break;
    case Disp::PLAYING:
        break;
    case Disp::NORMAL:
    default:
        break;
    }
}

void Dispatcher::unitHarvest(Unit* un, Uint32 dest)
{
    if (un == 0) {
        return;
    }
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            un->harvest(dest);
            break;
        case Disp::PLAYING:
        default:
            break;
    }
}

void Dispatcher::unitMove(Unit* un, Uint32 dest)
{
    if (un == 0) {
        return;
    }
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            un->move(dest);
            break;
        case Disp::PLAYING:
        default:
            break;
    }
}

void Dispatcher::unitAttack(Unit* un, UnitOrStructure* target, bool tisunit)
{
    if (un == 0) {
        return;
    }
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            un->attack(target);
            break;
        case Disp::PLAYING:
        default:
            break;
    }
}

void Dispatcher::unitDeploy(Unit* un)
{
    if (un == 0) {
        return;
    }
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            un->deploy();
            break;
        case Disp::PLAYING:
        default:
            break;
    }
}

void Dispatcher::unitCreate(const char* tname, Uint32 pos, Uint8 subpos, Uint8 owner)
{
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            p::uspool->createUnit(tname,pos,subpos,owner,256,0);
            break;
        case Disp::PLAYING:
        default:
            break;
    };
}

void Dispatcher::structureAttack(Structure* st, UnitOrStructure* target, bool tisunit)
{
    if (st == 0) {
        return;
    }
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            st->attack(target);
            break;
        case Disp::PLAYING:
        default:
            break;
    }
}

void Dispatcher::structureCreate(const char* tname, Uint32 pos, Uint8 owner)
{
    switch (logstate) {
        case Disp::RECORDING:
            // deliberate fallthrough
        case Disp::NORMAL:
            p::uspool->createStructure(tname,pos,owner,256,0,true);
            break;
        case Disp::PLAYING:
        default:
            break;
    };
}
