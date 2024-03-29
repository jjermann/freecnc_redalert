#include <cstdlib>
#include <cstring>
#include "SDL.h"
#include "compression.h"
#include "cpsimage.h"
#include "fcnc_endian.h"
#include "inifile.h"
#include "shpimage.h"
#include "vfs.h"

CPSImage::CPSImage(const char* fname, int scaleq) : cpsdata(0), image(0) {
    VFile* imgfile;
    this->scaleq = scaleq;
    imgfile = VFS_Open(fname);
    if (imgfile == NULL) {
        throw ImageNotFound();
    }
    imgsize = imgfile->fileSize();
    image = NULL;
    cpsdata = new Uint8[imgsize];
    imgfile->readByte(cpsdata, imgsize);
    header.size    = readword(cpsdata,0);
    header.unknown = readword(cpsdata,2);
    header.imsize  = readword(cpsdata,4);
    header.palette = readlong(cpsdata,6);
    if (header.palette == 0x3000000) {
        readPalette();
    } else {
        // magic here to select appropriate palette
        offset = 10;
    }
    VFS_Close(imgfile);
}

CPSImage::~CPSImage()
{
    delete[] cpsdata;
    SDL_FreeSurface(image);
}

void CPSImage::readPalette()
{
    Uint16 i;
    offset = 10;
    for (i = 0; i < 256; i++) {
        palette[i].r = readbyte(cpsdata, offset);
        palette[i].g = readbyte(cpsdata, offset+1);
        palette[i].b = readbyte(cpsdata, offset+2);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
        offset += 3;
    }
}

SDL_Surface* CPSImage::getImage()
{
    if (image == NULL) {
        loadImage();
    }
    return image;
}

void CPSImage::loadImage()
{
    Uint32 len;
    Uint8* imgsrc;
    Uint8 *imgdst;
    SDL_Surface* imgtmp;
    len = imgsize-offset;
    imgsrc = new Uint8[len];
    imgdst = new Uint8[header.imsize];
    memcpy(imgsrc, cpsdata + offset, len);
    memset(imgdst, 0, header.imsize);
    Compression::decode80(imgsrc, imgdst);
    imgtmp = SDL_CreateRGBSurfaceFrom(imgdst,320,200,8,320,0,0,0,0);
    SDL_SetColors(imgtmp,palette,0,256);
    SDL_SetColorKey(imgtmp,SDL_SRCCOLORKEY,0);
    delete[] imgsrc;
    delete[] cpsdata;
    cpsdata = NULL;
    if (scaleq >= 0) {
        image = scaler.scale(imgtmp,scaleq);
        SDL_SetColorKey(image,SDL_SRCCOLORKEY,0);
    } else {
        image = SDL_DisplayFormat(imgtmp);
    }
    SDL_FreeSurface(imgtmp);
    delete[] imgdst;
}

