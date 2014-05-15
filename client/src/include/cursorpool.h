// mode: -*- C++ -*-
#ifndef CURSORPOOL_H
#define CURSORPOOL_H
#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include "SDL.h"
#include <vector>
#include <map>
#include <string>
#include "inifile.h"

struct cursorinfo
{
    Uint16 anstart,anend;
};

class CursorPool
{
private:
    std::vector<cursorinfo*> cursorpool;
    std::map<std::string, Uint16> name2index;
    INIFile* cursorini;
public:
    CursorPool(const char* ininame);
    ~CursorPool();
    cursorinfo* getCursorByName(const char* name);
};

#endif /* CURSORPOOL_H */
