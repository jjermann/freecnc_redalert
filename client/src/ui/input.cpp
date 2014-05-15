/*****************************************************************************
 * input.cpp - Class to handle all input from the user
 *
 ****************************************************************************/

#include <string>
#include "ccmap.h"
#include "common.h"
#include "config.h"
#include "dispatcher.h"
#include "input.h"
#include "logger.h"
#include "playerpool.h"
#include "projectileanim.h"
#include "shpimage.h"
#include "structure.h"
#include "soundengine.h"
#include "unit.h"
#include "unitandstructurepool.h"
#include "graphicsengine.h"

/* The defines were introduced in SDL 1.2.5, but should work with all
 * versions since mousewheel support was added.
 */
#ifndef SDL_BUTTON_WHEELUP
#define SDL_BUTTON_WHEELUP 4
#endif
#ifndef SDL_BUTTON_WHEELDOWN
#define SDL_BUTTON_WHEELDOWN 5
#endif

bool Input::drawing = false;
bool Input::minimapEnabled = false;
SDL_Rect Input::markrect;

/** Constructor, sets up the input handeler.
 * @param the sidebar.
 * @param the map.
 * @param the width of the screen.
 * @param the height of the screen.
 */
Input::Input(Uint16 screenwidth, Uint16 screenheight, SDL_Rect *maparea) :
    width(screenwidth), height(screenheight), done(0), donecount(0),
    finaldelay(getConfig().finaldelay), gamemode(p::ccmap->getGameMode()),
    maparea(maparea), tabwidth(pc::sidebar->getTabLocation()->w),
    tilewidth(p::ccmap->getMapTile(0)->w), lplayer(p::ppool->getLPlayer()),
    kbdmod(k_none), lmousedown(m_none), rmousedown(m_none),
    currentaction(a_none), rcd_scrolling(false), sx(0), sy(0)
{
    if (lplayer->isDefeated()) {
        logger->gameMsg("TEMPORARY FEATURE: Free MCV because of no initial");
        logger->gameMsg("units or structures.  Fixing involves adding triggers.");
        logger->gameMsg("Don't right click :-)");
        currentaction = a_place;
        placeposvalid = true;
        temporary_place_unit = true;
        strcpy(bname,"MCV");
        p::ppool->getLPlayer()->setVisBuild(Player::SOB_BUILD,true);
        p::ppool->getLPlayer()->setVisBuild(Player::SOB_SIGHT, true);
        placetype = p::uspool->getStructureTypeByName("sbag");
    }
}

Input::~Input()
{}

