#include <cstdlib>
#include "common.h"
#include "minigfx.h"
#include "shpimage.h"
#include "font.h"
#include "logger.h"

MiniGFX::MiniGFX()
{
    SDL_WM_SetCaption("SHP Viewer", NULL);
    screen = SDL_SetVideoMode(320, 240, 8, SDL_HWSURFACE|SDL_DOUBLEBUF/*|SDL_FULLSCREEN*/);
    if (screen == NULL) {
        logger->error("Unable to init videomode: %s\n", SDL_GetError());
        throw 0;
    }
}

MiniGFX::~MiniGFX()
{}

void MiniGFX::draw(SHPImage* shp, Font* fnt, Uint32 frame,Uint16 max)
{
    SDL_Surface* image, *dummy;
    shp->getImage(frame,&image,&dummy,0);
    delete dummy;
    draw_it(image,fnt,frame,shp->getWidth(),shp->getHeight(),max);
}

void MiniGFX::draw(TemplateImage* tem, Font* fnt, Uint32 frame,Uint16 max)
{
    SDL_Surface* image;
    image = tem->getImage(frame);
    draw_it(image,fnt,frame,image->w,image->h,max);
}

void MiniGFX::draw(Dune2Image* d2i, Font* fnt, Uint32 frame,Uint16 max)
{
    SDL_Surface* image;
    image = d2i->getImage(frame);
    draw_it(image,fnt,frame,image->w,image->h,max);
}

void MiniGFX::draw_it(SDL_Surface* img, Font *fnt, Uint32 frame, Uint16 width, Uint16 height, Uint16 max)
{
    SDL_Rect dest;

    static Uint32 blackpix = SDL_MapRGB(screen->format,0x40, 0x40, 0x40);

    char textmsg[1024];

    dest.x = 0;
    dest.y = 0;

    dest.w = 320;
    dest.h = 240;
    SDL_FillRect(screen, &dest, blackpix);

    dest.x += 25;

    dest.w = width;
    dest.h = height;
    dest.x = std::max(160-dest.w,0);
    dest.y = std::max(120-dest.h,0);

    SDL_BlitSurface(img,NULL,screen,&dest);

    sprintf(textmsg, "Frame %d / %d", frame,max);
    fnt->drawString(textmsg, screen, 0, 0);


    SDL_Flip(screen);
    SDL_FreeSurface(img);
}
