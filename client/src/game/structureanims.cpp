#include <cmath>
#include "config.h"
#include "ccmap.h"
#include "common.h"
#include "playerpool.h"
#include "projectileanim.h"
#include "soundengine.h"
#include "structureanims.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "weaponspool.h"
#include "logger.h"

BuildingAnimEvent::BuildingAnimEvent(Uint32 p, Structure* str, Uint8 mode) : ActionEvent(p)
{
    strct = str;
    strct->referTo();
    this->strct = strct;
    anim_data.done = false;
    anim_data.mode = mode;
    anim_data.frame0 = 0;
    anim_data.frame1 = 0;
    anim_data.damagedelta = 0;
    anim_data.damaged = false;
    toAttack = false;
    if (getaniminfo().animdelay != 0 && mode != 0) // no delay for building anim
        setDelay(getaniminfo().animdelay);
    e = NULL;
    ea = NULL;
    layer2      = ((strct->type->getNumLayers()==2)?true:false);
}

BuildingAnimEvent::~BuildingAnimEvent()
{
    if ((e != NULL)||(ea!=NULL)) {
        if (toAttack) {
            strct->attackAnim = ea;
            p::aequeue->scheduleEvent(ea);
        } else {
            strct->buildAnim = e;
            p::aequeue->scheduleEvent(e);
        }
    }
    strct->unrefer();
}

void BuildingAnimEvent::run()
{
    BuildingAnimEvent* tmp_ev;

    if( !strct->isAlive() ) {
        delete this;
        return;
    }
    anim_func(&anim_data);

    strct->setImageNum(anim_data.frame0,0);
    if (layer2) {
        strct->setImageNum(anim_data.frame1,1);
    }
    if (anim_data.done) {
        if (anim_data.mode != 6 && anim_data.mode != 5) {
            strct->setImageNum(anim_data.damagedelta,0);
            if (layer2) {
                strct->setImageNum(anim_data.damagedelta2,1);
            }
        }
        if (anim_data.mode == 0) {
            //FIX: why do we need this?? (frame0)
            //strct->setImageNum(anim_data.damagedelta + anim_data.frame0,0);
            strct->setImageNum(anim_data.damagedelta,0);
        }
        strct->usemakeimgs = false;
        if ((anim_data.mode == 0) || (anim_data.mode == 7)) {
            switch (getaniminfo().animtype) {
            case 1:
                tmp_ev = new LoopAnimEvent(getaniminfo().animspeed,strct);
                setSchedule(tmp_ev);
                break;
            case 4:
                tmp_ev = new ProcAnimEvent(getaniminfo().animspeed,strct);
                setSchedule(tmp_ev);
                break;
            default:
                strct->animating = false;
                break;
            }
        } else if (e == NULL) {
            strct->animating = false;
        }
        delete this;
        return;
    }
    p::aequeue->scheduleEvent(this);
}

void BuildingAnimEvent::updateDamaged()
{
    bool odam = anim_data.damaged;
    anim_data.damaged = (strct->checkdamage() > 0);
    if (anim_data.damaged) {
        if (getaniminfo().dmgoff != 0 || getaniminfo().dmgoff2 != 0) {
            anim_data.damagedelta = getaniminfo().dmgoff+1;
            anim_data.damagedelta2 = getaniminfo().dmgoff2+1;
        } else {
            anim_data.damagedelta = getaniminfo().loopend+1;
            if (layer2) {
                anim_data.damagedelta2 = getaniminfo().loopend2+1;
            }
        }
        if (!odam && pc::sfxeng != NULL && !p::ccmap->isLoading()) {
            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("crmble2.aud");
            else pc::sfxeng->queueSound("xplobig4.aud");
        }
    } else {
        anim_data.damagedelta = 0;
        anim_data.damagedelta2 = 0;
    }
}

BuildAnimEvent::BuildAnimEvent(Uint32 p, Structure* str, bool sell) : BuildingAnimEvent(p,str,0)
{
    updateDamaged();
    this->sell = sell;
    strct = str;
    framend = getaniminfo().makenum;
    frame = (sell?(framend-1):0);
}

