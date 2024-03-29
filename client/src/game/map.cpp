/*****************************************************************************
 * map.cpp - map class, the functions to handle a already loaded map goes here
 ****************************************************************************/
#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <cmath>
#include <iostream>
#include "SDL.h"
#include "ccmap.h"
#include "common.h"
#include "config.h"
#include "freecnc.h"
#include "imagecache.h"
#include "imageproc.h"
#include "loadingscreen.h"
#include "logger.h"
#include "netconnection.h"
#include "path.h"
#include "playerpool.h"
#include "shpimage.h"
#include "snprintf.h"
#include "structure.h"
#include "unit.h"
#include "unitandstructurepool.h"

Uint32 CnCMap::getOverlay(Uint32 pos)
{
    if (overlaymatrix[pos] & HAS_OVERLAY)
        return overlays[pos]<<16;
    return 0;
}

Uint32 CnCMap::getTerrain(Uint32 pos, Sint16* xoff, Sint16* yoff)
{
    Uint32 terrain = 0;

    if (overlaymatrix[pos] & HAS_TERRAIN) {
        terrain = terrains[pos].shpnum;
        *xoff = terrains[pos].xoffset;
        *yoff = terrains[pos].yoffset;
    }
    return terrain;
}

/** @brief Sets up things that don't depend on the map being loaded.
 */
CnCMap::CnCMap()
{
    ConfigType config;
    loaded  = false;
    config = getConfig();
    pc::imagepool = new std::vector<SHPImage*>();
    pc::imgcache->setImagePool(pc::imagepool);
    this->maptype = config.gamenum;
    this->gamemode = config.gamemode;
    scrollstep = config.scrollstep;
    scrolltime = config.scrolltime;
    maxscroll  = config.maxscroll;
    /* start at top right corner of map. */
    // the startpos for the map is stored in position 0
    scrollpos.curx = 0;
    scrollpos.cury = 0;
    scrollpos.curxtileoffs = 0;
    scrollpos.curytileoffs = 0;

    scrollvec.x = 0;
    scrollvec.y = 0;
    scrollvec.t = 0;
    toscroll    = false;
    for (Uint8 i=0;i<NUMMARKS;++i) {
        scrollbookmarks[i].x = 0;
        scrollbookmarks[i].y = 0;
        scrollbookmarks[i].xtile = 0;
        scrollbookmarks[i].ytile = 0;
    }
    minimap = NULL;
    oldmmap = NULL;
    loading = false;
    translate_64 = (getConfig().gamenum == GAME_TD);
    for (Uint8 k=0; k<8; k++) {
      for (Uint8 l=0; l<10; l++) movecost[k][l] = config.movecost[k][l];
    }
}



/** Destructor, free up some memory */
CnCMap::~CnCMap()
{
    Uint32 i;

    for( i = 0; i < tileimages.size(); i++ )
        SDL_FreeSurface(tileimages[i]);

    for( i = 0; i < pc::imagepool->size(); i++ )
        delete (*pc::imagepool)[i];
    delete p::uspool;
    delete p::ppool;
    SDL_FreeSurface(minimap);
    for( i = 0; i < numShadowImg; i++ ) {
        SDL_FreeSurface(shadowimages[i]);
    }
    delete pc::imagepool;
}

/** @TODO Map loading goes here.
 */
void CnCMap::loadMap(const char* mapname, LoadingScreen* lscreen) {
    char* buff;
    loading = true;
    missionData.mapname = mapname;
    /* Load the ini part of the map */
    asprintf(&buff,"Reading %s.INI",missionData.mapname);
    lscreen->setCurrentTask(buff);
    free(buff);
    loadIni();

    /* Load the bin part of the map (the tiles) */
    asprintf(&buff,"Loading %s.BIN",missionData.mapname);
    lscreen->setCurrentTask(buff);
    free(buff);
    if (maptype == GAME_TD)
        loadBin();

    //   Path::setMapSize(width, height);
    p::ppool->setAlliances();
    if (gamemode == 2) {
        Sint32 len = strlen("ready\n");
        Sint32 result = pc::conn->send((const Uint8 *)"ready\n", len);
        if (result < len) {
            logger->error("error occurred when sending to server.\n");
            throw LoadMapError();
        }
    }

    loading = false;
    loaded = true;
}

