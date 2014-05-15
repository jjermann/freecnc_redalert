#include <cmath>
#include "ccmap.h"
#include "common.h"
#include "logger.h"
#include "path.h"
#include "projectileanim.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "unitanimations.h"
#include "unitqueue.h"
#include "weaponspool.h"

UnitAnimEvent::UnitAnimEvent(Uint32 p, Unit* un) : ActionEvent(p)
{
    //logger->debug("UAE cons: this:%p un:%p\n",this,un);
    this->un = un;
    un->referTo();
    scheduled = NULL;
}

UnitAnimEvent::~UnitAnimEvent()
{
    //logger->debug("UAE dest: this:%p un:%p sch:%p\n",this,un,scheduled);
    if (scheduled != NULL) {
        p::aequeue->scheduleEvent(scheduled);
    }
    un->unrefer();
}

void UnitAnimEvent::setSchedule(UnitAnimEvent* e)
{
    //logger->debug("Scheduling an event. (this: %p, e: %p)\n",this,e);
    if (scheduled != NULL) {
        scheduled->setSchedule(NULL);
        scheduled->stop();
    }
    scheduled = e;
}

void UnitAnimEvent::stopScheduled()
{
    if (scheduled != NULL) {
        scheduled->stop();
    }
}

MoveAnimEvent::MoveAnimEvent(Uint32 p, Unit* un) : UnitAnimEvent(p,un)
{
    //logger->debug("MoveAnim cons (this %p un %p)\n",this,un);
    stopping = false;
    blocked = false;
    range = 0;
    this->dest = un->getTargetCell();
    this->un = un;
    path = NULL;
    newpos = POS_INVALID;
    istep = 0;
    moved_half = true;
    waiting = false;
    pathinvalid = true;
    speed=p;
}

MoveAnimEvent::~MoveAnimEvent()
{
    if (un->moveanim == this)
        un->moveanim = NULL;
    //logger->debug("MoveAnim dest (this %p un %p dest %u)\n",this,un,dest);
    if( !moved_half && newpos != POS_INVALID) {
        p::uspool->abortMove(un,newpos);
    }
    delete path;
    if (un->walkanim != NULL) {
        un->walkanim->stop();
    }
    if (un->turnanim1 != NULL) {
        un->turnanim1->stop();

    }
    if (un->turnanim2 != NULL) {
        un->turnanim2->stop();
    }
}

void MoveAnimEvent::run()
{
    Sint8 uxoff, uyoff;
    Uint8 oldsubpos;

    waiting = false;
    if( !un->isAlive() ) {
        delete this;
        return;
    }

    if( path == NULL ) {
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range, un);
        if( !path->empty() ) {
            startMoveOne(false);
        } else {
            if (un->attackanim != NULL) {
                un->attackanim->stop();
            }
            delete this;
        }
        return;
    }

    if( blocked ) {
        blocked = false;
        startMoveOne(true);
        return;
    }
    /* if distance left is smaller than xmod we're ready */

    un->xoffset += xmod;
    un->yoffset += ymod;

    if( !moved_half && (abs(un->xoffset) >= 12 || abs(un->yoffset) >= 12) ) {
        oldsubpos = un->subpos;
        un->subpos = p::uspool->postMove(un, newpos);
        un->cellpos = newpos;
        un->xoffset = -un->xoffset;
        un->yoffset = -un->yoffset;
        if( un->type->isInfantry() ) {
            un->infgrp->getSubposOffsets(oldsubpos, un->subpos, &uxoff, &uyoff);
            un->xoffset += uxoff;
            un->yoffset += uyoff;
            xmod = 0;
            ymod = 0;
            if( un->xoffset < 0 )
                xmod = 1;
            else if( un->xoffset > 0 )
                xmod = -1;
            if( un->yoffset < 0 )
                ymod = 1;
            else if( un->yoffset > 0 )
                ymod = -1;
        }
        moved_half = true;
    }

    if( abs(un->xoffset) < abs(xmod) )
        xmod = 0;
    if( abs(un->yoffset) < abs(ymod) )
        ymod = 0;

    if( xmod == 0 && ymod == 0 ) {
        un->xoffset = 0;
        un->yoffset = 0;
        moveDone();
        return;
    }

    p::aequeue->scheduleEvent(this);
}

