#include <cstring>
#include <stdexcept>
#include <string>
#include "SDL.h"
#include "fcnc_endian.h"
#include "font.h"
#include "logger.h"
#include "vfs.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::runtime_error;
using std::string;
#endif

Font::Font(const char *fontname) : SHPBase(fontname)
{
    VFile *fontfile;
    Uint16 wpos, hpos, cdata, nchars;
    Uint8 fnheight, fnmaxw;
    Uint32 fntotalw;
    Uint8 *chardata, *wchar, *hchar;
    Uint32 ypos, i, pos, curchar;
    Uint8 data;
    Uint16 *dataoffsets;
    SDL_Surface *imgtmp;
    SDL_Color white[] = {{0x0, 0x0, 0x0}, {0xff, 0xff, 0xff}};
//    SDL_Color white[] = {{0xff,0xff,0xff,0},{0x0,0x0,0x0,0x0}};;
    fontfile = VFS_Open(fontname);

    if (0 == fontfile) {
        string s = "Unable to load font ";
        s += fontname;
        throw(runtime_error(s));
    }

    fontfile->seekSet(8);
    fontfile->readWord(&wpos, 1);
    fontfile->readWord(&cdata, 1);
    fontfile->readWord(&hpos, 1);
    fontfile->seekCur(2);
    fontfile->readWord(&nchars, 1);
    nchars = SDL_Swap16(nchars); // Yes, even on LE systems.
    fontfile->readByte(&fnheight, 1);
    fontfile->readByte(&fnmaxw, 1);

    nchars++;

    wchar = new Uint8[nchars];
    hchar = new Uint8[nchars<<1];

    dataoffsets = new Uint16[nchars];
    fontfile->readWord(dataoffsets, nchars);

    fontfile->seekSet(wpos);
    fontfile->readByte( wchar, nchars );
    fontfile->seekSet(hpos);
    fontfile->readByte( hchar, nchars<<1 );

    chrdest = new SDL_Rect[nchars];

    fntotalw = 0;
    for( i = 0 ; i<nchars; i++ ) {
        chrdest[i].x = fntotalw;

        chrdest[i].y = 0;
        chrdest[i].h = fnheight;
        chrdest[i].w = wchar[i];
        fntotalw += wchar[i];
    }
    chardata = new Uint8[fnheight*fntotalw];
    memset(chardata, 0, fnheight*fntotalw);

    for( curchar = 0; curchar < nchars; curchar++ ) {
        fontfile->seekSet( dataoffsets[curchar] );
        for( ypos = hchar[curchar<<1]; ypos < (Uint32)(hchar[curchar<<1]+hchar[(curchar<<1)+1]); ypos++ ) {
            pos = chrdest[curchar].x+ypos*fntotalw;
            for( i = 0; i < wchar[curchar]; i+=2 ) {
                fontfile->readByte( &data, 1 );
                chardata[pos+i] = (data&0xf)!=0?1:0;
                if( i+1<wchar[curchar] )
                    chardata[pos+i+1] = (data>>4)!=0?1:0;
            }
        }
    }
    imgtmp = SDL_CreateRGBSurfaceFrom(chardata, fntotalw, fnheight,
                                      8, fntotalw, 0, 0, 0, 0);
    //   SDL_SetColors(imgtmp, palette[0], 0, 256);
    SDL_SetColors(imgtmp, white, 0, 2);
    SDL_SetColorKey(imgtmp, SDL_SRCCOLORKEY, 0);

    fontimg = SDL_DisplayFormat(imgtmp);
    SDL_FreeSurface(imgtmp);

    VFS_Close(fontfile);

    delete[] chardata;
    delete[] wchar;
    delete[] hchar;
    delete[] dataoffsets;
}

Font::~Font()
{
    SDL_FreeSurface(fontimg);
    delete[] chrdest;
}

Uint32 Font::getHeight()
{
    return chrdest[0].h;
}

Uint32 Font::calcStringWidth(const char *string)
{
    Uint32 wdt = 0;
    Uint32 i;

    for( i = 0; string[i] != '\0'; i++ )
        wdt += chrdest[string[i]].w+1;
    return wdt;
}

void Font::drawString(const char *string, SDL_Surface *dest,
                      Uint32 startx, Uint32 starty)
{
    Uint32 i;
    SDL_Rect destr;
    destr.x = startx;
    destr.y = starty;
    destr.h = chrdest[0].h;

    for( i = 0; string[i] != '\0'; i++ ) {
        destr.w = chrdest[string[i]].w;
        SDL_BlitSurface( fontimg, &chrdest[string[i]], dest, &destr );
        //      destr.w = chrdest['a'+i].w;
        //      SDL_BlitSurface( fontimg, &chrdest['a'+i], dest, &destr );
        destr.x += destr.w+1;
    }
}
