// mode: -*- C++ -*-
#ifndef PATH_H
#define PATH_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <stack>
#include "SDL_types.h"
#include "unit.h"
#include "structure.h"
#include "unitorstructure.h"

/* empty, push, pop, top */
class Path : public std::stack<Uint8>
{
public:
    Path(Uint32 crBeg, Uint32 crEnd, Uint8 max_dist, Unit* un);
    ~Path();
};

#endif
