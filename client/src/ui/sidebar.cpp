/***********************************************5~5~******************************
 * sidebar.cpp - sidebar class
 *
 ****************************************************************************/
#include <cstdio>
#include <cstdlib>
#include <math.h>  
#include <stdexcept>
#include "common.h"
#include "config.h"
#include "input.h"
#include "imagecache.h"
#include "logger.h"
#include "shpimage.h"
#include "sidebar.h"
#include "soundengine.h"
#include "playerpool.h"
#include "unitandstructurepool.h"
#include "vfs.h"


using std::runtime_error;

/**
 * @param pl local player object
 * @param height height of screen in pixels
 * @param theatre terrain type of the theatre (e.g. desert, temperate, ...)
 */
Sidebar::Sidebar(Player *pl, Uint16 height, const char *theatre)
    : tab(0), sbar(0), pbar(0), radarlogo(0), visible(true), vischanged(true),
    theatre(theatre), buttondown(0), bd(false), radaranim(NULL),
    radaranimating(false), unitoff(0), structoff(0), player(pl), scaleq(-1)
{
    const char* tmpname;
    Uint8 side;
    SHPImage *tmpshp;

    static const char* radarnames[6];
    static const char* radarbacks[3];

    if (getConfig().gamenum == GAME_RA) {
      radarnames[0]="ussrradr.shp";
      radarnames[1]="natoradr.shp";
      radarnames[2]="natoradr.shp";
      radarbacks[0]="uradrfrm.shp";
      radarbacks[1]="nradrfrm.shp";
      radarbacks[2]="nradrfrm.shp";
    } else {
      radarnames[0]="hradar.nod";
      radarnames[1]="hradar.gdi";
      radarnames[2]="hradar.jp";
      radarnames[3]="radar.nod";
      radarnames[4]="radar.gdi";
      radarnames[5]="radar.jp";
    }


    mapheight=height;
    // If we can't load these files, there's no point in proceeding, thus we let
    // the default handler for runtime_error in main() catch.
    if (getConfig().gamenum == GAME_RA) {
        gamefnt = new Font("12metfnt.fnt");
    } else {
        gamefnt = new Font("scorefnt.fnt");
    }

    if (getConfig().gamenum == GAME_TD) {
        tmpname = VFS_getFirstExisting(2,"htabs.shp","tabs.shp");
        if (tmpname == NULL) {
            throw runtime_error("Unable to find the tab images! (Missing updatec.mix?)");
        } else if (strcasecmp(tmpname,"htabs.shp")==0) {
            isoriginaltype = false;
        } else {
            isoriginaltype = true;
        }
        try {
            tmpshp = new SHPImage(tmpname, scaleq);
        } catch (ImageNotFound&) {
            throw runtime_error("Unable to load the tab images!");
        }
    } else {
        isoriginaltype = false;
        try {
            tmpshp = new SHPImage("tabs.shp",scaleq);
        } catch (ImageNotFound&) {
            throw runtime_error("Unable to load the tab images!");
        }
    }


    if (getConfig().gamenum == GAME_RA) tmpshp->getImage(6, &tab, 0, 0);
    else tmpshp->getImage(0, &tab, 0, 0);
    delete tmpshp;

    tablocation.x = 0;
    tablocation.y = 0;
    tablocation.w = tab->w;
    tablocation.h = tab->h;

    spalnum = pl->getStructpalNum();
    if (!isoriginaltype) {
        spalnum = 0;
    }
    if ((player->getSide()&~PS_MULTI) == PS_BAD) {
        side = isoriginaltype?3:0;
    } else if ((player->getSide()&~PS_MULTI) == PS_GOOD) {
        side = isoriginaltype?4:1;
    } else {
        side = isoriginaltype?5:2;
    }

    radarname = radarnames[side];
    try {
        tmpshp = new SHPImage(radarname, scaleq);
    } catch (ImageNotFound&) {
        logger->error("Hmm.. managed to misdetect sidebar type\n");
        try {
            if (side < 3)
                side += 3;
            else
                side -= 3;
            radarname = radarnames[side];
            tmpshp = new SHPImage(radarname, scaleq);
        } catch (ImageNotFound&) {
            logger->error("Unable to load the radar-image! (Maybe you run c&c gold but have forgoten updatec.mix?)\n");
            throw SidebarError();
        }
    }
    tmpshp->getImage(0, &radarlogo, 0, 0);
    delete tmpshp;

    if (getConfig().gamenum == GAME_RA) {
        radarback = radarbacks[side];
        try {
            tmpshp = new SHPImage(radarback, scaleq);
        } catch (ImageNotFound&) {
            logger->error("Unable to load the radar-image! (Maybe you run c&c gold but have forgoten updatec.mix?)\n");
            throw SidebarError();
        }
        tmpshp->getImage(1, &radarbg, 0, 0);
        delete tmpshp;
    }

    sbarlocation.x = sbarlocation.y = sbarlocation.w = sbarlocation.h = 0;

    // TODO: Should reuse inifiles loaded in unitandstructurepool.cpp
    structureini = new INIFile("structure.ini");
    unitsini = new INIFile("unit.ini");

    sidebar_busy_by_vehicle = 0;
    sidebar_busy_by_soldier = 0;
    sidebar_busy_by_building = 0;

    pc::sidebar = this;
    setupbuttons(height);
}