void MoveAnimEvent::startMoveOne(bool wasblocked)
{
    Uint8 loopend=((UnitType*)un->type)->getAnimInfo().loopend;
    Uint8 face;

    newpos = p::uspool->preMove(un, path->top(), &xmod, &ymod);
    if( newpos == POS_INVALID ) {
        delete path;
        path = NULL;
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range, un);
        pathinvalid = false;
        if( path->empty() ) {
            xmod = 0;
            ymod = 0;
            p::aequeue->scheduleEvent(this);
            return;
        }
        newpos = p::uspool->preMove(un, path->top(), &xmod, &ymod);
        if( newpos == POS_INVALID ) {
            if (wasblocked) {
                xmod = 0;
                ymod = 0;
                if (un->attackanim != NULL) {
                    un->attackanim->stop();
                }
                this->stop();
                p::aequeue->scheduleEvent(this);
                return;
            } else {
                /* TODO: tell the blocking unit to move here */

                blocked = true;

                if (un->walkanim != NULL) {
                    un->walkanim->stop();
                }

                p::aequeue->scheduleEvent(this);
                return;
            }
        }
    }

    Uint8 tmpdelay=(Uint32)speed*100/p::ccmap->getMovePercentage(newpos,un);

    face = ((Uint8)((loopend+1)*(8-path->top())/8))&loopend;
    path->pop();

    moved_half = false;

    if (((UnitType *)un->getType())->isInfantry()) {
        if (un->walkanim != NULL) {
            un->walkanim->changedir(face);
        } else {
            un->walkanim = new WalkAnimEvent(tmpdelay, un, face, 0);
            p::aequeue->scheduleEvent(un->walkanim);
        }
        p::aequeue->scheduleEvent(this);
    } else {
        Uint8 curface = un->getImageNum(0)&loopend;
        Uint8 delta = (abs(curface-face))&loopend;
        if( curface != face ) {
            if( (delta <= (Uint8)((loopend+1)/8)) || (delta >= (Uint8)(loopend*7/8))) {
                un->turn(face,0);
                p::aequeue->scheduleEvent(this);
            } else {
                waiting = true;
                un->turn(face,0);
                un->turnanim1->setSchedule(this);
            }
            if (un->getType()->getNumLayers() > 1) {
                un->turn(face,1);
            }
        } else {
            p::aequeue->scheduleEvent(this);
        }
    }
    setDelay(tmpdelay);
}

void MoveAnimEvent::moveDone()
{
    un->xoffset = 0;
    un->yoffset = 0;

    if (pathinvalid) {
        delete path;
        p::uspool->setCostCalcOwnerAndType(un->owner, 0);
        path = new Path(un->getPos(), dest, range, un);
        pathinvalid = false;
    }
    if( !path->empty() && !stopping ) {
        startMoveOne(false);
    } else {
        if( dest != un->getPos() && !stopping ) {
            delete path;
            p::uspool->setCostCalcOwnerAndType(un->owner, 0);
            path = new Path(un->getPos(), dest, range, un);
            pathinvalid = false;
        }
        if( path->empty() || stopping ) {
            delete this;
        } else {
            startMoveOne(false);
        }
    }
}

void MoveAnimEvent::stop()
{
    stopping = true;
    //stopScheduled();
}

void MoveAnimEvent::update()
{
    //logger->debug("Move updating\n");
    dest = un->getTargetCell();
    pathinvalid = true;
    stopping = false;
    range = 0;
}

WalkAnimEvent::WalkAnimEvent(Uint32 p, Unit *un, Uint8 dir, Uint8 layer) : UnitAnimEvent(p,un)
{
    //fprintf(stderr,"debug: WalkAnim constructor\n");
    this->un = un;
    this->dir = dir;
    this->layer = layer;
    stopping = false;
    istep = 0;
    calcbaseimage();
}

