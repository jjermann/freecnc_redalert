// mode: -*- C++ -*-
#ifndef COMMON_H
#define COMMON_H

#include "SDL_types.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

/* This file contains data types and constants that are used by multiple
 * classes, in possibly different files
 */

class ActionEventQueue;
class CnCMap;
class UnitAndStructurePool;
class PlayerPool;
class WeaponsPool;
class Dispatcher;

// Currently client side only, but we'll be using it in the server as well.
class NetConnection;

// Forward dcls for client only things.
class SoundEngine;
class GraphicsEngine;
class MessagePool;
class SHPImage;
class Sidebar;
class Cursor;
class Input;
class ImageCache;

// Forward dcls for server only things.
namespace AI {
    class AIPlugMan;
}
// Pointers to the instance of commonly used objects.  Try to maintain the
// client/server seperation, so when it comes to the real split things will go
// easier.

// Used by both client and server
namespace p {
    extern ActionEventQueue* aequeue;
    extern CnCMap* ccmap;
    extern UnitAndStructurePool* uspool;
    extern PlayerPool* ppool;
    extern WeaponsPool* weappool;
    extern Dispatcher* dispatcher;
}

// Client only
namespace pc {
    extern SoundEngine* sfxeng;
    extern GraphicsEngine* gfxeng;
    extern NetConnection* conn;
    extern MessagePool* msg;
    extern std::vector<SHPImage *>* imagepool;
    extern ImageCache* imgcache;
    extern Sidebar* sidebar;
    extern Cursor* cursor;
    extern Input* input;
}

// Server only
namespace ps {
    extern AI::AIPlugMan* aiplugman;
}

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)

// Workaround for MSVC 6's dodgy headers
// http://x42.deja.com/getdoc.xp?AN=520752890
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

template<class T> inline const T& max(const T& a, const T& b) {
    return (a>b)?a:b;
}


template<class T> inline const T& min(const T& a, const T& b) {
    return (a<b)?a:b;
}

#else
using std::max;
using std::min;
#endif

// @TODO These don't need to be #define'd
#define VERSION "0.2.1-CVS"
#define MAXPLAYERS 6
#define GAME_TD 1
#define GAME_RA 2
#define POS_INVALID 0xffff

extern int mapscaleq;

/* From math.h when __USE_BSD || defined __USE_XOPEN is defined
 * Not sure what is needed for MSVC though, so I'm sticking these here
 * for now
 */
#ifdef M_PI
#undef M_PI
#endif
#define M_PI   3.14159265358979323846
#ifdef M_PI_2
#undef M_PI_2
#endif
#define M_PI_2 1.57079632679489661923

/// @TODO: This shouldn't be here
struct animinfo_t {
    Uint32 animdelay;
    Uint8 loopend, loopend2, animspeed, animtype, sectype;
    Uint8 dmgoff, dmgoff2;
    Uint16 makenum;
};

/// @TODO: This shouldn't be here
struct powerinfo_t {
    Uint16 power;
    Uint16 drain;
    bool powered;
};

/// @TODO: This shouldn't be here
enum PSIDE {
    PS_UNDEFINED = 0, PS_GOOD = 0x1, PS_BAD = 0x2,
    PS_NEUTRAL = 0x4, PS_SPECIAL = 0x8, PS_MULTI = 0x10
};

/// @TODO: This shouldn't be here
enum armour_t {
    AC_none = 0, AC_wood = 1, AC_light = 2, AC_heavy = 3, AC_concrete = 4
};

/// @TODO: This shouldn't be here
enum move_t {
    MT_none = 0, MT_build = 1, MT_build_water = 2, MT_foot = 3, MT_track = 4, MT_wheel = 5, MT_float = 6, MT_air = 7
};

/// @TODO: This shouldn't be here
enum LOADSTATE {
    PASSENGER_NONE = 0, PASSENGER_LOAD = 1, PASSENGER_UNLOAD = 2
};

/// @TODO: This shouldn't be here
enum TalkbackType {
    TB_report, TB_ack, TB_atkun, TB_atkst, TB_die, TB_postkill, TB_invalid
};

#ifndef _WITHOUT_STRCASECMP
int strncasecmp(const char*, const char*, size_t) throw();
int strcasecmp(const char*, const char*) throw();
#endif

/// Same as strdup but uses C++ style allocation
inline char* cppstrdup(const char* s) {
    char* r = new char[strlen(s)+1];
    return strcpy(r,s);
}

inline bool isRelativePath(const char *p) {
#ifdef _WIN32
    return ((strlen(p) == 0) || p[1] != ':') && p[0] != '\\' && p[0] != '/';
#else
    return p[0] != '/';
#endif
}

std::vector<char*> splitList(char* line, char delim);

char* stripNumbers(const char* src);

char* determineBinaryLocation(const char *launchcmd);

const char* getBinaryLocation();

/// From Boost.  Renamed to avoid any possible clashes.
class fcnc_bad_lexical_cast {};

/// From Boost.  Renamed to avoid any possible clashes.
template<typename Target, typename Source>
Target fcnc_lexical_cast(Source arg)
{
  std::stringstream interpreter;
  Target result;

  if(!(interpreter << arg) ||
     !(interpreter >> result) ||
     !(interpreter >> std::ws).eof())
    throw fcnc_bad_lexical_cast();

  return result;
}

#endif /* COMMON_H */
