#include <cstring>
#include <stdexcept>
#include "common.h"
#include "inifile.h"
#include "logger.h"
#include "soundengine.h"

using std::runtime_error;

Uint8 *SoundEngine::audio_chunk;
Uint8 *SoundEngine::audio_pos;
Uint32 SoundEngine::audio_len;

/** Constructor, initalizes the soundengine and start playing sounds
 */
SoundEngine::SoundEngine(bool nosound)
{
    SDL_AudioSpec desired;

    /* init the sound */
    desired.freq = FREQUENCY;
    desired.format = AUDIO_S16SYS;  /* Signed 16-bit Sample */
    desired.channels = 1;           /* mono - should be configurable? */
    desired.samples = 1024;         /* Maximum buffer size */
    desired.callback = SoundEngine::fill_audio;  /* Fill audio buffer */
    desired.userdata = this;
    desired.padding = 0; // keeps valgrind happy

    if (!nosound) {
        if ( SDL_OpenAudio(&desired, NULL) < 0) {
            logger->error("Couldn't open audio %s\n", SDL_GetError());
            throw SoundError();
        }
        /* clear the soundbuffer */
        audio_chunk = new Uint8[MAX_UNCOMPRESSED_SIZE];
        memset(audio_chunk, 0, MAX_UNCOMPRESSED_SIZE);
        audio_pos = audio_chunk;
        audio_len = 0;
        this->nosound = false;
        /* unpause the soundplaying */
        SDL_PauseAudio(0);
        songindex = 0;
        lastbad = 0;
    } else {
        logger->note("Sound is disabled\n");
        this->nosound = true;
    }
    music = NULL;
    playingvqa = 0;
}

/** Destructor, closes the audio and frees some memory */
SoundEngine::~SoundEngine()
{
    if (!nosound) {
        SDL_CloseAudio();
        delete[] audio_chunk;
        if( music != NULL )
            delete music;
        while( !gamesounds.empty() ) {
            delete gamesounds[0];
            gamesounds.pop_front();
        }
        for (Uint16 i=0;i<songlist.size();++i)
            delete[] songlist[i];
    }
}

/** starts playing a song.
 * @param the name of the desired song.
 */
void SoundEngine::playSong(const char *sname )
{
    if (nosound)
        return;
    //    SDL_LockAudio();
    if( (music == NULL) && (lastbad < 5)) {
        try {
            /* the theme probably said "No theme" */
            if( !strcasecmp( sname, "No theme" ) ) {
                music = new Sound( songlist[songindex] );
            } else {
                music = new Sound(sname);
            }
            lastbad = 0;
        } catch (SoundError&) {
            music = NULL;
            ++lastbad;
            if (lastbad < 5) {
                if ( !strcasecmp( sname, "No theme" ) ) {
                    if ((++songindex) > (songlist.size()-1))
                        songindex = 0;
                    playSong(sname);
                }
            } else {
                logger->warning("last five entries were bad, disabling music\n");
            }
        }
    }
    //    SDL_UnlockAudio();
}

/** stop playing the song */
void SoundEngine::stopSong()
{
    if (nosound)
        return;
    SDL_LockAudio();
    if( music != NULL ) {
        delete music;
        music = NULL;
    }
    SDL_UnlockAudio();
}

/** add a ingame sound to the sound buffer/queue, will start playing next time
 * the buffer needs filling. */
void SoundEngine::queueSound(const char* sname)
{
    if (nosound)
        return;
    if (sname == NULL) {
        logger->warning("Attempted to queue a NULL sound\n");
        return;
    }
    Sound *gs;
    SDL_LockAudio();
    try {
        gs = new Sound(sname);
    } catch (SoundError&) {
        SDL_UnlockAudio();
        return;
    }
    gamesounds.push_back(gs);
    SDL_UnlockAudio();
}

/** Prepares the soundengine for playing the sound from a vqa movie
 * @param the initial buffer.
 * @param length of initial buffer.
 */
void SoundEngine::initVQAsound(Uint8 *buf, Uint16 len)
{
    if (nosound)
        return;
    /* add the first 1/2 sec or so of sound to the buffer but don't start
       playing it yet. */
    memcpy( audio_chunk, buf, len );
    audio_pos = audio_chunk;
    audio_len = 0;

    /* set up the initlen, the samplelen and create the semafore */
    vqainitlen = len;
    vqasamplelen = 0;
    playingvqa = 0;
    vqasem = SDL_CreateSemaphore(0);

    /* these two are used to make sure no audio is skipped and that all
       audio is added in the right place.*/
    lastposinvqastream = len;
    lastposinvqabuf = lastposinvqastream;
}

/** Add more sound from the vqa. The function uses a semafor so that it
 * won't return until the sound it added to the playing buffer. But not all 
 * the sound is added because of the initial sample.
 * @param pointer to the sound sample.
 * @param length of the sample.
 */