/** Method to handle the input events. */
void Input::handle()
{
    SDL_Event event;
    int mx, my;
    Uint8 sdir, radarstat;
    Uint8* keystate;
    static ConfigType config = getConfig();

    while ( SDL_PollEvent(&event) ) {
        switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            // Mousewheel scroll up
            if (event.button.button == SDL_BUTTON_WHEELUP) {
                pc::sidebar->scrollSidebar(true);
                // Mousewheel scroll down
            } else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
                pc::sidebar->scrollSidebar(false);
            } else {
                mx = event.button.x;
                my = event.button.y;
                if( mx >= maparea->x && mx < maparea->x + maparea->w &&
                        my >= maparea->y && my < maparea->y + maparea->h ) {
                    if( !rcd_scrolling && (event.button.button == SDL_BUTTON_LEFT )) {
                        /* start drawing the selection square if in map */
                        lmousedown = m_map;
                        markrect.x = mx;
                        markrect.y = my;
                    } else if( event.button.button == SDL_BUTTON_RIGHT ) {
                        rmousedown = m_map;
                        sx = mx;
                        sy = my;
                        rcd_scrolling = true;
                    }
                }
                if (mx > width-tabwidth && pc::sidebar->getVisible()) {
                    clickSidebar(mx,my, event.button.button);
                }
            }
            break;
        case SDL_MOUSEBUTTONUP:
            //  SDL_GetMouseState(&mx, &my);
            mx = event.button.x;
            my = event.button.y;
            if (event.button.button == SDL_BUTTON_LEFT) {
                pc::sidebar->resetButton();
                if( drawing ) {
                    selectRegion();
                } else if( my >= maparea->y ) {
                    if( my < maparea->y + maparea->h ) { /* map */
                        if( mx >= maparea->x && mx < maparea->x+maparea->w ) {
                            if (lmousedown == m_map) {
                                clickMap(mx, my);
                            }
                        }
                    }
                } else if( my < pc::sidebar->getTabLocation()->h ) { /* clicked a tab */
                    if( mx < tabwidth ) {
                        //printf("Options menu\n");
                    } else if( mx > width-tabwidth ) {
                        pc::sidebar->toggleVisible();
                    }
                }
                /* should also check if minimap is clicked, */

                /* We can't be drawing now */
                drawing = false;
                lmousedown = m_none;
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                // not moved mouse
                if ((mx < (sx+10)) && ((mx+10) > sx) && (my < (sy+10))  && ((my+10) > sy)) {
                    currentaction = a_none;
                    selected.clearSelection();
                } else {}
                rmousedown = m_none;
                rcd_scrolling = false;
            }
            break;
            /* A key has been pressed or released */
        case SDL_KEYDOWN:
            /* If it wasn't a press, ignore this event */
            if( event.key.state != SDL_PRESSED )
                break;
            if (event.key.keysym.sym == config.bindablekeys[KEY_SIDEBAR]) {
                pc::sidebar->toggleVisible();
            } else if (event.key.keysym.sym == config.bindablekeys[KEY_STOP]) {
                selected.stop();
            } else if (event.key.keysym.sym == config.bindablekeys[KEY_ALLY]) {
                if (gamemode != 0) {
                    Player* lplayer, *tplayer;
                    Unit* tmpunit = selected.getRandomUnit();
                    Structure* tmpstruct = selected.getRandomStructure();
                    lplayer = p::ppool->getLPlayer();
                    if (tmpunit != NULL) {
                        tplayer = p::ppool->getPlayer(tmpunit->getOwner());
                    } else if (tmpstruct != NULL) {
                        tplayer = p::ppool->getPlayer(tmpstruct->getOwner());
                    } else {
                        tplayer = NULL;
                    }
                    if (tplayer != NULL) {
                        if (lplayer->allyWithPlayer(tplayer)) {
                            logger->gameMsg("%s allied with player %s",lplayer->getName(),tplayer->getName());
                        } else {
                            if (lplayer->unallyWithPlayer(tplayer)) {
                                logger->gameMsg("%s declared war on player %s",lplayer->getName(),tplayer->getName());
                            } else {
                                // tried to unally with self
                            }
                        }
                    }
                }
            } else {
                Uint8 k_temp = kbdmod;
                /* Otherwise check which key was pressed */
                switch( event.key.keysym.sym ) {
                case SDLK_ESCAPE:
                    /* It was esc so quit */
                    done = 1;
                    break;
                case SDLK_RSHIFT:
                case SDLK_LSHIFT:
                    k_temp |= k_shift;
                    kbdmod = (keymod)k_temp;
                    break;
                case SDLK_RCTRL:
                case SDLK_LCTRL:
                    k_temp |= k_ctrl;
                    kbdmod = (keymod)k_temp;
                    break;
                case SDLK_RALT:
                case SDLK_LALT:
                    k_temp |= k_alt;
                    kbdmod = (keymod)k_temp;
                    break;
                case SDLK_1:
                case SDLK_2:
                case SDLK_3:
                case SDLK_4:
                case SDLK_5:
                case SDLK_6:
                case SDLK_7:
                case SDLK_8:
                case SDLK_9:
                case SDLK_0:
                    if (kbdmod == k_ctrl) {
                        selected.saveSelection(event.key.keysym.sym-48);
                        logger->gameMsg("Saved group %i",event.key.keysym.sym-48);
                    } else if (kbdmod == k_none) {
                        Unit* tmpunit;
                        selected.loadSelection(event.key.keysym.sym-48);
                        tmpunit = selected.getRandomUnit();
                        if (tmpunit != NULL) {
                            logger->gameMsg("Group %i selected",event.key.keysym.sym-48);
                            pc::sfxeng->queueSound(((UnitType *)tmpunit->getType())
                                                  ->getRandTalk(TB_report));
                        }
                    }
                    break;
                case SDLK_F1:
                case SDLK_F2:
                case SDLK_F3:
                case SDLK_F4:
                case SDLK_F5:
                    if (kbdmod == k_ctrl) {
                        p::ccmap->storeLocation(event.key.keysym.sym-282);
                    } else if (kbdmod == k_none) {
                        p::ccmap->restoreLocation(event.key.keysym.sym-282);
                    }
                    break;
                case SDLK_F7:
                    logger->gameMsg("M A R K @ %i",SDL_GetTicks());
                    logger->debug("Mark placed at %i\n",SDL_GetTicks());
                    break;
                case SDLK_F8:
                    p::uspool->showMoves();
                    break;
                case SDLK_v:
                    if (!p::ppool->getLPlayer()->canSeeAll()) {
                        p::ppool->getLPlayer()->setVisBuild(Player::SOB_SIGHT, true);
                        logger->gameMsg("Map revealed");
                    }
                    break;
                case SDLK_m:
                    p::ppool->getLPlayer()->changeMoney(10000);
                    logger->gameMsg("You gained 10000 Credits from a anonymous source");
                    break;
                case SDLK_c:
                    if (!p::ppool->getLPlayer()->canBuildAny()) {
                        p::ppool->getLPlayer()->setVisBuild(Player::SOB_BUILD, true);
                        logger->gameMsg("Build (nearly) anywhere");
                    }
                    break;
                case SDLK_b:
                    if (!p::ppool->getLPlayer()->canBuildAll()) {
//                        p::uspool->preloadUnitAndStructures(98);
//                        p::uspool->generateProductionGroups();
                        p::ppool->getLPlayer()->enableBuildAll();
                        pc::sidebar->updateSidebar();
                        logger->gameMsg("Prerequisites disabled");
                        logger->gameMsg("From now on you're no longer safe from segfaults in the buildqueues :P");
                    }
                    break;
                case SDLK_g:
                    if (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON) {
                        SDL_WM_GrabInput(SDL_GRAB_OFF);
                        logger->gameMsg("Mouse grab disabled");
                    } else {
                        SDL_WM_GrabInput(SDL_GRAB_ON);
                        logger->gameMsg("Mouse grab enabled");
                    }
                    break;
                default:
                    break;
                }
                break;
            }
        case SDL_KEYUP:
            if( event.key.state != SDL_RELEASED )
                break;

            {
            Uint8 k_temp = kbdmod;
            switch( event.key.keysym.sym ) {
            case SDLK_RSHIFT:
            case SDLK_LSHIFT:
                k_temp &= ~k_shift;
                kbdmod = (keymod)k_temp;
                break;
            case SDLK_RCTRL:
            case SDLK_LCTRL:
                k_temp &= ~k_ctrl;
                kbdmod = (keymod)k_temp;
                break;
            case SDLK_RALT:
            case SDLK_LALT:
                k_temp &= ~k_alt;
                kbdmod = (keymod)k_temp;
                break;
            default:
                break;
            }
            }
            break;

            /* We got the quit event so quit */
        case SDL_QUIT:
            done = 1;
            break;
            /* By default we just ignore the event */
        default:
            break;
        }
    }
    sdir = CnCMap::s_none;
    keystate = SDL_GetKeyState(NULL);
    if (keystate[SDLK_LEFT])
        sdir |= CnCMap::s_left;
    else if (keystate[SDLK_RIGHT])
        sdir |= CnCMap::s_right;
    if (keystate[SDLK_UP])
        sdir |= CnCMap::s_up;
    else if (keystate[SDLK_DOWN])
        sdir |= CnCMap::s_down;
    if (sdir != CnCMap::s_none)
        p::ccmap->accScroll(sdir);

    if (keystate[SDLK_PAGEDOWN])
        pc::sidebar->scrollSidebar(false);
    else if (keystate[SDLK_PAGEUP])
        pc::sidebar->scrollSidebar(true);

    if (p::uspool->hasDeleted() ) {
        selected.checkSelection();
    }

    if (p::ppool->pollSidebar() || lplayer->updateBuildQueues()) {
        pc::sidebar->updateSidebar();
    }
    if (p::ppool->pollPowerbar()) {
        pc::sidebar->drawPowerbar();
    }

    radarstat = p::ppool->statRadar();
    switch (radarstat) {
    case 0: // do nothing
        break;
    case 1: // got radar
        pc::sidebar->startRadarAnim(0,&minimapEnabled);
        break;
    case 2: // lost radar
    case 3: // radar powered down
        minimapEnabled = false;
        pc::sidebar->startRadarAnim(1,&minimapEnabled);
        break;
    default:
        logger->error("BUG: unexpected value returned from PlayerPool::statRadar: %i\n",
                      radarstat);
        break;
    }

    updateMousePos();

    if( p::ppool->hasWon() || p::ppool->hasLost() ) {
        ++donecount;
    }
    if (donecount == 1) {
        if (p::ppool->hasWon()) {
            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("misnwon1.aud");
            else pc::sfxeng->queueSound("accom1.aud");
            logger->gameMsg("MISSION ACCOMPLISHED");
        } else {
            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("misnlst1.aud");
            else pc::sfxeng->queueSound("fail1.aud");
            logger->gameMsg("MISSION FAILED");
        }
    }
    if (donecount > finaldelay) {
        done = 1;
    }
}