WalkAnimEvent::~WalkAnimEvent()
{
    un->setImageNum((((UnitType*)un->type)->getAnimInfo().loopend+1)*dir/8,layer);
    if (un->walkanim == this)
        un->walkanim = NULL;
}

void WalkAnimEvent::run()
{
    Uint8 layerface;
    if (!stopping) {
        layerface = baseimage + istep;
        // XXX: Assumes 6 frames to loop over
        istep = (istep + 1)%6;
        un->setImageNum(layerface,layer);
        p::aequeue->scheduleEvent(this);
    } else {
        delete this;
        return;
    }
}

void WalkAnimEvent::calcbaseimage()
{
    // XXX: this is really nasty, will be taken care of after the rewrite
    baseimage = 16 + 3*(dir/2);
}

TurnAnimEvent::TurnAnimEvent(Uint32 p, Unit *un, Uint8 dir, Uint8 layer) : UnitAnimEvent(p,un)
{
    Uint8 loopend=((UnitType*)un->type)->getAnimInfo().loopend;            
    //logger->debug("Turn cons (t%p u%p d%i l%i)\n",this,un,dir,layer);
    Uint8 layerface;
    this->un = un;
    this->dir = dir;
    this->layer = layer;
    stopping = false;
    layerface = un->getImageNum(layer)&loopend;
    if(((layerface-dir)&loopend) < ((dir-layerface)&loopend)) {
        turnmod = -(((UnitType *)un->getType())->getTurnMod());
    } else {
        turnmod = (((UnitType *)un->getType())->getTurnMod());
    }
}

TurnAnimEvent::~TurnAnimEvent()
{
    //logger->debug("TurnAnim dest\n");
    if (layer == 0) {
        if (un->turnanim1 == this)
            un->turnanim1 = NULL;
    } else {
        if (un->turnanim2 == this)
            un->turnanim2 = NULL;
    }
}

void TurnAnimEvent::run()
{
    Uint8 loopend=((UnitType*)un->type)->getAnimInfo().loopend;            
    Uint8 layerface;

    //logger->debug("TurnAnim run (s%i)\n",stopping);
    if (stopping) {
        delete this;
        return;
    }

    layerface = un->getImageNum(layer)&loopend;
    if( abs((layerface-dir)&loopend) > abs(turnmod) ) {
        layerface += turnmod;
        layerface &= loopend;
    } else
        layerface = dir;

    un->setImageNum(layerface,layer);
    if( layerface == dir || !un->isAlive()) {
        delete this;
        return;
    }
    p::aequeue->scheduleEvent(this);
}

UAttackAnimEvent::UAttackAnimEvent(Uint32 p, Unit *un) : UnitAnimEvent(p,un)
{
    //logger->debug("UAttack cons\n");
    this->un = un;
    this->target = un->getTarget();
    stopping = false;
    waiting = 0;
    target->referTo();
}

UAttackAnimEvent::~UAttackAnimEvent()
{
    //logger->debug("UAttack dest\n");
    target->unrefer();
    if (un->attackanim == this)
        un->attackanim = NULL;
}

void UAttackAnimEvent::update()
{
    //logger->debug("UAtk updating\n");
    target->unrefer();
    target = un->getTarget();
    target->referTo();
    stopping = false;
}

void UAttackAnimEvent::stop()
{
    if (un == NULL) {
        logger->error("UAttackAnimEvent::stop: un is NULL!?\n");
        abort();
    }
    stopping = true;
}

