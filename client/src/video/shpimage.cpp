/*****************************************************************************
 * shpimage.cpp - code to load/decode shp files
 *
 ****************************************************************************/
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include "SDL.h"
#include "compression.h"
#include "fcnc_endian.h"
#include "imageproc.h"
#include "inifile.h"
#include "logger.h"
#include "shpimage.h"
#include "vfs.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::string;
using std::runtime_error;
#endif

const Uint8 SHPBase::numpals = 6;

/**
 * @TODO These palettes only differ between 179 and 190.
 * In this range are two gradients from bright to dark of that colour.
 * Should calculate the gradients and SDL_MapRGB them to the palette.
 */

// Colour for NOD structures and Soviet units and structures (red)
const Uint8 palone[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0x7f, 0x7e, 0x7d, 0x7c, 0x7a, 0x2e, 0x78, 0x2f, 0x7d, 0x7c, 0x7b, 0x7a, 0x2a, 0x79, 0x78, 0x78,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

// Colour used for NOD units. (bluey gray)
const Uint8 paltwo[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0xa1, 0xc8, 0xc9, 0xca, 0xcc, 0xcd, 0xce, 0x0c, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0x73, 0xc6, 0x72,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

// Colour used for a multiplayer side (fluorescent orange)
const Uint8 palthree[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0x18, 0x19, 0x1a, 0x1b, 0x1d, 0x1f, 0x2e, 0x2f, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x2b, 0x2f,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

// Another multiplayer colour (fluorescent green)
const Uint8 palfour[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0x05, 0xa5, 0xa6, 0xa7, 0x9f, 0x8e, 0x8c, 0xc7, 0xa6, 0xa7, 0x9d, 0x03, 0x9f, 0x8f, 0x8e, 0x8d,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

// Another multiplayer colour (turquoise)
const Uint8 palfive[] =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        0x02, 0x77, 0x76, 0x87, 0x88, 0x8b, 0x70, 0x0c, 0x76, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x72, 0x70,
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

SDL_Color SHPBase::palette[32][256];
void SHPBase::setPalette(SDL_Color *pal)
{
    memcpy(palette[0], pal, 256*sizeof(SDL_Color));
}
void SHPBase::calculatePalettes()
{
    int i;
    for( i = 0; i < 256; i++ )
        palette[1][i] = palette[0][palone[i]];
    for (i = 0; i < 256; ++i)
        palette[2][i] = palette[0][paltwo[i]];
    for (i = 0; i < 256; ++i)
        palette[3][i] = palette[0][palthree[i]];
    for (i = 0; i < 256; ++i)
        palette[4][i] = palette[0][palfour[i]];
    for (i = 0; i < 256; ++i)
        palette[5][i] = palette[0][palfive[i]];
}

// Note: scaleq has a default of -1
SHPBase::SHPBase(const char* fname, Sint8 scaleq) : name(fname), scaleq(scaleq), scaler(new ImageProc()) {
}

SHPBase::~SHPBase() {
    delete scaler;
}

SDL_Surface* SHPBase::scale(SDL_Surface *input, int quality) {
    return scaler->scale(input, quality);
}
/** Constructor, loads a shpfile.
 * @param fname the filename
 * @param scaleq scaling option (-1 is disabled)
 */
SDL_Color SHPImage::shadowpal[2] = {{0xff,0xff,0xff,0},{0x0,0x0,0x0,0x0}};

SHPImage::SHPImage(const char *fname, Sint8 scaleq) : SHPBase(fname, scaleq)
{
    int i, j;
    VFile *imgfile;
    imgfile = VFS_Open(fname);
    if (0 == imgfile) {
        throw ImageNotFound();
    }
    shpdata = new Uint8[imgfile->fileSize()];
    imgfile->readByte(shpdata, imgfile->fileSize());

    // Header
    header.NumImages = readword(shpdata,0);
    header.Width = readword(shpdata, 6);

    header.Height = readword(shpdata, 8);
    header.Offset = new Uint32[header.NumImages + 2];
    header.Format = new Uint8[header.NumImages + 2];
    header.RefOffs = new Uint32[header.NumImages + 2];
    header.RefFormat = new Uint8[header.NumImages + 2];

    // "Offsets"
    j = 14;
    for (i = 0; i < header.NumImages + 2; i++) {
        header.Offset[i] = readthree(shpdata, j);
        j += 3;
        header.Format[i] = readbyte(shpdata, j);
        j += 1;
        header.RefOffs[i] = readthree(shpdata, j);
        j += 3;
        header.RefFormat[i] = readbyte(shpdata, j);
        j += 1;
    }
    VFS_Close(imgfile);
}

/** Destructor, freas the memory used by the shpimage. */
SHPImage::~SHPImage()
{
    delete[] shpdata;
    delete[] header.Offset;
    delete[] header.Format;
    delete[] header.RefOffs;
    delete[] header.RefFormat;
}

/** Extract a frame from a SHP into two SDL_Surface* (shadow is separate)
 * @param imgnum the index of the frame to decode.
 * @param img pointer to the SDL_Surface* into which the frame is decoded.
 * @param shadow pointer to the SDL_Surface* into which the shadow frame is
 * decoded.  This can be 0 if you don't need the shadow.
 */
void SHPImage::getImage(Uint16 imgnum, SDL_Surface **img, SDL_Surface **shadow, Uint8 palnum)
{
    if (0 == img) {
        string s = name + ": can't decode to a NULL surface";
        /// @TODO This really should be a logic_error.
        throw runtime_error(s);
    }

    Uint8* imgdata = new Uint8[header.Width * header.Height];
    DecodeSprite(imgdata, imgnum);

    if (0 != shadow) {
        Uint8* shadowdata = new Uint8[header.Width * header.Height];
        memset(shadowdata, 0, header.Width * header.Height);
        for (int i = 0; i<header.Width * header.Height; ++i) {
            if (4 == imgdata[i]) {
                imgdata[i] = 0;
                shadowdata[i] = 1;
            }
        }
        SDL_Surface* shadowimg = SDL_CreateRGBSurfaceFrom(shadowdata,
                header.Width, header.Height, 8, header.Width, 0, 0, 0, 0);
        SDL_SetColors(shadowimg, shadowpal, 0, 2);
        SDL_SetColorKey(shadowimg, SDL_SRCCOLORKEY, 0);
        SDL_SetAlpha(shadowimg, SDL_SRCALPHA|SDL_RLEACCEL, 128);

        if( scaleq >= 0 ) {
            *shadow = scale(shadowimg, scaleq);
            SDL_SetColorKey(*shadow, SDL_SRCCOLORKEY, 0);
        } else {
            *shadow = SDL_DisplayFormat(shadowimg);
        }
        SDL_FreeSurface(shadowimg);
        delete[] shadowdata;
    } else {
        for (int i = 0; i<header.Width * header.Height; ++i) {
            if (4 == imgdata[i]) {
                imgdata[i] = 0;
            }
        }
    }
    SDL_Surface* imageimg = SDL_CreateRGBSurfaceFrom(imgdata, header.Width,
            header.Height, 8, header.Width, 0, 0, 0, 0);
    SDL_SetColors(imageimg, palette[palnum], 0, 256);
    SDL_SetColorKey(imageimg, SDL_SRCCOLORKEY, 0);
    if( scaleq >= 0 ) {
        *img = scale(imageimg, scaleq);
        SDL_SetColorKey(*img, SDL_SRCCOLORKEY, 0);
    } else {
        *img = SDL_DisplayFormat(imageimg);
    }
    SDL_FreeSurface(imageimg);
    delete[] imgdata;
}

/** @todo: this is a really ugly solution to get the shadow.shp as black and transparent.
     It also flips which is which. This must be done in a better way in the rewrite */
void SHPImage::getImageAsAlpha(Uint16 imgnum, SDL_Surface **img)
{
    int i;
    Uint8 *imgdata;
    SDL_Surface *imageimg;

    imgdata = new Uint8[header.Width * header.Height];

    DecodeSprite(imgdata, imgnum);

    for( i = 0; i<header.Width * header.Height; i++ ) {
        if( imgdata[i] != 0 ) {
            imgdata[i] = 0;
        } else {
            imgdata[i] = 1;
        }
    }
    imageimg = SDL_CreateRGBSurfaceFrom(imgdata, header.Width, header.Height,
                                        8, header.Width, 0, 0, 0, 0);
    SDL_SetColors(imageimg, shadowpal, 0, 2);
    SDL_SetColorKey(imageimg, SDL_SRCCOLORKEY, 0);
    if( scaleq >= 0 ) {
        *img = scale(imageimg, scaleq);
        SDL_SetColorKey(*img, SDL_SRCCOLORKEY, 0);
    } else {
        *img = SDL_DisplayFormat(imageimg);
    }
    SDL_FreeSurface(imageimg);
    delete[] imgdata;

}


/** Method to decompress a format xx compressed image.
 * @param imgdst The buffer in which to put the image (must contain XOR image).
 * @param imgnum The index of the frame to decompress.
 */
void SHPImage::DecodeSprite(Uint8 *imgdst, Uint16 imgnum)
{
    Uint32 len;
    Uint8 *imgsrc;
    Uint32 i;

    if (imgnum >= header.NumImages) {
        logger->error("%s: Invalid SHP imagenumber (%i >= %i)\n",name.c_str(),imgnum,header.NumImages);
        return;
    }
    switch (header.Format[imgnum]) {
    case FORMAT_80:
        len = header.Offset[imgnum + 1] - header.Offset[imgnum];
        imgsrc = new Uint8[len];
        memcpy( imgsrc, shpdata + header.Offset[imgnum], len );
        memset(imgdst, 0, sizeof(imgdst));
        Compression::decode80(imgsrc, imgdst);
        break;
    case FORMAT_40:
        for( i = 0; i < header.NumImages; i++ ) {
            if( header.Offset[i] == header.RefOffs[imgnum] )
                break;
        }
        DecodeSprite( imgdst, i );
        len = header.Offset[imgnum + 1] - header.Offset[imgnum];
        imgsrc = new Uint8[len];
        memcpy( imgsrc, shpdata + header.Offset[imgnum], len );
        Compression::decode40(imgsrc, imgdst);
        break;
    case FORMAT_20:
        DecodeSprite( imgdst, imgnum - 1 );
        len = header.Offset[imgnum + 1] - header.Offset[imgnum];
        imgsrc = new Uint8[len];
        memcpy( imgsrc, shpdata + header.Offset[imgnum], len );
        Compression::decode40(imgsrc, imgdst);
        break;
    default:
        logger->error("Possible memory corruption detected: unknown header format in %s at frame %i/%i.\n",name.c_str(),imgnum,header.NumImages);
        return;
        break;
    }
    delete[] imgsrc;
}

/** Constructor loads a dune2 shpfile.
 * @param the mixfiles.
 * @param the name of the dune2 shpfile.
 */
Dune2Image::Dune2Image(const char *fname, Sint8 scaleq) : SHPBase(fname,scaleq)
{
    VFile *imgfile;

    imgfile = VFS_Open(fname);
    if( imgfile == NULL ) {
        fprintf(stderr, "File \"%s\" not found.\n", fname);
        shpdata = NULL;
        throw ImageNotFound();
    }
    //shpdata = mixes->extract(fname);
    shpdata = new Uint8[imgfile->fileSize()];
    imgfile->readByte(shpdata, imgfile->fileSize());
    VFS_Close(imgfile);
}

/** Destructor, frees up the memory used by a dune2 shp. */
Dune2Image::~Dune2Image()
{
    delete[] shpdata;
}

/** Decode a image in the dune2 shp.
 * @param the number of the image to decode.
 * @returns a SDL_Surface containing the image.
 */
SDL_Surface *Dune2Image::getImage(Uint16 imgnum)
{
    SDL_Surface *image, *optimage;
    Uint32 startpos;
    Uint8 *d, *data;

    startpos = getD2Header( imgnum );

    data = new Uint8[header.cx * header.cy];

    if( ~header.compression & 2 ) {
        d = new Uint8[header.size_out];

        memset(d, 0, header.size_out);

        Compression::decode20( d, data, Compression::decode80( shpdata+startpos, d ) );

        delete[] d;
    } else
        Compression::decode20( shpdata+startpos, data, header.size_out );

    image = SDL_CreateRGBSurfaceFrom(data, header.cx, header.cy,
                                     8, header.cx, 0, 0, 0, 0);

    /// @bug TEMPORARY HACK
    // The index 0x0c is used to give some cursors shadows, this is defined in
    // the palette as (0,0,0), which is also the colour of the index 0, which
    // has to be set transparent.  Not sure why it kills 0x0c as well as 0x0.
    // 0x0c does not get killed in 8 bit mode.
    for (Sint32 pos = 0; pos < header.cx*header.cy; ++pos) {
        if (data[pos] == 0x0c)
            data[pos] = 0x9a;
    }

    SDL_SetColors(image, palette[0], 0, 256);
    SDL_SetColorKey(image, SDL_SRCCOLORKEY, 0);

    if (scaleq >= 0) {
        optimage = scale(image, scaleq);
        SDL_SetColorKey(optimage, SDL_SRCCOLORKEY, 0);
    } else {
        optimage = SDL_DisplayFormat(image);
    }
    SDL_FreeSurface(image);
    delete[] data;

    return optimage;
}

/** Read the header of a specified dune2 shp.
 * @param the number of the image to read the header from.
 * @returns the offset of the image.
 */
Uint32 Dune2Image::getD2Header(Uint16 imgnum)
{
    Uint16 imgs;
    Uint32 curpos;

    imgs = readword(shpdata, 0);

    if (imgnum >= imgs) {
        logger->error("%s: getD2Header called with invalid param: %i (>= %i)\n",
                name.c_str(), imgnum, imgs);
        return 0;
    }

    if( readword(shpdata, 4) ) {
        curpos = readword(shpdata, imgnum*2 + 2 );
    } else {
        curpos = readlong(shpdata, imgnum*4 + 2) + 2;
    }

    header.compression = readword(shpdata, curpos);
    curpos+=2;
    header.cy = readbyte(shpdata, curpos);
    curpos++;
    header.cx = readword(shpdata, curpos);
    curpos += 2;
    header.cy2 = readbyte(shpdata, curpos);
    curpos++;
    header.size_in = readword(shpdata, curpos);
    curpos += 2;
    header.size_out = readword(shpdata, curpos);
    curpos += 2;

    if( header.compression & 1 )
        curpos += 16;

    return curpos;

}

// ratemp defaults to false
TemplateImage::TemplateImage(const char *fname, Sint8 scaleq, bool ratemp)
    : SHPBase(fname, scaleq), ratemp(ratemp)
{
    tmpfile = VFS_Open(fname);
    if (0 == tmpfile) {
        throw ImageNotFound();
    }
}

TemplateImage::~TemplateImage()
{
    VFS_Close(tmpfile);
}

Uint16 TemplateImage::getNumTiles()
{
    Uint16 data;
    tmpfile->seekSet(4);
    tmpfile->readWord(&data, 1);
    return data;
}

SDL_Surface *TemplateImage::getImage(Uint16 imgnum)
{
    Uint32 imgStart, index1;
    Uint16 imgwidth, imgheight, numtil;
    Uint8 index1val;

    Uint8 *imgdata;
    SDL_Surface *sdlimage, *retimage;

    tmpfile->seekSet(0);
    /* Read width, hight and number of tiles in template */
    tmpfile->readWord(&imgwidth, 1);
    tmpfile->readWord(&imgheight, 1);
    tmpfile->readWord(&numtil, 1);

    if (imgnum >= numtil) {
        return NULL;
    }

    /* Skip some constants */
    if (ratemp) {
        tmpfile->seekCur(10);
    } else {
        tmpfile->seekCur(6);
    }
    /* Load the offset to the image */
    tmpfile->readDWord(&imgStart, 1);

    /* Skip some constants */
    if (ratemp) {
        tmpfile->seekCur(16);
    } else {
        tmpfile->seekCur(12);
    }

    /* Load address of index1 */
    tmpfile->readDWord(&index1, 1);

    /* Read the index1 value of the tile */
    tmpfile->seekSet(index1+imgnum);
    tmpfile->readByte( &index1val, 1 );

    if( index1val == 0xff ) {
        return NULL;
    }
    /* Seek the start of the image */
    tmpfile->seekSet(imgStart+imgwidth*imgheight*index1val);

    /* allocate space for the imagedata and load it */
    imgdata = new Uint8[imgwidth*imgheight];
    tmpfile->readByte( imgdata, imgwidth*imgheight );

    /* The image is made up from the data */
    sdlimage = SDL_CreateRGBSurfaceFrom(imgdata, imgwidth, imgheight,
                                        8, imgwidth, 0, 0, 0, 0);

    /* Set the palette to be the map's palette */
    SDL_SetColors(sdlimage, palette[0], 0, 256);
    SDL_SetColorKey(sdlimage, SDL_SRCCOLORKEY, 0);

    if( scaleq >= 0 ) {
        retimage = scale(sdlimage, scaleq);
        SDL_SetColorKey(sdlimage, SDL_SRCCOLORKEY, 0);
    } else {
        retimage = SDL_DisplayFormat(sdlimage);
    }
    SDL_FreeSurface(sdlimage);
    delete[] imgdata;

    return retimage;
}