Sidebar::~Sidebar()
{
    Uint32 i;
    delete unitsini;
    delete structureini;
    SDL_FreeSurface(tab);
    SDL_FreeSurface(radarlogo);
    if (getConfig().gamenum == GAME_RA)
        SDL_FreeSurface(radarbg);
    SDL_FreeSurface(pbar);
    SDL_FreeSurface(sbar);
    delete gamefnt;
    for (i=0; i < buttons.size(); ++i) {
        delete buttons[i];
    }
    for (i=0; i < uniticons.size(); ++i) {
        delete[] uniticons[i];
    }
    for (i=0; i < structicons.size(); ++i) {
        delete[] structicons[i];
    }
}

/**
 * @returns whether the sidebar's visibility has changed.
 */
bool Sidebar::getVisChanged()
{
    if (vischanged) {
        vischanged = false;
        return true;
    }
    return false;
}

void Sidebar::toggleVisible()
{
    visible = !visible;
    vischanged = true;
}


void Sidebar::drawPowerbar()
{
    if (sbar && getConfig().gamenum == GAME_RA) {
      SHPImage *tmpshp;
      SDL_Rect dest;
      SDL_Surface* temp;
    
      if (!pbar) try {
        Uint16 buildpow,powersz;

        temp = SDL_CreateRGBSurface(SDL_SWSURFACE, startoffs.bw, (sbar->h-startoffs.bh), 16, 0, 0, 0, 0);
        pbar = SDL_DisplayFormat(temp);
        SDL_FreeSurface(temp);
        
        tmpshp=new SHPImage("powerbar.shp", scaleq);
        tmpshp->getImage(1, &temp, 0, 0);
        powersz=temp->h;
        tmpshp->getImage(0, &temp, 0, 0);
        
        // powerbar top + middle (HACK!)
        dest.x = 0;
        buildpow = (pbar->h-powersz)/18;
        for (Uint8 j=(buildpow+2);j!=0;j--) {
          dest.y = -18+18*j;
          SDL_BlitSurface( temp, NULL, pbar, &dest );
        }

        // Powerbar bottom
        tmpshp->getImage(1, &temp, 0, 0);
        delete tmpshp;
        dest.x = 0;
        dest.y = pbar->h-temp->h;
        SDL_BlitSurface(temp, NULL, pbar, &dest );
      } catch (ImageNotFound&) {
        logger->error("Something's missing...\n");
      }

      // Draw the Powerbar background (pbar)
      dest.x = 0;
      dest.y = startoffs.bh;
      SDL_BlitSurface(pbar, NULL, sbar, &dest );

      // get the marker...
      tmpshp=new SHPImage("power.shp", scaleq);
      tmpshp->getImage(0, &temp, 0, 0);
      delete tmpshp;

      // draw the power status
      const Uint16 pb_total=sbar->h-startoffs.bh-48-22;
      Uint16 pb_step=pb_total/4;
      Uint32 used=player->getPowerUsed();
      Uint32 power=player->getPower();
      Uint32 tmppower=0;
      Uint32 tmppix[3];

      if (used < power) {
        tmppix[0]=SDL_MapRGB(sbar->format,0, 0xff, 0);
        tmppix[1]=SDL_MapRGB(sbar->format,0, 0xdd, 0);
        tmppix[2]=SDL_MapRGB(sbar->format,0, 0xaa, 0);
      } else if (used == power) {
        tmppix[0]=SDL_MapRGB(sbar->format,0xff, 0xff, 0);
        tmppix[1]=SDL_MapRGB(sbar->format,0xdd, 0xdd, 0);
        tmppix[2]=SDL_MapRGB(sbar->format,0xaa, 0xaa, 0);
      } else {
        tmppix[0]=SDL_MapRGB(sbar->format,0xff, 0x30, 0);
        tmppix[1]=SDL_MapRGB(sbar->format,0xdd, 0x30, 0);
        tmppix[2]=SDL_MapRGB(sbar->format,0xaa, 0x30, 0);
      }

      // generated power:
      dest.h=0;
      while ((tmppower+200)<power) {
        tmppower+=200;
        dest.h+=pb_step;
        pb_step*=3;
        pb_step/=4;
      }
      dest.h+=pb_step*(power-tmppower)/200;
      dest.x = 11;
      dest.y = sbar->h-dest.h-48;
      dest.w = 2;
      SDL_FillRect(sbar, &dest, tmppix[0]);
      dest.x += dest.w;
      dest.w = 1;
      SDL_FillRect(sbar, &dest, tmppix[1]);
      dest.x += dest.w;
      dest.w = 1;
      SDL_FillRect(sbar, &dest, tmppix[2]);

      // used power (marker):
      dest.h = 0;
      tmppower=0;
      pb_step=pb_total/4;

      while ((tmppower+200)<used) {
        tmppower+=200;
        dest.h+=pb_step;
        pb_step*=3;
        pb_step/=4;
      }
      dest.h+=pb_step*(used-tmppower)/200;
      dest.x = 2;
      dest.y=sbar->h-dest.h-48-6;
      SDL_BlitSurface(temp, NULL, sbar, &dest );
      SDL_FreeSurface(temp);
    }
}