void UAttackAnimEvent::run()
{
    Uint32 distance;
    Sint32 xtiles, ytiles; // upper right is positive
    Uint16 atkpos;
    float alpha;
    Uint8 facing;
    Uint8 loopend;
    if (un->type->getNumLayers() > 1 ) loopend=((UnitType*)un->type)->getAnimInfo().loopend2;
    else loopend=((UnitType*)un->type)->getAnimInfo().loopend;

    //logger->debug("attack run t%p u%p\n",this,un);
    waiting = 0;
    if( !un->isAlive() || stopping ) {
        delete this;
        return;
    }

    if( !target->isAlive() || stopping) {
        if ( !target->isAlive() ) {
            un->doRandTalk(TB_postkill);
        }
        delete this;
        return;
    }
    atkpos = un->getTargetCell();

    xtiles = -(un->cellpos % p::ccmap->getWidth() - atkpos % p::ccmap->getWidth());
    ytiles = un->cellpos / p::ccmap->getWidth() - atkpos / p::ccmap->getWidth();
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);

    if( distance > un->type->getWeapon()->getRange() /* weapons range */ ) {
        setDelay(0);
        waiting = 3;
        un->move(atkpos,false);
        un->moveanim->setRange(un->type->getWeapon()->getRange());
        un->moveanim->setSchedule(this);
        return;
    }
    //Make sure we're facing the right way
    if( ytiles == 0 ) {
        if( xtiles < 0 ) {
            alpha = M_PI_2;
        } else {
            alpha = 3*M_PI_2;
        }
    } else {
        alpha = atan(-(float)xtiles/(float)ytiles);
        if( ytiles < 0) {
            alpha = M_PI+alpha;
        } else if (xtiles>0 && ytiles > 0) {
            alpha = M_PI*2+alpha;
        }
    }
    facing = ((Uint8)((loopend+1)*alpha/2/M_PI))&loopend;
    if (un->type->isInfantry()) {
        if (facing != (un->getImageNum(0)&loopend)) {
            un->setImageNum((Uint8)((loopend+1)*facing/8),0);
        }
    } else if (un->type->getNumLayers() > 1 ) {
        if (abs((int)(facing - (un->getImageNum(1)&loopend))) > un->type->getROT()) {
            setDelay(0);
            waiting = 2;
            un->turn(facing,1);
            un->turnanim2->setSchedule(this);
            return;
        }
    } else {
        if (abs((int)(facing - un->getImageNum(0)&loopend)) > un->type->getROT()) {
            setDelay(0);
            waiting = 1;
            un->turn(facing,0);
            un->turnanim1->setSchedule(this);
            return;
        }
    }
    // We can shoot

    un->type->getWeapon()->fire(un, target->getBPos(un->getPos()), target->getSubpos());
    // set delay to reloadtime
    setDelay(un->type->getWeapon()->getReloadTime());
    waiting = 4;
    p::aequeue->scheduleEvent(this);
}


HarvestAnimEvent::HarvestAnimEvent(Uint32 p, Unit *un) : UnitAnimEvent(p,un)
{
    this->un = un;
    targetcell = un->getTargetCell();
    maxbails = ((UnitType*)(un->getType()))->getMaxBails();
    reftype = p::uspool->getStructureTypeByName(((UnitType*)un->getType())->getRefineStruct());
    player = p::ppool->getPlayer(un->getOwner());
    stopping = false;
    if (un->getRefineStr()) {
        refstr=un->getRefineStr();
        refpos=un->getRefineStr()->getFreePos((UnitType*)un->getType());
    } else {
        refstr=NULL;
        refpos=POS_INVALID;
    }
    if (!un->bailsFilled()) {
        harv_t=harv_search;
    } else {
        harv_t=harv_return;
    }
}

HarvestAnimEvent::~HarvestAnimEvent()
{
    if (un->harvestanim == this) {
        un->harvestanim = NULL;
    }
}

void HarvestAnimEvent::update()
{
    harv_t=harv_search;
    targetcell = un->getTargetCell();
    stopping = false;
}

void HarvestAnimEvent::stop()
{
    if (un == NULL) {
        logger->error("HarvestAnimEvent::stop: un is NULL!?\n");
        abort();
    }
    stopping = true;
}

inline bool HarvestAnimEvent::harvestOreAt(const Uint16 x, const Uint16 y)
{
    Uint8 ore_type, ore_amount;
    if (p::ccmap->getResource(p::ccmap->translateToPos(x, y), &ore_type, &ore_amount)) {
        targetcell = p::ccmap->translateToPos(x, y);
        return true;
    }
    return false;
}

