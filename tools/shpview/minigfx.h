#ifndef MINIGFX_H
#define MINIGFX_H

#include "SDL.h"
#include "font.h"
#include "shpimage.h"

class MiniGFX
{
private:
    SDL_Surface *screen;
public:
    MiniGFX();
    ~MiniGFX();
    void draw_it(SDL_Surface* img, Font* fnt, Uint32 frame,Uint16 width,Uint16 height,Uint16 max);
    void draw(SHPImage* shp, Font* fnt, Uint32 frame,Uint16 max);
    void draw(TemplateImage* tem, Font* fnt, Uint32 frame,Uint16 max);
    void draw(Dune2Image* d2i, Font* fnt, Uint32 frame,Uint16 max);
};

#endif
