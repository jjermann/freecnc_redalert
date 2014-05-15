// mode: -*- C++ -*-
#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <map>
#include <string>
#include <vector>
#include "SDL.h"
#include "shpimage.h"

struct ImageCacheEntry
{
    ImageCacheEntry();
    ~ImageCacheEntry();
    void clear();
    SDL_Surface *image;
    SDL_Surface *shadow;
};

class ImageCache
{
public:
    ImageCache();
    ~ImageCache();
    void setImagePool(std::vector<SHPImage *> *imagepool);
    ImageCacheEntry& getImage(Uint32 imgnum);

    /// @brief Loads the shpimage fname into the imagecache.
    Uint32 loadImage(const char* fname);

    void newCache();
private:
    std::map<Uint32, ImageCacheEntry> cache, prevcache;
    std::map<std::string, Uint32> namecache;
    std::vector<SHPImage *> *imagepool;
};

#endif
