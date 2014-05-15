// mode: -*- C++ -*-
#ifndef SOUND_H
#define SOUND_H

#include "SDL.h"

class VFile;

// Uint32 appear first due to alignment problem
struct aud_header
{
    Uint32 size;
    Uint32 outsize;
    Uint16 frequency;
    Uint8 flags;
    Uint8 type;
};

class Sound
{
public:
    Sound(const char *sndname );
    ~Sound();
    Uint16 decodeSample(Uint8 **data);
private:
    void Convert11to22(Uint8* buf, Uint16 len);
    VFile *sndfile;
    Uint32 offset;
    Uint8 *outBuf;
    Sint32 index;
    Sint32 sample;
    aud_header header;
    bool wsadpcm;
};

#endif