bool HarvestAnimEvent::findNearbyOre()
{
    Uint16 x, y, i;
    Sint16 j;
    p::ccmap->translateFromPos(targetcell, &x, &y);   

    for (i = 1; i <= dynamic_cast<UnitType*>(un->getType())->getSight(); ++i) {
        for (j = -i; j < i; ++j) {
            if (harvestOreAt(x+j, y+i))   return true; // Lower edge
            if (harvestOreAt(x+j+1, y-i)) return true; // Upper edge
            if (harvestOreAt(x+i, y+j+1)) return true; // Right edge
            if (harvestOreAt(x-i, y+j))   return true; // Left edge
        }
    }
    return false;
}

void HarvestAnimEvent::run()
{
    Uint8 ore_type,ore_amount;
    waiting = 0;
    if( !un->isAlive() || stopping ) {
        delete this;
        return;
    }

    switch (harv_t) {
    case harv_search:
        //we arrived
        if (targetcell==un->getPos()) {
            harv_t=harv_gather;
            waiting=0;
            setDelay(0);
            p::aequeue->scheduleEvent(this);
            return;
        //still moving
        } else {
            setDelay(0);
            waiting = 3;
            un->move(targetcell,false);
            un->moveanim->setRange(0);
            un->moveanim->setSchedule(this);
            return;
        }
        break;
    case harv_gather:
        //resources found
        if (p::ccmap->getResource(targetcell, &ore_type, &ore_amount)) {
            //harvester is not full yet
            if (un->addHarvest(bail_ore)) {
                p::ccmap->setResource(targetcell, ore_type, (ore_amount-1));
                waiting = 6;
                setDelay(50);
                p::aequeue->scheduleEvent(this);
                return;
            //harvester is full
            } else {
                harv_t=harv_return;
                waiting=0;
                setDelay(0);
                p::aequeue->scheduleEvent(this);
                return;
            }
        } else {
            //No ore nearby...
            if (!findNearbyOre()) {
                //Do we have ore aboard?
                if (un->getHarvested()) {
                    harv_t=harv_return;
                    waiting=0;
                    setDelay(0);
                    p::aequeue->scheduleEvent(this);
                    return;
                } else {
                    delete this;
                    return;
                }
            //continue harvesting with the new position given by findNearbyOre
            } else {
                harv_t=harv_search;
                waiting=0;
                setDelay(0);
                p::aequeue->scheduleEvent(this);
                return;
            }
        }
        break;
    case harv_return:
        //Does refstr still exists?
        if (player->isOwnedStructure(un->getRefineStr(),reftype)) {
            //Did it change?
            if (refstr!=un->getRefineStr()) {
                refstr=un->getRefineStr();
                refpos=refstr->getFreePos((UnitType*)un->getType());
            }
            //have we arrived?
            if (refpos==un->getPos()) {
                harv_t=harv_refine;
                waiting=0;
                setDelay(0);
                p::aequeue->scheduleEvent(this);
                return;
            //not arrived, continue moving...
            } else {
                setDelay(0);
                waiting = 3;
                un->move(refpos,false);
                un->moveanim->setRange(0);
                un->moveanim->setSchedule(this);
                return;
            }
        //Is there a new refinery available? (sets it if possible)
        } else if (un->setNewRefineStr()) {
            refstr=un->getRefineStr();
            refpos=refstr->getFreePos((UnitType*)un->getType());
            harv_t=harv_return;
            waiting=0;
            setDelay(0);
            p::aequeue->scheduleEvent(this);
            return;
        //No refinery there anymore, move to the last known position
        } else if (refpos!=POS_INVALID) {
            un->move(refpos,false);
            delete this;
            return;
        //no valid position, we just stay where we are...
        } else {
            delete this;
            return;
        }
        break;
    case harv_refine:
        refstr->runAnim(7,false,un->getHarvested());
        un->clearHarvest();
        harv_t=harv_search;
        waiting=6;
        setDelay(50);
        p::aequeue->scheduleEvent(this);
        return;
    }
}

