#include <cstring>
#include "common.h"
#include "font.h"
#include "message.h"
#include "logger.h"

MessagePool *MessagePool::currentInstance;

MessagePool::MessagePool()
{
    msgfont = new Font("scorefnt.fnt");
    textimg = NULL;
    updated = false;
    currentInstance = this;
}

MessagePool::~MessagePool()
{
    while(!msglist.empty()) {
        delete msglist[0];
        msglist.pop_front();
    }

    delete msgfont;
    SDL_FreeSurface(textimg);
}

SDL_Surface *MessagePool::getMessages()
{
    Uint32 curtick;
    curtick = SDL_GetTicks();
    Uint32 i, msgy;
    Uint32 width, maxwidth;
    SDL_Rect dest;
    while(!msglist.empty() && curtick > msglist[0]->getDeleteTime()) {
        delete msglist[0];
        msglist.pop_front();
        updated = true;
    }
    if( updated ) {
        SDL_FreeSurface(textimg);
        textimg = NULL;
        if(!msglist.empty()) {
            maxwidth = 0;
            for( i = 0; i < msglist.size(); i++ ) {
                width = msgfont->calcStringWidth(msglist[i]->getMessage());
                maxwidth = max(maxwidth, width);
            }
            textimg = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCCOLORKEY, maxwidth,
                                           msglist.size()*(msgfont->getHeight()+1), 16, 0, 0, 0, 0);
            dest.x = 0;
            dest.y = 0;
            dest.w = textimg->w;
            dest.h = textimg->h;
            SDL_FillRect(textimg, &dest, 0);
            SDL_SetColorKey(textimg, SDL_SRCCOLORKEY, 0);
            msgy = 0;
            for( i = 0; i < msglist.size(); i++ ) {
                msgfont->drawString(msglist[i]->getMessage(), textimg, 0, msgy);
                msgy += msgfont->getHeight()+1;
            }
        }

    }
    return textimg;
}

void MessagePool::postMessage(const char *msg)
{
    msglist.push_back(new Message(msg, SDL_GetTicks()+10000));
    updated = true;
}


void MessagePool::clear()
{
    while(!msglist.empty()) {
        delete msglist[0];
        msglist.pop_front();
    }
    updated = true;
}