SDL_Surface *Sidebar::getSidebarImage(SDL_Rect location)
{
    SDL_Rect dest, src;
    SDL_Surface *temp;
    SHPImage *tmpshp;

    if( location.w == sbarlocation.w && location.h == sbarlocation.h )
        return sbar;

    SDL_FreeSurface(sbar);

    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, location.w, location.h, 16, 0, 0, 0, 0);
    sbar = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    location.x = 0;
    location.y = 0;

    if (isoriginaltype) {
    } else if (getConfig().gamenum == GAME_RA) {
        drawPowerbar();
    } else {
        try {
            tmpshp = new SHPImage("btexture.shp", scaleq);
            tmpshp->getImage(1, &temp, 0, 0);
            delete tmpshp;
            dest.x = 0;
            dest.w = location.w;
            dest.h = temp->h;
            src.x = 0;
            src.y = 0;
            src.h = temp->h;
            src.w = location.w;
            for( dest.y = 0; dest.y < location.h; dest.y += dest.h ) {
                SDL_BlitSurface( temp, &src, sbar, &dest );
            }
            SDL_FreeSurface(temp);
        } catch (ImageNotFound&) {
            logger->error("Hmm.. possible misdetction or corrupt file\n");
            SDL_FillRect(sbar, &location, SDL_MapRGB(sbar->format, 0xa0, 0xa0, 0xa0));
        }
    }
    sbarlocation = location;

    if (getConfig().gamenum == GAME_RA) {
        dest.x = 0;
        dest.y = 0;
        dest.w = radarbg->w;
        dest.h = radarbg->h;
        SDL_BlitSurface( radarbg, NULL, sbar, &dest);
    }
    if (!Input::isMinimapEnabled()) {
        dest.x = 0;
        dest.y = 0;
        dest.w = radarlogo->w;
        dest.h = radarlogo->h;
        SDL_BlitSurface( radarlogo, NULL, sbar, &dest);
    }

    for (Sint8 x=buttons.size()-1;x>=0;--x) {
      buttons[x]->blit(sbar);
    }
    return sbar;
}

void Sidebar::addButton(Uint16 x, Uint16 y, const char* fname, Uint8 f, Uint8 pal)
{
    SidebarButton *tmp = new SidebarButton(x, y, fname, f, theatre, pal);
    buttons.push_back(tmp);
    vischanged = true;
}

