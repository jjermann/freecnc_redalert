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
#include "SDL_thread.h"
#include "sound.h"

#define FREQUENCY               22050
#define MAX_CHUNK_SIZE   16384
#define MAX_UNCOMPRESSED_SIZE  MAX_CHUNK_SIZE << 2
#define MAX_COMPRESSED_SIZE MAX_CHUNK_SIZE

class SoundEngine
{
public:
    SoundEngine(bool nosound = false);
    ~SoundEngine();
    void playSong(const char *sname);
    void stopSong();
    void queueSound(const char* sname);
    void initVQAsound(Uint8 *buf, Uint16 len);
    void addVQAsound(Uint8 *buf, Uint16 len);
    void closeVQAsound();
    Uint32 getBufferLen()
    {
        return audio_len;
    }
    bool createPlayList(const char* gamename);
private:
    void fillBuffer();

    Sound *music;
    std::vector<char*> songlist;
    Uint8 songindex;
    Uint8 lastbad;

    std::deque<Sound *> gamesounds;

    Uint8 playingvqa;
    Uint16 vqasamplelen;
    Uint16 vqainitlen;
    SDL_sem *vqasem;
    Uint16 lastposinvqastream;
    Uint16 lastposinvqabuf;

    /* Static functions and variables to handle the buffer filling process */
    static void fill_audio(void *instance, Uint8 *stream, int len);
    static Uint8 *audio_chunk;      /* Pointer to chunk of sound data */
    static Uint8 *audio_pos;        /* Pointer to current position in sound data */
    static Uint32 audio_len;    /* Length of audio_chunk */
    bool nosound;
};

/* class to throw when we encounter an error */
class SoundError
    {}
;

#endif