void Input::updateMousePos()
{
    /* Check the position of the mousepointer and scroll if we are
     * less than 10 pixels from an edge. */
    int mx, my;
    SDL_GetMouseState(&mx, &my);


    if( drawing ) { /* set cursor to the default one when drawing */
        if( mx < maparea->x )
            markrect.w = maparea->x;
        else if( mx >= maparea->x+maparea->w )
            markrect.w = maparea->x+maparea->w-1;
        else
            markrect.w = mx;
        if( my < maparea->y )
            markrect.h = my = maparea->y;
        else if( my >= maparea->y+maparea->h )
            markrect.h = maparea->y+maparea->h-1;
        else
            markrect.h = my;

        pc::cursor->setCursor(CUR_STANDARD, CUR_NOANIM);
    }
    /* Check if we should start drawing */
    else if( lmousedown == m_map && currentaction == a_none) {
        if( my >= maparea->y && my < maparea->y+maparea->h &&
                mx >= maparea->x && mx < maparea->x+maparea->w )
            if(abs(markrect.x - mx) > 2 || abs(markrect.y - my) > 2) {
                drawing = true;
                markrect.w = mx;
                markrect.h = my;
            }
    } else { /* set the correct cursor at the end of this else */
        Uint16 cursornum = 0;
        Uint8 scroll = CnCMap::s_none;

        if (( mx > width - 10 ) || (rcd_scrolling && (mx >= (sx+5))) )
            scroll |= CnCMap::s_right;
        else if (( mx < 10 ) || (rcd_scrolling && ((mx+5) <= sx)) )
            scroll |= CnCMap::s_left;
        if (( my > height - 10 ) || (rcd_scrolling && (my >= (sy+5))) )
            scroll |= CnCMap::s_down;
        else if (( my < 2 ) || (rcd_scrolling && ((my+5) <= sy)) )
            scroll |= CnCMap::s_up;

        if( scroll != CnCMap::s_none ) {
            Uint8 tmp;
            if (rcd_scrolling) {
                tmp = p::ccmap->absScroll((mx-sx),(my-sy), 5);
            } else {
                tmp = p::ccmap->accScroll(scroll);
            }
            if (tmp == CnCMap::s_none) {
                cursornum = Cursor::getNoScrollOffset();
            } else {
                scroll = tmp;
            }
            switch(scroll) {
            case CnCMap::s_up:
                cursornum += CUR_SCROLLUP;
                break;
            case CnCMap::s_upleft:
                cursornum += CUR_SCROLLUL;
                break;
            case CnCMap::s_left:
                cursornum += CUR_SCROLLLEFT;
                break;
            case CnCMap::s_downleft:
                cursornum += CUR_SCROLLDL;
                break;
            case CnCMap::s_down:
                cursornum += CUR_SCROLLDOWN;
                break;
            case CnCMap::s_downright:
                cursornum += CUR_SCROLLDR;
                break;
            case CnCMap::s_right:
                cursornum += CUR_SCROLLRIGHT;
                break;
            case CnCMap::s_upright:
                cursornum += CUR_SCROLLUR;
                break;
            default:
                cursornum = CUR_STANDARD;
            }

            if (currentaction != a_deploysuper) {
                pc::cursor->setCursor(cursornum, CUR_NOANIM);
            }
        } else if (currentaction == a_repair || currentaction == a_sell) {
            checkButton(mx, my);
            return;
        } else if (currentaction == a_place) {
            checkPlace(mx, my);
            return;
        } else if (currentaction == a_deploysuper) {
            pc::cursor->setXY(mx, my);
            return;
        } else {
            setCursorByPos(mx, my);
        }
    }
    pc::cursor->setXY(mx, my);
}