void Sidebar::setupbuttons(Uint16 height)
{
    const char* tmpname;
    Uint16 scrollbase;
    Uint8 t;

    startoffs.bh = tab->h+radarlogo->h;
        
    SHPImage *strip,*strip_scroll,*strip_power,*strip_minib;

    tmpname = VFS_getFirstExisting(3,"stripna.shp","hstrip.shp","strip.shp");
    if (tmpname == 0) {
        logger->error("Unable to find strip images for sidebar, exiting\n");
        throw SidebarError();
    }

    try {
        strip = new SHPImage(tmpname, scaleq);
        strip_scroll = new SHPImage("stripup.shp", scaleq);
        if (getConfig().gamenum == GAME_RA)
        {
            strip_power = new SHPImage("powerbar.shp", scaleq);
            strip_minib = new SHPImage("sell.shp", scaleq);
        }
        // @TODO: fix this
        else
        {
            strip_power = new SHPImage("stripup.shp", scaleq);
            strip_minib = new SHPImage("stripup.shp", scaleq);
        }
    } catch (ImageNotFound&) {
        logger->error("Unable to load strip images for sidebar, exiting\n");
        throw SidebarError();
    }

    geom.bh = strip->getHeight();
    geom.bw = strip->getWidth();
    scrollb = strip_scroll->getHeight();
    startoffs.bw = strip_power->getWidth();
    miniboffs.bw = strip_minib->getWidth();
    miniboffs.bh = strip_minib->getHeight();

    delete strip;
    delete strip_scroll;
    delete strip_power;
    delete strip_minib;
  
    if (geom.bh > 100)
        geom.bh = geom.bh>>2;

    if (getConfig().gamenum == GAME_RA) {
      buildbut = ((height-startoffs.bh-scrollb)/geom.bh);
  
      // Repair
      addButton(startoffs.bw,startoffs.bh-miniboffs.bh,"repair.shp",sbo_build,0); // 0
      // Sell
      addButton(startoffs.bw+(miniboffs.bw+10),startoffs.bh-miniboffs.bh,"sell.shp",sbo_build,0); // 1
      // Map
      addButton(startoffs.bw+2*(miniboffs.bw+10),startoffs.bh-miniboffs.bh,"map.shp",sbo_build,0); // 2

      // Scrolling
      scrollbase = startoffs.bh + geom.bh*buildbut;
      addButton(startoffs.bw+geom.bw,scrollbase,"stripup.shp",sbo_scroll|sbo_unit|sbo_up,0); // 3
      addButton(startoffs.bw,scrollbase,"stripup.shp",sbo_scroll|sbo_structure|sbo_up,0); // 4
  
      addButton(startoffs.bw+geom.bw+(geom.bw>>1),scrollbase,"stripdn.shp",sbo_scroll|sbo_unit|sbo_down,0); // 5
      addButton(startoffs.bw+(geom.bw>>1),scrollbase,"stripdn.shp",sbo_scroll|sbo_structure|sbo_down,0); // 6
  
      // The order in which the addButton calls are made MUST be preserved
      // Two loops are made so that all unit buttons and all structure buttons
      // are grouped contiguously (4,5,6,7,...) compared to (4,6,8,10,...)
  
      for (t=0;t<buildbut;++t) {
          addButton(startoffs.bw+geom.bw,startoffs.bh+geom.bh*t,"stripna.shp",sbo_build|sbo_unit,0);
      }
      for (t=0;t<buildbut;++t) {
          addButton(startoffs.bw,startoffs.bh+geom.bh*t,"stripna.shp",sbo_build|sbo_structure,spalnum);
      }
    } else {
      buildbut = ((height-startoffs.bh)/geom.bh)-2;
      startoffs.bh += geom.bh;

      // Repair
      addButton(startoffs.bw,startoffs.bh-miniboffs.bh,"repair.shp",sbo_build,0); // 0
      // Sell
      addButton(startoffs.bw+(miniboffs.bw+10),startoffs.bh-miniboffs.bh,"sell.shp",sbo_build,0); // 1
      // Map
      addButton(startoffs.bw+2*(miniboffs.bw+10),startoffs.bh-miniboffs.bh,"sell.shp",sbo_build,0); // 2
  
      scrollbase = startoffs.bh + geom.bh*buildbut;
      addButton(10+geom.bw,scrollbase,"stripup.shp",sbo_scroll|sbo_unit|sbo_up,0); // 0
      addButton(10,scrollbase,"stripup.shp",sbo_scroll|sbo_structure|sbo_up,0); // 1
  
      addButton(10+geom.bw+(geom.bw>>1),scrollbase,"stripdn.shp",sbo_scroll|sbo_unit|sbo_down,0); // 2
      addButton(10+(geom.bw>>1),scrollbase,"stripdn.shp",sbo_scroll|sbo_structure|sbo_down,0); // 3
  
      // The order in which the addButton calls are made MUST be preserved
      // Two loops are made so that all unit buttons and all structure buttons
      // are grouped contiguously (4,5,6,7,...) compared to (4,6,8,10,...)
  
      for (t=0;t<buildbut;++t) {
          addButton(10+geom.bw,startoffs.bh+geom.bh*t,"strip.shp",sbo_build|sbo_unit,0);
      }
      for (t=0;t<buildbut;++t) {
          addButton(10,startoffs.bh+geom.bh*t,"strip.shp",sbo_build|sbo_structure,spalnum);
      }
    }

    //createdummylists();
    updateAvailableLists();

    updateicons();
    if (uniticons.empty() && structicons.empty()) {
        visible = false;
        vischanged = true;
    }
}

Uint8 Sidebar::getButton(Uint16 x,Uint16 y)
{
    SDL_Rect tmp;

    for (Uint8 i=0;i<buttons.size();++i) {
        tmp = buttons[i]->getRect();
        if (x>=tmp.x && y>=tmp.y && x<(tmp.x+tmp.w) && y<(tmp.y+tmp.h)) {
            return i;
        }
    }
    return 255;
}

