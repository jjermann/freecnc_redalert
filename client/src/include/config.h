// mode: -*- C++ -*-
#ifndef CONFIG_H
#define CONFIG_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include "SDL_types.h"
#include "SDL_video.h"
#include "SDL_keysym.h"
#include <string>

namespace {
    const Uint8 NUMBINDABLE = 3;
}

enum KEY_TYPE {
    KEY_SIDEBAR = 0, KEY_STOP, KEY_ALLY
};

struct ConfigType
{
    Uint32 videoflags;
    Uint16 width, height, bpp, serverport;
    Uint8 intro, gamenum, gamemode, totalplayers, playernum, buildspeed,
        scrollstep, scrolltime, maxscroll, maxqueue, finaldelay, dispatch_mode,
        sellprop, repairprop, moneyspeed, repairspeed;
    bool nosound, playvqa, allowsandbagging, debug;
    unsigned int movecost[8][10];
    SDL_GrabMode grabmode;
    SDLKey bindablekeys[NUMBINDABLE];
    Uint8 bindablemods[NUMBINDABLE];
    Uint8 buildable_radius;
    std::string mapname, vqamovie, nick, side_colour, mside, serveraddr; // ,disp_logname;
    double buildable_ratio;
};

const ConfigType& getConfig();

#endif /* CONFIG_H */