void Input::clickMap(int mx, int my)
{
    Unit *curunit;
    Unit* tmpunit = NULL;
    InfantryGroup* ig;
    Structure *curstructure;
    Uint16 pos;
    Uint8 subpos;
    bool sndplayed = false;
    bool enemy;
    Player* lplayer = p::ppool->getLPlayer();

    clickedTile(mx, my, &pos, &subpos);
    if( pos == POS_INVALID ) return;

    switch (currentaction) {
    case a_repair:
    // @TODO: do it properly...
        curstructure=checkButton(mx, my);
        if (curstructure) {
            if (curstructure->repair(true)==true) {
                pc::sfxeng->queueSound("repair1.aud");
            }
        }
        return;
    case a_sell:
    // @TODO: do it properly...
        curstructure=checkButton(mx, my);
        if (curstructure) {
            curstructure->runAnim(0,true);
            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("strusld1.aud");
              else pc::sfxeng->queueSound("cashturn.aud");
            p::ppool->getLPlayer()->changeMoney(curstructure->getType()->getCost()*getConfig().sellprop/100);
        }
        return;
    case a_place:
        pos = checkPlace(mx, my);
        if( pos != POS_INVALID ) {
            currentaction = a_none;
            if( temporary_place_unit ) {
                if (lplayer->isDefeated()) {
                    lplayer->setVisBuild(Player::SOB_BUILD, false);
                }
                p::dispatcher->unitCreate(bname, pos, 0, lplayer->getPlayerNum());
            } else {
                p::dispatcher->structureCreate(bname, pos, lplayer->getPlayerNum());
                if (getConfig().gamenum == GAME_RA) { pc::sfxeng->queueSound("placbldg1.aud"); }
                else { pc::sfxeng->queueSound("hvydoor1.aud"); }
                if (!p::uspool->getStructureTypeByName(bname)->isWall()) {
                    if (getConfig().gamenum == GAME_RA) { pc::sfxeng->queueSound("build5.aud"); }
		    else { pc::sfxeng->queueSound("constru2.aud"); }
//                    pc::sidebar->updateSidebar();
                }
                lplayer->getBuildQueue(bname,0)->done();
            }
        }
        return;
    case a_deploysuper:
        currentaction = a_none;
        if (strcasecmp("ion",bname) == 0) {
            pc::sfxeng->queueSound("ion1.aud");
        } else if (strcasecmp("atom",bname) == 0) {
            pc::sfxeng->queueSound("alaunch1.aud");
        } else if (strcasecmp("pinf",bname) == 0) {
            pc::sfxeng->queueSound("reinfor1.aud");
        }
        //TODO: atom, chronos, gps, spy, etc animation
        return;
    default:
        break;
    }

    curunit = p::uspool->getUnitAt(pos, subpos);
    curstructure = p::uspool->getStructureAt(pos,selected.getWall());
    ig = p::uspool->getInfantryGroupAt(pos);
    if (ig != NULL && curunit == NULL) {
        curunit = ig->getNearest(subpos);
        if (curunit->getOwner() == p::ppool->getLPlayerNum()) {
            curunit = NULL;
        }
    }
    //unit at pos:
    if( curunit != NULL ) {
        enemy = curunit->getOwner() != p::ppool->getLPlayerNum();
        //enemy:
        if (enemy) {
            if (selected.canAttack() && (
                        !(lplayer->isAllied(
                              p::ppool->getPlayer(curunit->getOwner()))
                         ) || kbdmod == k_ctrl) ) {

		selected.attackUnit(curunit);
                tmpunit = selected.getRandomUnit();
                if (tmpunit != NULL) {
                    pc::sfxeng->queueSound(((UnitType *)tmpunit->getType())->getRandTalk(TB_atkun));
                }
            } else {
                selected.clearSelection();
                selected.addUnit(curunit, enemy);
            }
            return;
        //manual attack:
        } else if( kbdmod == k_ctrl) {
            if( selected.canAttack() ) {
                selected.attackUnit(curunit);
                tmpunit = selected.getRandomUnit();
                if (tmpunit != NULL) {
                    pc::sfxeng->queueSound(((UnitType *)tmpunit->getType())->getRandTalk(TB_atkun));
                }
            }
        //manual add/remove more:
        } else if( kbdmod == k_shift ) {
            if( selected.isEnemy() ) {
                selected.clearSelection();
            }
            if( curunit->isSelected() )
                selected.removeUnit(curunit);
            else {
                selected.addUnit(curunit, false);
                if (!sndplayed) {
                    pc::sfxeng->queueSound(((UnitType *)curunit->getType())->getRandTalk(TB_report));
                    sndplayed = true;
                }
            }
        //select:
        } else if( !curunit->isSelected() ) {
            /*if (!enemy && !selected.empty() && !selected.isEnemy() && selected.canLoad(curunit)) {
              selected.loadUnits(curunit);
              } else {*/
            selected.clearSelection();
            selected.addUnit(curunit, false);
            //}
            if (!sndplayed) {
                pc::sfxeng->queueSound(((UnitType *)curunit->getType())->getRandTalk(TB_report));
                sndplayed = true;
            }
        //unselect:
        } else if ((!selected.isEnemy())&&(curunit->canDeploy())) {
            selected.removeUnit(curunit);
            selected.purgeUnit(curunit);
            p::dispatcher->unitDeploy(curunit);
            pc::sidebar->updateSidebar();
        }
    //struct at pos:
    } else if( curstructure != NULL ) {
        enemy = curstructure->getOwner() != p::ppool->getLPlayerNum();

        //enemy:
        if( enemy ) {
            if (selected.canAttack() && (
                        !(lplayer->isAllied(
                              p::ppool->getPlayer(curstructure->getOwner()))
                         ) ||
                        kbdmod == k_ctrl) ) {
                selected.attackStructure(curstructure);
                tmpunit = selected.getRandomUnit();
                if (tmpunit != NULL) {
                    pc::sfxeng->queueSound(((UnitType *)tmpunit->getType())->getRandTalk(TB_atkst));
                }
            } else if (!curstructure->isWall()) {
                selected.clearSelection();
                selected.addStructure(curstructure, enemy);
            }
            return;
        }
        //manual attack:
        if( kbdmod == k_ctrl ) {
            if( selected.canAttack() ) {
                selected.attackStructure(curstructure);
                tmpunit = selected.getRandomUnit();
                if (tmpunit != NULL) {
                    pc::sfxeng->queueSound(((UnitType *)tmpunit->getType())->getRandTalk(TB_atkst));
                }
                return;
            }
        //manual add/remove more:
        } else if( kbdmod == k_shift ) {
            if( selected.isEnemy() ) {
                selected.clearSelection();
            }
            if( curstructure->isSelected() )
                selected.removeStructure(curstructure);
            else {
                if (!curstructure->isWall())
                    selected.addStructure(curstructure, false);
            }
            return;
        //???
        } else if( kbdmod == k_alt ) {
            // hack
            curstructure->runSecAnim(5);
            return;
        //select
        } else if( !curstructure->isSelected() ) {
            /*if (!enemy && !selected.empty() && !selected.isEnemy() && selected.canLoad(curstructure)) {
              selected.loadUnits(curstructure);
              } else {*/
            selected.clearSelection();
            if (!curstructure->isWall()) {
                selected.addStructure(curstructure, false);
            }
            //}
            return;
        //primary?
        } else {
            if ( (curstructure != lplayer->getPrimary(curstructure->getType())) &&
                    (((StructureType*)curstructure->getType())->primarySettable()) ) {
                lplayer->setPrimary(curstructure);
                pc::sfxeng->queueSound("pribldg1.aud");
            }
            return;
        }
    //no unit/struct at pos:
    //no attack on terrain yet, but at least we hear sthg...
    } else if ((kbdmod == k_ctrl) && selected.canAttack()) {
        // @TODO: attack the position (no target)
        tmpunit = selected.getRandomUnit();
        if (tmpunit != NULL) {
            pc::sfxeng->queueSound(((UnitType *)tmpunit->getType())->getRandTalk(TB_atkun));
        }
    } else if(selected.canHarvest(pos)) {
        selected.harvest(pos);
        return;
    //rest (move)
    } else {
        p::uspool->setCostCalcOwnerAndType(lplayer->getPlayerNum(),0);
        if( selected.canMove()) {
logger->gameMsg("move");
            if (!sndplayed) {
                pc::sfxeng->queueSound(((UnitType *)selected.getRandomUnit()->getType())->getRandTalk(TB_ack));
                sndplayed = true;
            }
            selected.moveUnits(pos);
            new ExplosionAnim(1, pos, p::ccmap->getMoveFlashNum(),
                    p::ccmap->getMoveFlash()->getNumImg(), 0, 0);
        }
    }
}