/** sets the scroll to the specified direction.
 * @param direction to scroll in.
 */
Uint8 CnCMap::accScroll(Uint8 direction)
{
    bool validx = false, validy = false;
    if (direction & s_up) {
        if (scrollvec.y >= 0)
            scrollvec.y = -scrollstep;
        else if (scrollvec.y > -maxscroll)
            scrollvec.y -= scrollstep;
        validy = (valscroll & s_up);
        if (!validy) {
            scrollvec.y = 0;
            direction ^= s_up;
        }
    }
    if (direction & s_down) {
        if (scrollvec.y <= 0)
            scrollvec.y = scrollstep;
        else if (scrollvec.y < maxscroll)
            scrollvec.y += scrollstep;
        validy = (valscroll & s_down);
        if (!validy) {
            scrollvec.y = 0;
            direction ^= s_down;
        }
    }
    if (direction & s_left) {
        if (scrollvec.x >= 0)
            scrollvec.x =  -scrollstep;
        else if (scrollvec.x > -maxscroll)
            scrollvec.x -= scrollstep;
        validx = (valscroll & s_left);
        if (!validx) {
            scrollvec.x = 0;
            direction ^= s_left;
        }
    }
    if (direction & s_right) {
        if (scrollvec.x <= 0)
            scrollvec.x = scrollstep;
        else if (scrollvec.x < maxscroll)
            scrollvec.x += scrollstep;
        validx = (valscroll & s_right);
        if (!validx) {
            scrollvec.x = 0;
            direction ^= s_right;
        }
    }
    if (validx || validy) {
        scrollvec.t = 0;
        toscroll = true;
    }
    return direction;
}

Uint8 CnCMap::absScroll(Sint16 dx, Sint16 dy, Uint8 border)
{
    static const double fmax = (double)maxscroll/100.0;
    Uint8 direction = s_none;
    bool validx = false, validy = false;
    if (dx <= -border) {
        validx = (valscroll & s_left);
        if (validx) {
            scrollvec.x = (Sint8)(min(dx,(Sint16)100) * fmax);
            direction |= s_left;
        } else {
            scrollvec.x = 0;
        }
    } else if (dx >= border) {
        validx = (valscroll & s_right);
        if (validx) {
            scrollvec.x = (Sint8)(min(dx,(Sint16)100) * fmax);
            direction |= s_right;
        } else {
            scrollvec.x = 0;
        }
    }
    if (dy <= -border) {
        validy = (valscroll & s_up);
        if (validy) {
            scrollvec.y = (Sint8)(min(dy,(Sint16)100) * fmax);
            direction |= s_up;
        } else {
            scrollvec.y = 0;
        }
    } else if (dy >= border) {
        validy = (valscroll & s_down);
        if (validy) {
            scrollvec.y = (Sint8)(min(dy,(Sint16)100) * fmax);
            direction |= s_down;
        } else {
            scrollvec.y = 0;
        }
    }
    toscroll = (validx || validy);
    return direction;
}


