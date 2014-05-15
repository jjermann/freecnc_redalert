#include "SDL.h"
#include <cstdlib>
#include <cstring>
#include "common.h"
#include "config.h"
#include "cursor.h"
#include "cursorpool.h"
#include "logger.h"

Uint8 Cursor::nsoff;

/** Constructor, load the cursorimage and set upp all surfaces to buffer
 * background in etc.
 */
Cursor::Cursor()
{
    SDL_Surface *tmp;

    curimg = 0;
    nimgs = 1;

    cursorimg = new Dune2Image("mouse.shp", -1);
    if (getConfig().gamenum == GAME_RA) {
        transicn = new TemplateImage("trans.icn", mapscaleq, 1);
        nsoff = CUR_RA_NOSCROLL_OFFSET;
    } else {
        transicn = new TemplateImage("trans.icn", mapscaleq);
        nsoff = CUR_NOSCROLL_OFFSET;
    }

    image[curimg] = cursorimg->getImage(0);

    /* All cursors loaded */

    tmp = SDL_CreateRGBSurface (SDL_SWSURFACE, image[0]->w, image[0]->h, 16, 0xff, 0xff, 0xff, 0);
    bg = SDL_DisplayFormat( tmp );
    SDL_FreeSurface( tmp );

    currentcursor = CUR_STANDARD;
    x = 0;
    y = 0;
    cursor_offset = 0;

    cursorpool = new CursorPool("cursors.ini");
    ci = NULL;

    transw = transicn->getImage(0);
    transy = transicn->getImage(1);
    transr = transicn->getImage(2);
}

/** Destructor, free the surfaces */
Cursor::~Cursor()
{
    int i;
    for (i = 0; i < nimgs; ++i)
        SDL_FreeSurface(image[i]);
    SDL_FreeSurface(bg);
    SDL_FreeSurface(transw);
    SDL_FreeSurface(transy);
    SDL_FreeSurface(transr);

    delete cursorimg;
    delete transicn;
    delete cursorpool;
}

/** Change active cursor.
 * @param number of the new cursor.
 */
void Cursor::setCursor( Uint16 cursornum, Uint8 animimages )
{
    int i;
    if( currentcursor == cursornum )
        return;

    for( i = 0; i < nimgs; i++ )
        SDL_FreeSurface( image[i] );

    curimg = 0;
    nimgs = animimages;
    for( i = 0; i < nimgs; i++ )
        image[i] = cursorimg->getImage(cursornum+i);

    if( cursornum != CUR_STANDARD )
        cursor_offset = -((image[0]->w)>>1);
    else
        cursor_offset = 0;

    currentcursor = cursornum;
}

void Cursor::setCursor(const char* curname)
{
    int i;

    ci = cursorpool->getCursorByName(curname);

    if( currentcursor == ci->anstart)
        return;

    for( i = 0; i < nimgs; i++ )
        SDL_FreeSurface( image[i] );

    curimg = 0;
    nimgs = ci->anend - ci->anstart + 1;
    for( i = 0; i < nimgs; i++ )
        image[i] = cursorimg->getImage((ci->anstart)+i);

    if( strcasecmp(curname,"STANDARD") == 0 ) {
        cursor_offset = 0;
    } else {
        cursor_offset = -((image[0]->w)>>1);
    }

    currentcursor = ci->anstart;
}

void Cursor::setPlaceCursor(Uint8 stw, Uint8 sth, Uint8 *icn)
{
    int i, x, y;
    SDL_Surface *bigimg;

    Uint8 *data;
    SDL_Rect dest;
    static Uint32 oldptr;
    Uint32 newptr;

    newptr = 0;
    for(i = 0; i < stw*sth; i++) {
        newptr = newptr<<2|(icn[i]);
    }

    if (currentcursor == CUR_PLACE && newptr == oldptr) {
        return;
    }
    oldptr = newptr;

    for( i = 0; i < nimgs; i++ )
        SDL_FreeSurface( image[i] );
    curimg = 0;
    currentcursor = CUR_PLACE;
    nimgs = 1;
    cursor_offset = 0;

    data = new Uint8[stw*sth*transw->w*transw->h];
    memset(data, 0, stw*sth*transw->w*transw->h);

    bigimg = SDL_CreateRGBSurfaceFrom(data, stw*transw->w, sth*transw->h,
                                      8, stw*transw->w, 0, 0, 0, 0);

    SDL_SetColors(bigimg, SHPBase::getPalette(0), 0, 256);
    SDL_SetColorKey(bigimg, SDL_SRCCOLORKEY, 0);

    image[0] = SDL_DisplayFormat(bigimg);
    SDL_FreeSurface(bigimg);
    delete[] data;

    dest.w = transw->w;
    dest.h = transw->h;

    dest.y = 0;
    for( y = 0; y < sth; y++ ) {
        dest.x = 0;
        for( x = 0; x < stw; x++ ) {
            SDL_Surface** tile = 0;
            if (icn[y*stw+x] == 0) {
                dest.x += dest.w;
                continue;
            }
            switch (icn[y*stw+x]) {
            case 1:
                tile = &transw;
                break;
            case 2:
                tile = &transy;
                break;
            case 4:
                tile = &transr;
                break;
            default:
#if _MSC_VER && _MSC_VER < 1300
                logger->error("Possible memory corruption detected in %s(%d): icn[%i*%i+%i] = %i\n",__FILE__,__LINE__,y,stw,x,icn[y*stw+x]);
#else
                logger->error("Possible memory corruption detected in %s: icn[%i*%i+%i] = %i\n",__FUNCTION__,y,stw,x,icn[y*stw+x]);
#endif
                //throw InvalidValue();
                throw 0;
                break;
            }
            SDL_BlitSurface(*tile, NULL, image[0], &dest);
            dest.x += dest.w;
        }
        dest.y += dest.h;
    }
}

SDL_Surface *Cursor::getCursor()
{
    static Uint32 lastchange = 0;
    Uint32 tick = SDL_GetTicks();
    if( tick > lastchange +100 ) {
        curimg++;
        if( curimg >= nimgs )
            curimg = 0;
        lastchange = tick;
    }
    return image[curimg];
}