void Input::clickedTile(int mx, int my, Uint16* pos, Uint8* subpos)
{
    Uint16 xrest, yrest, tx, ty;
    mx -= maparea->x-p::ccmap->getXTileScroll();
    my -= maparea->y-p::ccmap->getYTileScroll();
    tx = mx/tilewidth;
    ty = my/tilewidth;
    if( tx >= p::ccmap->getWidth() || ty >= p::ccmap->getHeight() )
        *pos = POS_INVALID;
    else {
        *pos = (my/tilewidth)*p::ccmap->getWidth()+mx/tilewidth;
        *pos += p::ccmap->getScrollPos();
    }

    xrest = mx%tilewidth;
    yrest = my%tilewidth;
    *subpos = 0;
    if( xrest >= 8 && xrest < 16 && yrest >= 8 && yrest < 16 )
        return;

    *subpos = 1;
    if( yrest >= 12 )
        (*subpos)+=2;
    if( xrest >= 12 )
        (*subpos)++;
    /* assumes the subpositions are:
     * 1 2
     *  0
     * 3 4
     */
}

void Input::setCursorByPos(int mx, int my)
{
    Uint16 pos;
    Uint8 subpos;
    Unit *curunit;
    Structure *curstruct;
    bool enemy;

    /* clicked the map */
    if( my >= maparea->y && my < maparea->y+maparea->h &&
            mx >= maparea->x && mx < maparea->x+maparea->w ) {
        clickedTile(mx, my, &pos, &subpos);
        if( pos != POS_INVALID ) {
            //pos visible:
            if( p::ppool->getLPlayer()->getMapVis()[pos] ) {
                InfantryGroup* ig;
                curunit = p::uspool->getUnitAt(pos, subpos);
                curstruct = p::uspool->getStructureAt(pos,selected.getWall());
                ig = p::uspool->getInfantryGroupAt(pos);
                if (ig != NULL && curunit == NULL) {
                    curunit = ig->getNearest(subpos);
                    if (curunit->getOwner() == p::ppool->getLPlayerNum()) {
                        curunit = NULL;
                    }
                }
            //pos invisible:
            } else {
                curunit = NULL;
                curstruct = NULL;
            }
            //unit at pos:
            if( curunit != NULL) {
                enemy = !(lplayer->isAllied(p::ppool->getPlayer(curunit->getOwner())));
                //unit not selected:
                if( !curunit->isSelected() ) {
                    if( selected.canAttack() && (enemy || (kbdmod == k_ctrl))) {
                        if (selected.targetinRange(pos)) pc::cursor->setCursor("attack");
                        else pc::cursor->setCursor("noattack");
                        return;
                    }
                    /*
                    if (!enemy && !selected.empty() && !selected.isEnemy() && selected.canLoad(curunit)) {
                    pc::cursor->setCursor("enter");
                    return;
                    }*/
                    if( selected.empty() ||
                            (!selected.empty() && !selected.isEnemy() && kbdmod == k_shift) ||
                            (!selected.canAttack() && kbdmod != k_ctrl) ) {
                        pc::cursor->setCursor("select");
                        return;
                    } else {
                        pc::cursor->setCursor("nomove");
                        return;
                    }
                //unit selected
                } else {
                    if (curunit->getOwner() == p::ppool->getLPlayerNum()) {
                        if (((UnitType*)curunit->getType())->canDeploy()) {
                            if (curunit->canDeploy())
                                pc::cursor->setCursor("deploy");
                            else
                                pc::cursor->setCursor("nomove");
                            return;
                        } else {
                            pc::cursor->setCursor("select");
                            return;
                        }
                    }
                }
            //struct at pos:
            } else if( curstruct != NULL ) {
                enemy = !(lplayer->isAllied(p::ppool->getPlayer(curstruct->getOwner())));
                //struct not selected:
                if( !curstruct->isSelected() ) {
                    if( selected.canAttack() && (enemy || (kbdmod == k_ctrl))) {
                        if (selected.targetinRange(pos)) pc::cursor->setCursor("attack");
                        else pc::cursor->setCursor("noattack");
                        return;
                    } else if( !enemy || selected.empty() || selected.isEnemy()) {
                        if (curstruct->isWall()) {
                            pc::cursor->setCursor("nomove");
                            return;
                        } else {
                            /*if (!enemy && !selected.empty() && !selected.isEnemy() && selected.canLoad(curstruct)) {
                              pc::cursor->setCursor("enter");
                              return;
                              }*/
                            if( selected.empty() ||
                                    (!selected.empty() && !selected.isEnemy()) ||
                                    !selected.canAttack() ) {
                                pc::cursor->setCursor("select");
                                return;
                            }
                        }
                    } else if (enemy) {
                        // since we have already considerred the case
                        // when we can attack, we can't attack here.
                        if (kbdmod == k_ctrl) {
                            pc::cursor->setCursor("nomove");
                        } else {
                            pc::cursor->setCursor("select");
                        }
                        return;
                    }
                    return;
                //struct selected:
                } else {
                    if (!enemy) {
                        if ((curstruct != lplayer->getPrimary(curstruct->getType()))&&
                                (((StructureType*)curstruct->getType())->primarySettable()) ) {
                            pc::cursor->setCursor("deploy");
                            return;
                        } else {
                            pc::cursor->setCursor("select");
                            return;
                        }
                    } else {
                        pc::cursor->setCursor("select");
                        return;
                    }
                }
            //no unit/struct at position:
            //set attack cursor (but: we can't attack this (terrain) yet)
            } else if( selected.canAttack() && (kbdmod == k_ctrl) ) {
                if (selected.targetinRange(pos)) pc::cursor->setCursor("attack");
                else pc::cursor->setCursor("noattack");
                return;
            } else if(selected.canHarvest(pos)) {
                pc::cursor->setCursor("attack");
                return;
            //move?
            } else if( selected.canMove() ) {
                p::uspool->setCostCalcOwnerAndType(lplayer->getPlayerNum(),0);
                if( selected.legalMove(pos) || !p::ppool->getLPlayer()->getMapVis()[pos]) {
                    pc::cursor->setCursor("move");
                } else {
                    pc::cursor->setCursor("nomove");
                }
                return;
            }
        }
    }

    pc::cursor->setCursor(CUR_STANDARD, CUR_NOANIM);
}