/** scrolls according to the scroll vector.
*/
void CnCMap::doscroll()
{
    Sint32 xtile, ytile;
    if( scrollpos.curx*scrollpos.tilewidth+scrollpos.curxtileoffs <= -scrollvec.x &&
            scrollvec.x < 0) {
        scrollvec.t = 0;
        scrollvec.x = 0;
        scrollpos.curx = 0;
        scrollpos.curxtileoffs = 0;
    }
    if( scrollpos.cury*scrollpos.tilewidth+scrollpos.curytileoffs <= -scrollvec.y &&
            scrollvec.y < 0) {
        scrollvec.t = 0;
        scrollvec.y = 0;
        scrollpos.cury = 0;
        scrollpos.curytileoffs = 0;
    }
    if( scrollpos.curx*scrollpos.tilewidth+scrollpos.curxtileoffs+scrollvec.x >=
            scrollpos.maxx*scrollpos.tilewidth+scrollpos.maxxtileoffs &&
            scrollvec.x > 0) {
        scrollvec.t = 0;
        scrollvec.x = 0;
        scrollpos.curx = scrollpos.maxx;
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    }
    if( scrollpos.cury*scrollpos.tilewidth+scrollpos.curytileoffs+scrollvec.y >=
            scrollpos.maxy*scrollpos.tilewidth+scrollpos.maxytileoffs &&
            scrollvec.y > 0) {
        scrollvec.t = 0;
        scrollvec.y = 0;
        scrollpos.cury = scrollpos.maxy;
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    }

    if ((scrollvec.x == 0) && (scrollvec.y == 0)) {
        toscroll = false;
        setValidScroll();
        return;
    }
    xtile = scrollpos.curxtileoffs+scrollvec.x;
    while( xtile < 0 ) {
        scrollpos.curx--;
        xtile += scrollpos.tilewidth;
    }
    while( xtile >= scrollpos.tilewidth ) {
        scrollpos.curx++;
        xtile -= scrollpos.tilewidth;
    }
    scrollpos.curxtileoffs = xtile;

    ytile = scrollpos.curytileoffs+scrollvec.y;
    while( ytile < 0 ) {
        scrollpos.cury--;
        ytile += scrollpos.tilewidth;
    }
    while( ytile >= scrollpos.tilewidth ) {
        scrollpos.cury++;
        ytile -= scrollpos.tilewidth;
    }
    scrollpos.curytileoffs = ytile;


    ++scrollvec.t;
    /* scrolling continues at current rate for scrolltime
     * passes then decays quickly */
    if (scrollvec.t >= scrolltime) {
        scrollvec.x /=2;
        scrollvec.y /=2;
    }
    setValidScroll();
}



/** sets the maximum value the scroll can take on.
 * @param the maximum x scroll.
 * @param the maximum y scroll.
 */

void CnCMap::setMaxScroll( Uint32 x, Uint32 y, Uint32 xtile, Uint32 ytile, Uint32 tilew )
{
    scrollpos.maxx = 0;
    scrollpos.maxy = 0;
    scrollpos.maxxtileoffs = 0;
    scrollpos.maxytileoffs = 0;
    if( xtile > 0 ) {
        x++;
        xtile = tilew-xtile;
    }
    if( ytile > 0 ) {
        y++;
        ytile = tilew-ytile;
    }

    if( width > x ) {
        scrollpos.maxx = width - x;
        scrollpos.maxxtileoffs = xtile;
    }
    if( height > y ) {
        scrollpos.maxy = height - y;
        scrollpos.maxytileoffs = ytile;
    }

    scrollpos.tilewidth = tilew;

    if( scrollpos.curx > scrollpos.maxx ) {
        scrollpos.curx = scrollpos.maxx;
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    } else if( scrollpos.curx == scrollpos.maxx &&
               scrollpos.curxtileoffs > scrollpos.maxxtileoffs ) {
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    }

    if( scrollpos.cury > scrollpos.maxy ) {
        scrollpos.cury = scrollpos.maxy;
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    } else if( scrollpos.cury == scrollpos.maxy &&
               scrollpos.curytileoffs > scrollpos.maxytileoffs ) {
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    }

    if( scrollpos.curxtileoffs > scrollpos.tilewidth ) {
        scrollpos.curxtileoffs = scrollpos.tilewidth;
    }
    if( scrollpos.curytileoffs > scrollpos.tilewidth ) {
        scrollpos.curytileoffs = scrollpos.tilewidth;
    }

    setValidScroll();
}

void CnCMap::setValidScroll()
{
    Uint8 temp = s_all;
    if( scrollpos.curx == 0 && scrollpos.curxtileoffs == 0 ) {
        temp &= ~s_left;
    }
    if( scrollpos.cury == 0 && scrollpos.curytileoffs == 0 ) {
        temp &= ~s_up;
    }
    if( scrollpos.curx == scrollpos.maxx &&
            scrollpos.curxtileoffs == scrollpos.maxxtileoffs ) {
        temp &= ~s_right;
    }
    if( scrollpos.cury == scrollpos.maxy &&
            scrollpos.curytileoffs == scrollpos.maxytileoffs ) {
        temp &= ~s_down;
    }
    valscroll = temp;
}