void SoundEngine::addVQAsound(Uint8 *buf, Uint16 len)
{
    if (nosound)
        return;

    /* reset the position we're adding to if the sample won't fit */
    if( (lastposinvqastream + len) > MAX_UNCOMPRESSED_SIZE ) {
        lastposinvqastream = 0;
    }
    /* add the sample and increase the lastposinstream and the lpibuf if
       needed. */
    memcpy(audio_chunk+lastposinvqastream, buf, len);
    vqasamplelen = len;
    lastposinvqastream += len;
    if( lastposinvqastream > lastposinvqabuf )
        lastposinvqabuf = lastposinvqastream;
    playingvqa = 1;
    /* wait 'til this has been added and then set the length to 0 so we don't
       add it again. */
    SDL_SemWait(vqasem);
    vqasamplelen = 0;
}

/** Stops the playback of vqa movie sound but first plays the initlen samples
 * we have not yet played.
 */
void SoundEngine::closeVQAsound()
{
    if (nosound)
        return;
    vqasamplelen = vqainitlen;
    SDL_SemWait(vqasem);
    playingvqa = 0;
    SDL_DestroySemaphore(vqasem);
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
    Uint16 i;

    if( playingvqa ) {
        /* if we have parts of a full sample left, move it to the begining of
           the buffer so we can add a full sample. */
        if( vqasamplelen == 0 )
            return;

        if( (audio_pos + audio_len + vqasamplelen) > (audio_chunk+lastposinvqabuf) ) {
            tmpbuf = new Uint8[lastposinvqastream+lastposinvqabuf-(audio_pos-audio_chunk)];
            memcpy( tmpbuf+lastposinvqabuf-(audio_pos-audio_chunk), audio_chunk, lastposinvqastream );
            memcpy( tmpbuf, audio_pos, lastposinvqabuf-(audio_pos-audio_chunk) );
            memcpy( audio_chunk, tmpbuf, lastposinvqastream+lastposinvqabuf-(audio_pos-audio_chunk) );
            delete[] tmpbuf;
            lastposinvqastream = lastposinvqabuf = lastposinvqastream+lastposinvqabuf-(audio_pos-audio_chunk);
            audio_pos = audio_chunk;
        }
        /* increase the audio_len, the data is already there, and release the
           semaphor. */

        audio_len += vqasamplelen;

        SDL_SemPost(vqasem);
    } else {
        tmpbuf = NULL;
        /* move eventual spare data to a temp buffer. */
        if( audio_len != 0 ) {
            tmpbuf = new Uint8[audio_len];
            memcpy( tmpbuf, audio_pos, audio_len );
        }

        /* set the intire buffer to 0 */
        audio_pos = audio_chunk;
        memset( audio_chunk, 0, MAX_UNCOMPRESSED_SIZE );

        /* move back the data we copied to the tempbuffer (if any) */
        if( tmpbuf != NULL ) {
            memcpy( audio_chunk, tmpbuf, audio_len );
            audio_pos = audio_chunk+audio_len;
            delete[] tmpbuf;
        }

        /* mix in the music if there is any */
        if( music != NULL ) {
            len = music->decodeSample(&sndptr);
            if( len == 0 ) {
                /* here we can switch song since the old is over =) */
                delete music;
                music = NULL;
                if ((++songindex) > (songlist.size()-1))
                    songindex = 0;
                playSong(songlist[songindex]);
            } else {
                slen = max(len, slen);
                SDL_MixAudio(audio_pos, sndptr, len, SDL_MIX_MAXVOLUME);
            }
        }

        /* if there are any ingamesounds, add them all */
        if( !gamesounds.empty() ) {
            for( i = 0; i < gamesounds.size(); i++ ) {
                if( gamesounds[i] != NULL ) {
                    len = gamesounds[i]->decodeSample(&sndptr);
                    /* set finished sounds to 0 so we don't start them over */
                    if( len == 0 ) {
                        delete gamesounds[i];
                        gamesounds[i] = NULL;
                    } else {
                        slen = max(len, slen);
                        SDL_MixAudio(audio_pos, sndptr, len, SDL_MIX_MAXVOLUME);
                    }
                }
            }

            /* remove all sonds in the front of the deque that are NULL
               and thus finished */
            while( !gamesounds.empty() && gamesounds[0] == NULL ) {
                gamesounds.pop_front();
            }
        }
        /* increase the audio_len */
        //      audio_pos = audio_chunk;
        audio_len += slen;
    }
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
    if (audio_len == 0)
        return;
    len = (len > (int)audio_len ? audio_len : len);
    /* mix in as much data as possible */
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_len -= len;
    audio_pos += len;
}

bool SoundEngine::createPlayList(const char* gamename)
{
    INIFile* songsini;
    char* tmp;
    char tmp2[8];
    Uint8 index;
    if (nosound)
        return true;
    try {
        songsini = new INIFile("songs.ini");
    } catch(runtime_error& e) {
        logger->warning("The inifile \"songs.ini\" was not found!\n");
        return false;
    }
    index = 0;
    sprintf(tmp2,"song%i",index);
    tmp = songsini->readString(gamename,tmp2);
    while (tmp != NULL) {
        songlist.push_back(tmp);
        ++index;
        sprintf(tmp2,"song%i",index);
        tmp = songsini->readString(gamename,tmp2);
    }
    delete songsini;
    if (index != 0) {
        return true;
    } else {
        return false;
    }
}