void Input::selectRegion()
{
    Uint16 startx, starty, stopx, stopy;
    Uint16 scannerx, scannery, curpos, i;
    Unit *un;
    bool playedsnd = false;

    if (kbdmod != k_shift)
        selected.clearSelection();

    startx = (min(markrect.x, (Sint16)markrect.w)-maparea->x+p::ccmap->getXTileScroll())/tilewidth+p::ccmap->getXScroll();
    starty = (min(markrect.y, (Sint16)markrect.h)-maparea->y+p::ccmap->getYTileScroll())/tilewidth+p::ccmap->getYScroll();
    stopx = (abs(markrect.x - markrect.w)+1)/tilewidth+startx;
    stopy = (abs(markrect.y - markrect.h)+1)/tilewidth+starty;
    if( stopx >= p::ccmap->getWidth() )
        stopx = p::ccmap->getWidth() - 1;
    if( stopy >= p::ccmap->getHeight() )
        stopy = p::ccmap->getHeight() - 1;

    for( scannery = starty; scannery <= stopy; scannery++ ) {
        for( scannerx = startx; scannerx <= stopx; scannerx++ ) {
            curpos = scannery*p::ccmap->getWidth()+scannerx;
            for( i = 0; i < 5; i++ ) {
                un = p::uspool->getUnitAt(curpos, i);
                if( un != NULL ) {
                    if( un->getOwner() != p::ppool->getLPlayerNum() ) {
                        continue;
                    }
                    selected.addUnit(un, false);
                    if (!playedsnd) {
                        pc::sfxeng->queueSound(((UnitType *)un->getType())->getRandTalk(TB_report));
                        playedsnd = true;
                    }
                    if( !((UnitType *)un->getType())->isInfantry() )
                        break;
                }
            }
        }
    }

}


