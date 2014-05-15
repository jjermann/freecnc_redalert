// mode: -*- C++ -*-
/*****************************************************************************
 * game.h - header file for game class
 ****************************************************************************/
#ifndef GAME_H
#define GAME_H

#include "config.h"

class Game
{
public:
    Game();
    ~Game();
    void play();
    void dumpstats();
    class GameError {};
private:
    ConfigType config;
    Uint8 gamemode;
};

#endif
