// mode: -*- C++ -*-
#ifndef FREECNC_H
#define FREECNC_H

#include "SDL.h"
#include "config.h"

class FreeCNC
{
public:
    FreeCNC();
    ~FreeCNC();
    void run();
private:
    ConfigType config;
};

#endif /* FREECNC_H */