void BuildAnimEvent::anim_func(anim_nfo* data)
{
    if (!sell) {
        if (frame < framend) {
            data->frame0 = frame;
            ++frame;
        } else {
            data->done = true;
            data->frame0 = getType()->getDefaultFace();
        }
    } else {
        if (frame > 0) {
            data->frame0 = frame;
            --frame;
        } else {
            data->done = true;
        }
    }
}
BuildAnimEvent::~BuildAnimEvent()
{
    if (sell) p::uspool->removeStructure(strct);
}

LoopAnimEvent::LoopAnimEvent(Uint32 p, Structure* str) : BuildingAnimEvent(p,str,1)
{
    updateDamaged();
    framend = getaniminfo().loopend;
    frame = 0;
}

void LoopAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    data->frame0 = frame;
    if ((frame-data->damagedelta) < framend) {
        ++frame;
    } else {
        frame = data->damagedelta;
    }
}

ProcAnimEvent::ProcAnimEvent(Uint32 p, Structure* str) : BuildingAnimEvent(p,str,4)
{
    updateDamaged();
    framend = getaniminfo().loopend;
    frame = 0;
}

void ProcAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    data->frame0 = frame;
    ++frame;
    if ((frame-data->damagedelta) > framend) {
        frame = data->damagedelta;
    }
}

void ProcAnimEvent::updateDamaged()
{
    BuildingAnimEvent::updateDamaged();
    if (anim_data.damaged) {
        anim_data.damagedelta = 30; // fixme: remove magic numbers
        if (frame < 30) {
            frame += 30;
        }
    }
}

BTurnAnimEvent::BTurnAnimEvent(Uint32 p, Structure* str, Uint8 face) : BuildingAnimEvent(p,str,6)
{
    Uint8 layerface;
    Uint8 loopend=getaniminfo().loopend;
    updateDamaged();
    targetface = face;
    layerface = (str->getImageNums()[0]&loopend);
    if (layerface == face) {
        delete this;
        return;
    }
    if( ((layerface-face)&loopend) < ((face-layerface)&loopend) ) {
        turnmod = -1;
    } else {
        turnmod = 1;
    }
    this->str = str;
}

void BTurnAnimEvent::anim_func(anim_nfo* data)
{
    Uint8 layerface;
    Uint8 loopend=getaniminfo().loopend;
    layerface = (str->getImageNums()[0]&loopend);
    if( abs((layerface-targetface)&loopend) > abs(turnmod) ) {
        layerface += turnmod;
        layerface &= loopend;
    } else {
        layerface = targetface;
    }
    data->frame0 = layerface+data->damagedelta;
    if( layerface == targetface) {
        data->done = true;
    }
}

DoorAnimEvent::DoorAnimEvent(Uint32 p, Structure* str, bool opening) : BuildingAnimEvent(p,str,5)
{
    updateDamaged();
    if (opening) {
        frame = framestart;
    } else {
        frame = framend;
    }
    this->opening = opening;
}

void DoorAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    if (opening) {
        if (frame < framend) {
            ++frame;
        } else {
            data->done = true;
        }
    } else {
        if (frame > framestart) {
            --frame;
        } else {
            frame = framestart;
            data->done = true;
        }
    }
    data->frame1 = frame;
    data->frame0 = frame0;
}

void DoorAnimEvent::updateDamaged()
{
    BuildingAnimEvent::updateDamaged();
    if (anim_data.damaged) {
        framestart = getaniminfo().loopend2+1;
        framend = framestart+getaniminfo().loopend2;
        if (frame < framestart) {
            frame += framestart;
        }
    } else {
        framestart = 0;
        framend = getaniminfo().loopend2;
    }
    frame0 = anim_data.damagedelta;
}

//TODO: TD and RA somehow differ here, I'll leave it like this for the time beeing
RefineAnimEvent::RefineAnimEvent(Uint32 p, Structure* str, Uint32 newmoney) : BuildingAnimEvent(p,str,7)
{
    updateDamaged();
    money = newmoney;
    this->str = str;
    player=p::ppool->getPlayer(str->getOwner());
    maxstorage = player->getMaxStorage();
    frame = framestart;
}