void Input::clickSidebar(int mx, int my, Uint8 button)
{
    Uint8 createmode, butclick, rnd;
    BuildQueue *bqueue;
    bool sdep=true;

    mx -= (width-tabwidth);
    my -= pc::sidebar->getTabLocation()->h;
    butclick = pc::sidebar->getButton(mx,my);
    if (butclick != 255) {
        /** @todo find a more elegant way to do this, as scrolling will blank
         *  current place.
         */
        strncpy(bname,"xxxx",4);
        pc::sidebar->clickButton(butclick,bname,&createmode);
        if (strncasecmp("xxxx",bname,4) == 0) {
            currentaction = a_none; 
            return;
        }
        // check super weapons
        if(strncasecmp("atom",bname,4) == 0) {
            currentaction = a_deploysuper;
            pc::cursor->setCursor("atom");

            if (getConfig().gamenum == GAME_RA)
            pc::sfxeng->queueSound("slcttgt1.aud");
            else pc::sfxeng->queueSound("select1.aud");

            pc::sfxeng->queueSound("aready1.aud");
            return;// was scrolling or clicked on super weap
        } else if (strncasecmp("ion",bname,3) == 0) {
        } else if (strncasecmp("bomb",bname,4) == 0) {
        } else if (strncasecmp("infx",bname,4) == 0) {
            pc::sfxeng->queueSound("ironrdy1.aud");
        } else if (strncasecmp("pbmb",bname,4) == 0) {
        } else if (strncasecmp("sonr",bname,4) == 0) {
            pc::sfxeng->queueSound("pulse1.aud");
        } else if (strncasecmp("warp",bname,4) == 0) {
            pc::sfxeng->queueSound("chrordy1.aud");
        } else if (strncasecmp("pinf",bname,4) == 0) {
        } else if (strncasecmp("smig",bname,4) == 0) {
            pc::sfxeng->queueSound("spypln1.aud");
        } else if (strncasecmp("gpss",bname,4) == 0) {
            pc::sfxeng->queueSound("satlnch1.aud");
            return;
        // check Rest
        } else if (strcasecmp("repair",bname) == 0) {
            currentaction=a_repair;
            selected.clearSelection();
            return;
        } else if (strcasecmp("sell",bname) == 0) {
            currentaction=a_sell;
            selected.clearSelection();
            return;
        } else if (strcasecmp("map",bname) == 0) {
            pc::gfxeng->switchMiniZoom();
            return;
        } else sdep=false;

        //HACK to avoid copying stuff 10 times...
        if (sdep) {
            currentaction = a_deploysuper;
            pc::cursor->setCursor("superdeploy");

            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("slcttgt1.aud");
            else pc::sfxeng->queueSound("select1.aud");

            return;// was scrolling or clicked on super weap
        }
 
        /* get the build queue needed to build this item */
        bqueue = lplayer->getBuildQueue(bname, createmode);

        if(bqueue == NULL) return;

        /* get the build queue status */
        Uint8 q_num;
        double q_angle;
        bool q_ready, q_grayed, q_stopped, q_frozen;

        bqueue->getStatus(bname,&q_num,&q_grayed,&q_stopped,&q_frozen,&q_ready,&q_angle);

        switch(bqueue->click(bname, button))
        {
          case BQUEUE_NOMONEY:
	    if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("nofunds1.aud");
            else {
	        rnd = (int) (rand() / 100);
		if (rnd > 100) pc::sfxeng->queueSound("nocash1.aud");			
	        else pc::sfxeng->queueSound("mocash1.aud");
            }
            break;
          case BQUEUE_CONSTRUCT:
            if (!q_grayed || q_stopped) {
              if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("abldgin1.aud");
                else pc::sfxeng->queueSound("bldging1.aud");
            }
            break;
          case BQUEUE_FULL:
            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("bldgprg1.aud");
              else pc::sfxeng->queueSound("bldg1.aud");
            break;
          case BQUEUE_STOPPED:
            pc::sfxeng->queueSound("onhold1.aud");
            break;
          case BQUEUE_FROZEN:
            break;
          case BQUEUE_ABORTED:
            if (getConfig().gamenum == GAME_RA) pc::sfxeng->queueSound("cancld1.aud");
              else pc::sfxeng->queueSound("cancel1.aud");
            break;
          case BQUEUE_READY:
            break;
          case BQUEUE_PLACING:
            placeposvalid = true;
            temporary_place_unit = false;
            currentaction = a_place;
            placetype = p::uspool->getStructureTypeByName(bname);
            break;
        }
        return;
    }
}


