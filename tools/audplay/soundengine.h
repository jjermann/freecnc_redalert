// mode: -*- C++ -*-
#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include <deque>
#include <vector>
#include "SDL.h"
#include "sound.h"

#define FREQUENCY               22050
#define MAX_CHUNK_SIZE 		16384
#define MAX_UNCOMPRESSED_SIZE 	MAX_CHUNK_SIZE << 2
#define MAX_COMPRESSED_SIZE	MAX_CHUNK_SIZE

class SoundEngine
{
public:
    SoundEngine();
    ~SoundEngine();
    void queueSound(char* sname);
    Uint32 getBufferLen()
    {
        return audio_len;
    }
private:
    void fillBuffer();
    Sound * gamesound;
    bool played;
    /* Static functions and variables to handle the buffer filling process */
    static void fill_audio(void *instance, Uint8 *stream, int len);
    static Uint8 *audio_chunk;      /* Pointer to chunk of sound data */
    static Uint8 *audio_pos;        /* Pointer to current position in sound data */
    static Uint32 audio_len;	   /* Length of audio_chunk */
};

/* class to throw when we encounter an error */
class SoundError {};

#endif
