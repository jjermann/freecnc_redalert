// mode: -*- C++ -*-
#ifndef FONT_H
#define FONT_H
#include "SDL.h"
#include "font.h"
#include "shpimage.h"

class Font : SHPBase
{
public:
    Font(const char *fontname);
    ~Font();
    Uint32 getHeight();
    Uint32 calcStringWidth(const char *string);
    void drawString(const char *string, SDL_Surface *dest,
                    Uint32 startx, Uint32 starty);
private:
    SDL_Surface *fontimg;
    SDL_Rect *chrdest;
};
#endif