/** Event handler for clicking buttons.
 *
 * @bug This is an evil hack that should be replaced by something more flexible.
 */
void Sidebar::clickButton(Uint8 index, char* name, Uint8* createmode)
{
    Uint8 f = buttons[index]->getFunction();
    switch (f&0x3) {
    case 0:
        return;
        break;
    case 1: // build
        if (index<startbuildbutton) {
            downbutton(index);
            switch (index) {
            case 0:
                strcpy(name, "repair");
                break;
            case 1:
                strcpy(name, "sell");
                break;
            case 2:
                strcpy(name, "map");
                break;
            }
        } else {
            build(index - startbuildbutton + 1 - ((f&sbo_unit)?0:buildbut),(f&sbo_unit),name, createmode);
            break;
        }
        break;
    case 2: // scroll
        downbutton(index);
        scrollbuildlist((f&sbo_up), (f&sbo_unit));
        break;
    default:
        logger->error("Sidebar::clickButton. This should not happen (%i)\n",f&0x3);
        break;
    }
}

void Sidebar::build(Uint8 index, Uint8 type, char* name, Uint8* createmode)
{
    Uint8* offptr;
    std::vector<char*>* vecptr;

    if (type) {
        offptr = &unitoff;
        vecptr = &uniticons;
        *createmode = 1;
    } else {
        offptr = &structoff;
        vecptr = &structicons;
        *createmode = 0;
    }

    if ( (unsigned)(*vecptr).size() > ((unsigned)(*offptr)+index - 1) ) {
        strncpy(name,(*vecptr)[(*offptr+index-1)],13);
        name[strlen((*vecptr)[(*offptr+index-1)])-8] = 0x0;
    }

}

void Sidebar::scrollbuildlist(Uint8 dir, Uint8 type)
{
    Uint8* offptr;
    std::vector<char*>* vecptr;

    if (type) {
        offptr = &unitoff;
        vecptr = &uniticons;
    } else {
        offptr = &structoff;
        vecptr = &structicons;
    }

    if (dir) { //up
        if (*offptr>0) {
            --(*offptr);
        }
    } else { // down
        if ((unsigned)(*offptr+buildbut) < (unsigned)vecptr->size()) {
            ++(*offptr);
        }
    }

    updateicons();
}

void Sidebar::updateSidebar()
{
    updateAvailableLists();
    updateicons();
}

/** Rebuild the list of icons that are available.
 *
 * @bug Newer items should be appended, although with some grouping (i.e. keep
 * infantry at top, vehicles, aircraft, boats, then superweapons).
 */
void Sidebar::updateAvailableLists(void)
{
    std::vector<const char*> units_avail, structs_avail;
    char* nametemp;
    bool played_sound;
    Uint32 i;
    played_sound = false;
    units_avail = p::uspool->getBuildableUnits(player);
    structs_avail = p::uspool->getBuildableStructures(player);
    if (units_avail.size() != uniticons.size()) {
        unitoff = 0;
        if (units_avail.size() > uniticons.size()) {
            played_sound = true;
            pc::sfxeng->queueSound("newopt1.aud");
        }
        for (i=0;i<uniticons.size();++i) {
            delete[] uniticons[i];
        }
        uniticons.resize(0);
        for (i=0;i<units_avail.size();++i) {
            nametemp = new char[13];
            memset(nametemp,0x0,13);
            sprintf(nametemp,"%sICON.SHP",units_avail[i]);
            uniticons.push_back(nametemp);
        }
    }
    if (structs_avail.size() != structicons.size()) {
        structoff = 0;
        if ((structs_avail.size() > structicons.size())&&!played_sound) {
            pc::sfxeng->queueSound("newopt1.aud");
        }
        for (i=0;i<structicons.size();++i) {
            delete[] structicons[i];
        }
        structicons.resize(0);
        for (i=0;i<structs_avail.size();++i) {
            nametemp = new char[13];
            memset(nametemp,0x0,13);
            sprintf(nametemp,"%sICON.SHP",structs_avail[i]);
            structicons.push_back(nametemp);
        }
    }
    visible = !(uniticons.empty() && structicons.empty());
    vischanged = true;
}

/** Sets the images of the visible icons, having scrolled.
 */