void RefineAnimEvent::anim_func(anim_nfo* data)
{
    updateDamaged();
    if ((player->getStorage()+money)>=maxstorage) {
      if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("silond1.aud");
      player->changeMoney(maxstorage-(player->getStorage()));
      player->setStorage(maxstorage);
    } else {
      player->changeMoney(money);
      player->setStorage(player->getStorage()+money);
    }
    data->done=true;
#if 0
    if(bails>0) {
        if (frame < framend) {
            ++frame;
        } else {
            frame = framestart;
            --bails;
            player->changeMoney(100);
        }
    } else {
        data->done = true;
    }
    data->frame0 = frame;
#endif
}

void RefineAnimEvent::updateDamaged()
{
    BuildingAnimEvent::updateDamaged();
    if (anim_data.damaged) {
        if (frame < getaniminfo().dmgoff) {
            frame += getaniminfo().dmgoff;
        }
    }
    framestart = getaniminfo().loopend+1+anim_data.damagedelta;
    framend = framestart + 17; // fixme: avoid hardcoded values
}

BAttackAnimEvent::BAttackAnimEvent(Uint32 p, Structure *str) : BuildingAnimEvent(p,str,8)
{
    this->strct = str;
    strct->referTo();
    this->target = str->getTarget();
    target->referTo();
    done = false;
    if (str->type->getWeapon()->getCharges()) {
        str->runAnim(8);
        chargestate = 1;
    } else {
        chargestate = 0;
    }
}

BAttackAnimEvent::~BAttackAnimEvent()
{
    if (strct->animating && chargestate) strct->stopAnim();
    target->unrefer();
    strct->unrefer();
    strct->attackAnim = NULL;
}

void BAttackAnimEvent::update()
{
    target->unrefer();
    target = strct->getTarget();
    target->referTo();
}

void BAttackAnimEvent::run()
{
    Uint32 distance;
    Sint32 xtiles, ytiles;
    Uint16 atkpos,mwid;
    float alpha;
    Uint8 facing;
    mwid = p::ccmap->getWidth();
    Uint8 loopend=getaniminfo().loopend;
    if( !strct->isAlive() || done ) {
        delete this;
        return;
    }

    if( !target->isAlive() || done) {
        if (!target->isAlive()) {}
        delete this;
        return;
    }
    atkpos = target->getPos();

    xtiles = strct->cellpos % mwid - atkpos % mwid;
    ytiles = strct->cellpos / mwid - atkpos / mwid;
    distance = abs(xtiles)>abs(ytiles)?abs(xtiles):abs(ytiles);

    if( distance > strct->type->getWeapon()->getRange() /* weapons range */ ) {
        /* Since buildings can not move, give up for now.
         * Alternatively, we could just wait to see if the target ever 
         * enters range (highly unlikely when the target is a structure)
         */
        delete this;
        return;
    }

    if (chargestate==1 && !strct->animating) {
        chargestate=2; //fire
    } else if (chargestate==2 && !strct->animating) {
        strct->runAnim(8);
        chargestate=1;
    }
    if(strct->animating && chargestate==1) {
        setDelay(1);
        p::aequeue->scheduleEvent(this);
        return;
    }

    //Make sure we're facing the right way
    if( xtiles == 0 ) {
        if( ytiles < 0 ) {
            alpha = -M_PI_2;
        } else {
            alpha = M_PI_2;
        }
    } else {
        alpha = atan((float)ytiles/(float)xtiles);
        if( xtiles < 0 ) {
            alpha = M_PI+alpha;
        }
    }
    facing = (40-(Sint8)(alpha*16/M_PI))&0x1f;

    if ((strct->type->hasTurret())&&((strct->getImageNums()[0]&loopend)!=facing)) { // turn to face target first
        setDelay(0);
        strct->buildAnim = new BTurnAnimEvent(strct->type->getTurnspeed(), strct, facing);
        strct->buildAnim->setSchedule(this,true);
        p::aequeue->scheduleEvent(strct->buildAnim);
        return;
    }

    // We can shoot
    strct->type->getWeapon()->fire(strct, target->getBPos(strct->getPos()), target->getSubpos());
    setDelay(strct->type->getWeapon()->getReloadTime());
    p::aequeue->scheduleEvent(this);
}