Uint16 Input::checkPlace(int mx, int my)
{
    Uint16 x, y;
    Sint16 delta;
    Uint8 placexpos, placeypos;
    Uint8* placemat;
    std::vector<bool>& buildable = lplayer->getMapBuildable();
    Uint16 pos, curpos, placeoff;
    Uint8 subpos;

    // Is the cursor in the map?
    if (my < maparea->y || my >= maparea->y+maparea->h ||
            mx < maparea->x || mx >= maparea->x+maparea->w) {
        pc::cursor->setCursor(CUR_STANDARD, CUR_NOANIM);
        pc::cursor->setXY(mx, my);
        return POS_INVALID;
    }

    clickedTile(mx, my, &pos, &subpos);
    if (pos == POS_INVALID) {
        pc::cursor->setCursor(CUR_STANDARD, CUR_NOANIM);
        pc::cursor->setXY(mx, my);
        placeposvalid = false;
        return pos;
    }

    // Make sure we're placing inside the map (avoid wrapping)
    p::ccmap->translateFromPos(pos, &x, &y);
    delta = (maparea->w / tilewidth)+p::ccmap->getXScroll();
    delta -= (x + (placetype->getXsize()-1));
    if (delta <= 0) {
        x += delta-1;
        /// @BUG: Find a better way than this.
        // While working on this section, I had problems with it working only
        // when the sidebar was/wasn't visible, this seems to work around the
        // problem, but it's horrible, IMO.
        if (maparea->w % tilewidth) {
            ++x;
        }
        pos = p::ccmap->translateToPos(x,y);
    }
    delta = (maparea->h / tilewidth)+p::ccmap->getYScroll();
    delta -= (y + (placetype->getYsize()-1));
    if (delta <= 0) {
        y += delta-1;
        /// @BUG: Find a better way than this.
        // (See above for explanation)
        if (maparea->h % tilewidth) {
            ++y;
        }
       pos = p::ccmap->translateToPos(x,y);
    }

    /* check if pos is valid and set cursor */
    placeposvalid = true;
    /// @TODO Assumes land based buildings for now
    p::uspool->setCostCalcOwnerAndType(lplayer->getPlayerNum(),0);
    placemat = new Uint8[placetype->getXsize()*placetype->getYsize()];
    double blockedcount = 0.0;
    double rangecount = 0.0;
    for (placeypos = 0; placeypos < placetype->getYsize(); placeypos++) {
        for (placexpos = 0; placexpos < placetype->getXsize(); placexpos++) {
            curpos = pos+placeypos*p::ccmap->getWidth()+placexpos;
            placeoff = placeypos*placetype->getXsize()+placexpos;
            if (placetype->isBlocked(placeoff)) {
                ++blockedcount;
                placemat[placeoff] = 1;
                if (!p::ccmap->isBuildableAt(curpos,p::uspool->getTypeByName(bname))) {
                    placeposvalid = false;
                    placemat[placeoff] = 4;
                    continue;
                }
                if (buildable[curpos]) {
                    ++rangecount;
                } else {
                    placemat[placeoff] = 2;
                }
            } else {
                placemat[placeoff] = 0;
            }
        }
    }
    if (rangecount/blockedcount < getConfig().buildable_ratio) {
        placeposvalid = false;
    }
    pc::cursor->setPlaceCursor(placetype->getXsize(), placetype->getYsize(), placemat);

    delete[] placemat;
    drawing = false;

    mx = (pos%p::ccmap->getWidth() - p::ccmap->getXScroll())*tilewidth + maparea->x-p::ccmap->getXTileScroll();
    my = (pos/p::ccmap->getWidth() - p::ccmap->getYScroll())*tilewidth + maparea->y-p::ccmap->getYTileScroll();
    pc::cursor->setXY(mx, my);
    if (!placeposvalid) {
        pos = POS_INVALID;
    }
    return pos;
}

Structure *Input::checkButton(int mx, int my)
{
    Uint16 pos;
    Uint8 subpos;
    Structure *structure = NULL;

    clickedTile(mx, my, &pos, &subpos);
    if (pos != POS_INVALID &&
        my >= maparea->y && my < maparea->y+maparea->h &&
        mx >= maparea->x && mx < maparea->x+maparea->w) {

        structure = p::uspool->getStructureAt(pos);

        if (structure && structure->getOwner() == p::ppool->getLPlayerNum() &&
            ((currentaction == a_repair &&
              structure->getHealth() < structure->getType()->getMaxHealth()) ||
             currentaction == a_sell)) {
            pc::cursor->setCursor(currentaction==a_repair?"repair":"sell");
        }
        else {
            pc::cursor->setCursor(currentaction==a_repair?"norepair":"nosell");
            structure = NULL;
        }
    } else {
        pc::cursor->setCursor(CUR_STANDARD, CUR_NOANIM);
    }
    pc::cursor->setXY(mx, my);
    return structure;
}