void Sidebar::updateicons(void)
{
    Uint8 i;
    BuildQueue *queue;
    char placename[14];
    double angle;
    Uint8 num;
    bool grayed=false, stopped=false, ready=false, frozen=false;

    if (getConfig().gamenum == GAME_RA) startbuildbutton=7;
    else startbuildbutton=7;

    for (i=0;i<buildbut;++i) {
        if ((unsigned)(i+unitoff)>=(unsigned)uniticons.size()) {
            if (getConfig().gamenum == GAME_RA) buttons[startbuildbutton+i]->ChangeImage("stripna.shp");
            else buttons[startbuildbutton+i]->ChangeImage("strip.shp");
        } else {
            strncpy(placename, uniticons[i+unitoff], 13);
            placename[strlen(placename) - 8] = '\0';
            queue = player->getBuildQueue(placename, 1);
          
            if(queue) {
              queue->getStatus(placename, &num, &grayed, &stopped, &frozen, &ready, &angle);
              buttons[startbuildbutton+i]->setStatus(num, grayed, stopped, frozen, ready, angle);
            }
             
            buttons[startbuildbutton+i]->ChangeImage(uniticons[i+unitoff]);
        }
    }
    for (i=0;i<buildbut;++i) {
        if ((unsigned)(i+structoff)>=(unsigned)structicons.size()) {
            if (getConfig().gamenum == GAME_RA) buttons[buildbut+startbuildbutton+i]->ChangeImage("stripna.shp");
            else buttons[buildbut+startbuildbutton+i]->ChangeImage("strip.shp");
        } else {
            strncpy(placename, structicons[i+structoff], 13);
            placename[strlen(placename) - 8] = '\0';
            queue = player->getBuildQueue(placename, 0);
          
            if(queue) {
              queue->getStatus(placename, &num, &grayed, &stopped, &frozen, &ready, &angle);
              buttons[buildbut+startbuildbutton+i]->setStatus(num, grayed, stopped, frozen, ready, angle);
            }

            buttons[buildbut+startbuildbutton+i]->ChangeImage(structicons[i+structoff]);
        }
    }

    for (Sint8 x=buttons.size()-1;x>=0;--x) {
       buttons[x]->blit(sbar);
    }
}

void Sidebar::downbutton(Uint8 index)
{
    if (index>=startbuildbutton)
        return; // not a scroll, repair, sell or map button

    buttondown = index;
//    pc::sfxeng->queueSound("button.aud");
 
    bd = true;
    switch (index) {
    case 0:
        buttons[index]->ChangeImage("repair.shp",1);
        break;
    case 1:
        buttons[index]->ChangeImage("sell.shp",1);
        break;  
    case 2:
        buttons[index]->ChangeImage("map.shp",1);
        break;
    case 3:
        buttons[index]->ChangeImage("stripup.shp",1);
        break;
    case 4:
        buttons[index]->ChangeImage("stripup.shp",1);
        break;
    case 5:
        buttons[index]->ChangeImage("stripdn.shp",1);
        break;
    case 6:
        buttons[index]->ChangeImage("stripdn.shp",1);
        break;
    }

    updateicons();
}

void Sidebar::redrawButton(void)
{
    buttons[0]->ChangeImage("repair.shp",0);
    buttons[1]->ChangeImage("sell.shp",0);
    buttons[2]->ChangeImage("map.shp",0);
    if (bd) {
      switch (buttondown) {
        case 0:
            buttons[buttondown]->ChangeImage("repair.shp",1);
            break;
        case 1:
            buttons[buttondown]->ChangeImage("sell.shp",1);
            break;
        case 2:
            buttons[buttondown]->ChangeImage("map.shp",1);
            break;
      }
    }
    updateicons();
}

void Sidebar::resetButton(void)
{
    if (!bd)
        return;
    bd = false;

    switch (buttondown) {
    case 0:
        buttons[buttondown]->ChangeImage("repair.shp",0);
        break;
    case 1:
        buttons[buttondown]->ChangeImage("sell.shp",0);
        break;
    case 2:   
        buttons[buttondown]->ChangeImage("map.shp",0);
        break;
    case 3:   
        buttons[buttondown]->ChangeImage("stripup.shp",0);
        break;
        buttons[buttondown]->ChangeImage("stripdn.shp",0);
        break;
    case 4:   
        buttons[buttondown]->ChangeImage("stripup.shp",0);
        break;
    case 5:   
        buttons[buttondown]->ChangeImage("stripdn.shp",0);
        break;
    case 6:   
        buttons[buttondown]->ChangeImage("stripdn.shp",0);
        break;
    }
    buttondown = 0;
    updateicons();
}

void Sidebar::startRadarAnim(Uint8 mode, bool* minienable)
{
    if (radaranimating == false && radaranim == NULL && sbar != NULL) {
        radaranimating = true;
        radaranim = new RadarAnimEvent(mode,minienable);
    }
}