bool CnCMap::isBuildableAt(Uint16 pos, UnitOrStructureType* excpUn) const
{
    // Can't build where you haven't explored
    if (!p::ppool->getLPlayer()->getMapVis()[pos]) {
        return false;
    }
    // Can't build on tiberium
    /* TODO
    if (getTiberium(pos) != 0) {
        return false;
    }*/

    if (p::uspool->tileAboutToBeUsed(pos) ||
       (p::uspool->getUnitOrStructureAt(pos,0x80,true))) {
      return false;
    }
    if (excpUn) return (movecost[excpUn->getMoveType()][terraintypes[pos]]>0);
    else return false;
}

Uint8 CnCMap::getMovePercentage(Uint16 pos, Unit* excpUn) const
{
    if (excpUn==NULL) {
        return 0;
    } else {
        return movecost[excpUn->getType()->getMoveType()][terraintypes[pos]];
    }
}

bool CnCMap::isBuildableAt(Uint16 pos, Unit* excpUn) const
{
    UnitOrStructure* uos = 0;
    // Can't build where you haven't explored
    if (!p::ppool->getLPlayer()->getMapVis()[pos]) {
        return false;
    }
    // Can't build on tiberium
    /* TODO
    if (getTiberium(pos) != 0) {
        return false;
    }*/

    if (p::uspool->tileAboutToBeUsed(pos) ||
       (uos=p::uspool->getUnitOrStructureAt(pos,0x80,true))) {
         if (uos!=excpUn) return false;
    }
    if (excpUn) return (movecost[excpUn->getType()->getMoveType()][terraintypes[pos]]>0);
    else return false;
}

Uint16 CnCMap::getCost(Uint16 pos, Unit* excpUn) const
{
    Uint16 cost;

    if( !p::ppool->getLPlayer()->getMapVis()[pos] &&
            (excpUn == 0 || excpUn->getDist(pos)>1 )) {
        return 10;
    }

    /** @TODO: Tile cost should worked out as follows
     * if tmp == 1 then impassible
     * else unitspeed * tmp
     * where "tmp" is the terrain movement penalty.  This is a percentage of how much
     * of the unit's speed is lost when using this terrain type for the
     * unit's type of movement (foot, wheel, track, boat, air.
     * 
     * Unitspeed is used as it might be of use if more heuristics are used
     * when moving groups of units (e.g. either put slower moving units on
     * terrain that lets them move faster to get mixed units to stick
     * together or let faster moving units through a chokepoint first)
     */
    
    if (excpUn==NULL) {
        return 0xffff;
    } else if (movecost[excpUn->getType()->getMoveType()][terraintypes[pos]]>0) {
        cost = excpUn->getType()->getSpeed()*100/getMovePercentage(pos,excpUn) - 2 + p::uspool->getTileCost(pos,excpUn);
    } else {
        return 0xffff;
    }
    return cost;
}

