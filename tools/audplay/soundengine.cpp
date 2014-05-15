// cut down soundengine for audplay
#include <cstring>
#include "common.h"
#include "soundengine.h"
#include "inifile.h"
#include "logger.h"
#include "common.h"

using namespace std;

extern bool done;

Uint8 *SoundEngine::audio_chunk;
Uint8 *SoundEngine::audio_pos;
Uint32 SoundEngine::audio_len;

/** Constructor, initalizes the soundengine and start playing sounds
 */
SoundEngine::SoundEngine()
{
    SDL_AudioSpec desired;

    /* init the sound */
    desired.freq = FREQUENCY;
    desired.format = AUDIO_S16SYS;  /* Signed 16-bit Sample */
    desired.channels = 1;           /* mono - should be configurable? */
    desired.samples = 8192;         /* Maximum buffer size */
    desired.callback = SoundEngine::fill_audio;  /* Fill audio buffer */
    desired.userdata = this;

    if ( SDL_OpenAudio(&desired, NULL) < 0) {
        logger->error("Couldn't open audio %s\n", SDL_GetError());
        throw SoundError();
    }
    /* clear the soundbuffer */
    audio_chunk = new Uint8[MAX_UNCOMPRESSED_SIZE];
    memset(audio_chunk, 0, MAX_UNCOMPRESSED_SIZE);
    audio_pos = audio_chunk;
    audio_len = 0;
    played = false;
    /* unpause the soundplaying */
    SDL_PauseAudio(0);
}

/** Destructor, closes the audio and frees some memory */
SoundEngine::~SoundEngine()
{
    SDL_CloseAudio();
    delete[] audio_chunk;
    if (gamesound != NULL)
        delete gamesound;
}

/** add a ingame sound to the sound buffer/queue, will start playing next time
 * the buffer needs filling. */
void SoundEngine::queueSound(char* sname)
{
    if (sname == NULL) {
        logger->error("Attempted to queue a NULL sound\n");
        return;
    }
    Sound *gs;
    played = true;
    try {
        gs = new Sound(sname);
    } catch (SoundError&) {
        gamesound = NULL;
        throw SoundError();
    }
    gamesound = gs;
}

/** Method called when the buffer is running low so the playingbuffer can't
 * be filled.
 */
void SoundEngine::fillBuffer()
{
    Uint8 *sndptr;
    Uint16 slen = 0;
    Uint16 len;
    Uint8 *tmpbuf;

    if (played && gamesound == NULL) {
        done = true;
        return;
    }
    tmpbuf = NULL;
    if( audio_len != 0 ) {
        tmpbuf = new Uint8[audio_len];
        memcpy( tmpbuf, audio_pos, audio_len );
    }

    audio_pos = audio_chunk;
    memset( audio_chunk, 0, MAX_UNCOMPRESSED_SIZE );

    if( tmpbuf != NULL ) {
        memcpy( audio_chunk, tmpbuf, audio_len );
        audio_pos = audio_chunk+audio_len;
        delete[] tmpbuf;
    }

    if( gamesound ) {
        len = gamesound->decodeSample(&sndptr);
        if( len == 0 ) {
            delete gamesound;
            gamesound = NULL;
        } else {
            slen = max(len, slen);
            SDL_MixAudio(audio_pos, sndptr, len, SDL_MIX_MAXVOLUME);
        }
    }
    audio_len += slen;
}


/** callback function for audio.
 * @param the current instance of the soundengine.
 * @param the audio stream which is played.
 * @param the length of the playingstream.
 */
void SoundEngine::fill_audio(void *instance, Uint8 *stream, int len)
{
    /* if the buffer is shorter than the playingbuffer try to
       add more data to it */
    if (((int)audio_len) < len)
        ((SoundEngine *)instance)->fillBuffer();
    if (audio_len == 0) {
        return;
    }
    len = (len > (int)audio_len ? audio_len : len);
    /* mix in as much data as possible */
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_len -= len;
    audio_pos += len;
}