void Sidebar::scrollSidebar(bool scrollup)
{
    scrollbuildlist(scrollup, 0);
    scrollbuildlist(scrollup, 1);
}

Sidebar::SidebarButton::SidebarButton(Sint16 x, Sint16 y, const char* mixname,
        Uint8 f, const char* theatre, Uint8 pal)
{
    pic = 0;
    picloc.x = x;
    picloc.y = y;
    function = f;
    palnum = pal;
    this->theatre = theatre;
    ChangeImage(mixname);
    grayed = false;
    stopped = false;
    ready = false;  
    num = 0;
    angle = 12.0;
}

void Sidebar::SidebarButton::ChangeImage(const char* fname)
{
    ChangeImage(fname,0);
}

SDL_Surface* Sidebar::SidebarButton::fallback(const char* fname)
{
    SDL_Surface* ret;
    Uint32 width, height;
    Uint32 slen = strlen(fname);
    char* iname = new char[slen-7];
    strncpy(iname,fname,slen-8);
    iname[slen-8] = 0;
    width = pc::sidebar->getGeom().bw;
    height = pc::sidebar->getGeom().bh;
    ret = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCCOLORKEY, width, height, 16, 0, 0, 0, 0);
    SDL_FillRect(ret, NULL, 0);
    pc::sidebar->getFont()->drawString(iname,ret,0,0);
    delete[] iname;
    return ret;
}

void Sidebar::SidebarButton::ChangeImage(const char* fname, Uint8 number)
{
    const char* name;
    char goldname[32];
    Uint32 slen = strlen(fname);
    Uint32 num = 0;

    if (pc::sidebar->isOriginalType() || getConfig().gamenum == GAME_RA) {
        name = fname;
    } else {
        if (slen>8 && strcasecmp("icon.shp", fname+slen-8)==0) {
            strcpy(goldname, fname);
            goldname[slen-6] = 'N';
            goldname[slen-5] = 'H';
            goldname[slen-3] = theatre[0];
            goldname[slen-2] = theatre[1];
            goldname[slen-1] = theatre[2];
        } else {
            sprintf(goldname, "H%s", fname);
        }
        name = goldname;
    }

    try {
        num = pc::imgcache->loadImage(name);
        num += number;
        num |= palnum<<11;
        ImageCacheEntry& ICE = pc::imgcache->getImage(num);
        pic = ICE.image;
    } catch(ImageNotFound&) {
        pic = fallback(fname);
    }

    picloc.w = pic->w;
    picloc.h = pic->h;

    if (picloc.h > 100) {
        picloc.h >>=2;
        pic->h >>= 2;
    }
}

void Sidebar::SidebarButton::blit(SDL_Surface *dest)
{
  SDL_Rect dst;
  SDL_Surface *gr;
  
  if(!dest) return;

  dst = picloc;
  SDL_FillRect(dest, &dst, 0);
  SDL_BlitSurface(pic, NULL, dest, &dst);

  if(grayed) {
  SDL_Surface *dummy;
  SHPImage *tmpshp;
  int scaleq = -1; 
  Uint32 number;   
  Uint16 x, y;     
      try {
          tmpshp = new SHPImage("clock.shp", scaleq);
      } catch (ImageNotFound *e) {
        return;
      }    
    
    number = (Uint32)((angle * tmpshp->getNumImg()) / (M_PI * 2));
    
    if(number >= tmpshp->getNumImg())
      number = tmpshp->getNumImg() - 1;
    
    tmpshp->getImage(number, &gr, &dummy, palnum);
    SDL_FreeSurface(dummy);
    delete tmpshp;

    SDL_LockSurface(gr);
    
    // this should be done using palette tweaking :/
    
   if(gr->format->BytesPerPixel == 4) {
      Uint32 *pixels = (Uint32 *)gr->pixels;
      for(y = 0; y < gr->h; y++)
        for(x = 0; x < gr->w; x++)
          if(pixels[y * (gr->pitch >> 2) + x] == 0x0000a800)
            pixels[y * (gr->pitch >> 2) + x] = 0x00010101;  
    } else if(gr->format->BytesPerPixel == 2) {
      Uint16 *pixels = (Uint16 *)gr->pixels;   
      for(y = 0; y < gr->h; y++)
        for(x = 0; x < gr->w; x++)
          if(pixels[y * (gr->pitch >> 1) + x] == 0x0540)
             pixels[y * (gr->pitch >> 1) + x] = 0x0021; 
//      logger->gameMsg("%08x", pixels[y * (gr->pitch >> 1) + x]);
      
    } 
      
    SDL_UnlockSurface(gr);

    SDL_SetAlpha(gr, SDL_SRCALPHA, 128);
    dst = picloc;
    
    SDL_BlitSurface(gr, NULL, dest, &dst);
    SDL_FreeSurface(gr);
  }
   
  if(ready)
    drawText(dest, "ready");
  else if(stopped)
    drawText(dest, "stopped");
  
  drawNumber(dest);
}
 