void BAttackAnimEvent::stop()
{
    done = true;
}

BExplodeAnimEvent::BExplodeAnimEvent(Uint32 p, Structure* str) : BuildingAnimEvent(p,str,9)
{
    this->strct = str;
    if (getType()->isWall()) {
        lastframe = strct->getImageNums()[0];
    } else {
        lastframe = getType()->getSHPTNum()[0]-1;
    }
    counter = 0;
    setDelay(1);
}

BExplodeAnimEvent::~BExplodeAnimEvent()
{
    // spawn survivors and other goodies
    p::uspool->removeStructure(strct);
}

void BExplodeAnimEvent::run()
{
    if ((counter == 0) && !(getType()->isWall()) && (pc::sfxeng != NULL) && !p::ccmap->isLoading()) {
    if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("kaboom22.aud");
    else pc::sfxeng->queueSound("crumble.aud");
        // add code to draw flames
    }
    BuildingAnimEvent::run();
}

void BExplodeAnimEvent::anim_func(anim_nfo* data)
{
    ++counter;
    data->frame0 = 0; //lastframe;
    if (counter < 10) {
        data->done = false;
    } else {
        data->done = true;
    }
}

ChargeAnimEvent::ChargeAnimEvent(Uint32 p, Structure* str) : BuildingAnimEvent(p,str,0)
{
    if (((StructureType*)str->getType())->getWeapon()) {
        pc::sfxeng->queueSound(((StructureType*)str->getType())->getWeapon()->getCharges());
    }
    framend = getaniminfo().loopend;
    frame = 0;
}

void ChargeAnimEvent::anim_func(anim_nfo* data)
{
    if (frame < framend) {
        data->frame0 = frame;
        ++frame;
    } else {
        data->done = true;
        data->frame0 = getType()->getDefaultFace();
    }
}

ChargeAnimEvent::~ChargeAnimEvent()
{
}

RepairAnimEvent::RepairAnimEvent(Structure* str) : ActionEvent(50)
{
    this->strct=str;
    //TODO: what if the building was taken while repairing?
    player=p::ppool->getPlayer(str->getOwner());
    ticks = SDL_GetTicks();
    maxhealth=str->getType()->getMaxHealth();
    repairspeed = getConfig().repairspeed;
    repairhealth = getConfig().repairprop*maxhealth/str->getType()->getCost();
    p::aequeue->scheduleEvent(this);
}

void RepairAnimEvent::run()
{
    if (strct==NULL || (strct && strct->repairAnim==NULL)) {
        delete this;
        return;
    } else if  (strct->getHealth()==maxhealth) {
        strct->repairAnim=NULL;
        delete this;
        return;
    }

    Uint32 newticks=SDL_GetTicks();
    Uint32 tmpmoney=(newticks-ticks)/repairspeed;
    Uint32 tmphealth=tmpmoney*repairhealth/100;
    ticks=newticks;

    if(tmphealth >= (Uint32)(maxhealth-strct->getHealth())) {
        tmphealth = (maxhealth-strct->getHealth());
        tmpmoney = tmphealth*100/repairhealth;
    }

    if (player->getMoney()<tmpmoney) {
        strct->applyDamage(-player->getMoney()*repairhealth/100);
        player->setMoney(0);
    } else {
        strct->applyDamage(-tmphealth);
        player->setMoney(player->getMoney()-tmpmoney);
    }

    if (strct->getHealth()==maxhealth) {
        strct->repairAnim=NULL;
        delete this;
        return;
    } else {
        p::aequeue->scheduleEvent(this);
    }
}

RepairAnimEvent::~RepairAnimEvent()
{
}