SDL_Surface* CnCMap::getMiniMap(Uint8 pixsize) {
    static ImageProc ip;
    SDL_Rect pos = {0, 0, pixsize, pixsize};
    SDL_Surface *cminitile;
    if (pixsize == 0) {
        // Argh
        logger->error("CnCMap::getMiniMap: pixsize is zero, resetting to one\n");
        pixsize = 1;
    }
    if(minimap != NULL) {
        if (minimap->w == width*pixsize) {
            return minimap;
        } else {
            // Each minimap surface is about 250k, so caching a lot of zooms
            // would be somewhat expensive.  Could make how much memory to set aside
            // for this customizable so people with half a gig of RAM can see some
            // usage :-)
            // For now, just keep the previous one.
            SDL_FreeSurface(oldmmap);
            oldmmap = minimap;
            minimap = NULL;
        }
    }
    if (oldmmap != NULL && (oldmmap->w == pixsize*width)) {
        minimap = oldmmap;
        oldmmap = NULL;
        return minimap;
    }
    minimap = SDL_CreateRGBSurface (SDL_SWSURFACE, width*pixsize,height*pixsize, 16,
                                    0xff, 0xff, 0xff, 0);
    SDL_Surface *maptileOne = getMapTile(0);
    minimap->format->Rmask = maptileOne->format->Rmask;
    minimap->format->Gmask = maptileOne->format->Gmask;
    minimap->format->Bmask = maptileOne->format->Bmask;
    minimap->format->Amask = maptileOne->format->Amask;
    if( maptileOne->format->palette != NULL ) {
        SDL_SetColors(minimap, maptileOne->format->palette->colors, 0,
                      maptileOne->format->palette->ncolors);
    }
    int lineCounter = 0;
    for(Uint32 i = 0;  i < (Uint32) width*height; i++, pos.x += pixsize,
            lineCounter++) {
        if(lineCounter == width) {
            pos.y += pixsize;
            pos.x = 0;
            lineCounter = 0;
        }
        cminitile = ip.minimapScale(getMapTile(i), pixsize);
        SDL_BlitSurface(cminitile, NULL, minimap, &pos);
        SDL_FreeSurface(cminitile);
    }
    /* Now fill in clipping details for renderer and UI.
     * To make things easier, ensure that the geometry is divisable by the
     * specified width and height.
     */
    Uint16 tx = min(miniclip.sidew, (Uint16)minimap->w);
    Uint16 ty = min(tx, (Uint16)minimap->h);
    // w == width in pixels of the minimap
    miniclip.w = pixsize*(int)floor((double)tx/(double)pixsize);
    miniclip.h = pixsize*(int)floor((double)ty/(double)pixsize);
    // x == offset in pixels from the top-left hand corner of the sidebar under
    // the tab.
    miniclip.x = abs(miniclip.sidew-miniclip.w)/2+20;
    miniclip.y = abs(miniclip.sidew-miniclip.h)/2+5;
    // Tilew == number of tiles visible in minimap horizontally
    miniclip.tilew = miniclip.w/pixsize;
    miniclip.tileh = miniclip.h/pixsize;
    // pixsize == number of pixels wide and high a minimap tile is
    miniclip.pixsize = pixsize;
    return minimap;
}

void CnCMap::storeLocation(Uint8 loc)
{
    if (loc >= NUMMARKS) {
        return;
    }
    scrollbookmarks[loc].x = scrollpos.curx;
    scrollbookmarks[loc].y = scrollpos.cury;
    scrollbookmarks[loc].xtile = scrollpos.curxtileoffs;
    scrollbookmarks[loc].ytile = scrollpos.curytileoffs;
}

void CnCMap::restoreLocation(Uint8 loc)
{
    if (loc >= NUMMARKS) {
        return;
    }
    scrollpos.curx = scrollbookmarks[loc].x;
    scrollpos.cury = scrollbookmarks[loc].y;
    scrollpos.curxtileoffs = scrollbookmarks[loc].xtile;
    scrollpos.curytileoffs = scrollbookmarks[loc].ytile;

    if( scrollpos.curxtileoffs >= scrollpos.tilewidth ) {
        scrollpos.curxtileoffs = scrollpos.tilewidth-1;
    }
    if( scrollpos.curytileoffs >= scrollpos.tilewidth ) {
        scrollpos.curytileoffs = scrollpos.tilewidth-1;
    }
    if( scrollpos.curx > scrollpos.maxx ) {
        scrollpos.curx = scrollpos.maxx;
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    } else if( scrollpos.curx == scrollpos.maxx &&
               scrollpos.curxtileoffs > scrollpos.maxxtileoffs ) {
        scrollpos.curxtileoffs = scrollpos.maxxtileoffs;
    }
    if( scrollpos.cury > scrollpos.maxy ) {
        scrollpos.cury = scrollpos.maxy;
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    } else if( scrollpos.cury == scrollpos.maxy &&
               scrollpos.curytileoffs > scrollpos.maxytileoffs ) {
        scrollpos.curytileoffs = scrollpos.maxytileoffs;
    }

    setValidScroll();
}

Uint16 CnCMap::translateToPos(Sint16 x, Sint16 y) const
{
    if (x<0 || y<0 || x>width || y>height) return POS_INVALID;
    else return y*width+x;
}

void CnCMap::translateFromPos(Uint32 pos, Uint16 *x, Uint16 *y) const
{
    *y = pos/width;
    *x = pos-((*y)*width);
}