void Sidebar::SidebarButton::setStatus(Uint8 num, bool grayed, bool stopped, bool frozen, bool ready, double angle)
{
  this->num = num;
  this->grayed = grayed;
  this->stopped = stopped;
  this->frozen = frozen;
  this->ready = ready;
  this->angle = angle;
}
 
 
 
void Sidebar::SidebarButton::drawText(SDL_Surface *dest, const char *s)
{
  Sint32 x, y;
  
  x = picloc.x + ((picloc.w - pc::sidebar->getFont()->calcStringWidth(s)) >> 1);
//  y = picloc.y + picloc.h - 10;
  y = picloc.y + picloc.h - 30;
  pc::sidebar->getFont()->drawString(s, dest, x, y);
}
 
void Sidebar::SidebarButton::drawNumber(SDL_Surface *dest)
{
  Sint32 x, y;
  char numbuf[10];
  
  if(num == 0) return;
  
  sprintf(numbuf, "%u", num);
  
  x = picloc.x + 3;
  y = picloc.y + 3;
  pc::sidebar->getFont()->drawString(numbuf, dest, x, y);
}
 

Sidebar::SidebarButton::~SidebarButton()
{
}


Sidebar::RadarAnimEvent::RadarAnimEvent(Uint8 mode, bool* minienable) : ActionEvent(1)
{
    this->mode = mode;
    this->minienable = minienable;
    switch (mode) {
    case 0:
        frame = 0;
        framend = 20;
        break;
    case 1:
        //*minienable = false;
        frame = 20;
        framend = 30;
        break;
    default:
        frame = 0;
    }
    p::aequeue->scheduleEvent(this);
}

void Sidebar::RadarAnimEvent::run()
{
    SDL_Rect dest;
    SHPImage* tmpshp;

    if (frame <= framend) {
        tmpshp = new SHPImage(pc::sidebar->radarname, pc::sidebar->scaleq);

        dest.x = 0;
        dest.y = 0;
        dest.w = pc::sidebar->radarlogo->w;
        dest.h = pc::sidebar->radarlogo->h;

        SDL_FillRect(pc::sidebar->radarlogo, &dest, SDL_MapRGB(pc::sidebar->sbar->format, 0x0a, 0x0a, 0x0a));
        SDL_BlitSurface( pc::sidebar->radarlogo, NULL, pc::sidebar->sbar, &dest);

        tmpshp->getImage(frame, &(pc::sidebar->radarlogo), 0, 0);
        SDL_BlitSurface( pc::sidebar->radarlogo, NULL, pc::sidebar->sbar, &dest);

        delete tmpshp;
        pc::sidebar->redrawButton();

        ++frame;
        p::aequeue->scheduleEvent(this);
    } else {
        dest.x = 0;
        dest.y = 0;
        dest.w = pc::sidebar->radarlogo->w;
        dest.h = pc::sidebar->radarlogo->h;

        if (mode == 1) {
            tmpshp = new SHPImage(pc::sidebar->radarname, pc::sidebar->scaleq);
            SDL_FillRect(pc::sidebar->radarlogo, &dest, SDL_MapRGB(pc::sidebar->sbar->format, 0x0a, 0x0a, 0x0a));
            SDL_BlitSurface( pc::sidebar->radarlogo, NULL, pc::sidebar->sbar, &dest);
            tmpshp->getImage(0, &(pc::sidebar->radarlogo), 0, 0);
            SDL_BlitSurface( pc::sidebar->radarlogo, NULL, pc::sidebar->sbar, &dest);
            delete tmpshp;
            pc::sidebar->redrawButton();
            pc::sidebar->radaranim = NULL;
        } else {
            tmpshp = new SHPImage(pc::sidebar->radarback, pc::sidebar->scaleq);
            SDL_FillRect(pc::sidebar->radarbg, &dest, SDL_MapRGB(pc::sidebar->sbar->format, 0x0a, 0x0a, 0x0a));
            SDL_BlitSurface( pc::sidebar->radarbg, NULL, pc::sidebar->sbar, &dest);

            tmpshp->getImage(1, &(pc::sidebar->radarbg), 0, 0);
            SDL_BlitSurface( pc::sidebar->radarbg, NULL, pc::sidebar->sbar, &dest);

            delete tmpshp;
            pc::sidebar->redrawButton();

            *minienable = true;
            pc::sidebar->radaranim = NULL;
        }

        pc::sidebar->radaranimating = false;
        delete this;
        return;
    }
}
