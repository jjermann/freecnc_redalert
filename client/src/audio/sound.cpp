#include "common.h"
#include "compression.h"
#include "logger.h"
#include "sound.h"
#include "soundengine.h"
#include "vfs.h"

/** Constructor, loads a sound file from a mix
 * and prepares to play it.
 * @param the name of the soundfile.
 */
Sound::Sound(const char *sndname)
{
    //Uint8 t;

    /* load from mix */
    sndfile = VFS_Open(sndname);
    if( sndfile == NULL ) {
        logger->warning("Unable to load soundfile: %s\n", sndname);
        throw SoundError(); /* throw an error if the file was not found */
    }

    // Note: Headers are identical for TS and TD auds
    sndfile->readWord(&header.frequency,1);
    sndfile->readDWord(&header.size,1);
    sndfile->readDWord(&header.outsize,1);
    sndfile->readByte(&header.flags,1);
    sndfile->readByte(&header.type,1);

    /* set sample and index to 0, these are used in the
     Decompression alogorithm */
    sample = 0;
    index = 0;
    /* check the compression type and throw an error if it's of
       unsuported or unknown type */
    switch (header.type) {
    case 1:
        //fprintf(stderr, "%s is a WW Compressed format.  Support that format is experimental.\n", sndname);
        wsadpcm = true;
        //throw SoundError();
        break;
    case 99: /* YAY! good value */
        wsadpcm = false;
        break;
    default:
        logger->warning("%s has a corrupt header.\n", sndname);
        throw SoundError();
        break;
    }
    /* start at offset 12, after the header */
    offset = 12;
    /* allocate space for the output buffer */
    outBuf = new Uint8[MAX_UNCOMPRESSED_SIZE];
}

/** Destructor, frees some memory */
Sound::~Sound()
{
    delete[] outBuf;
    VFS_Close(sndfile);
}

/** Decodes some of the sound and puts it in the output buffer
 * @param a pointer which will point to the output buffer.
 * @returns the length of the decoded sound.
 */
Uint16 Sound::decodeSample(Uint8 **data)
{
    int i;
    Uint16 size, outSize, len;
    Uint32 ID;
    Uint8 dataFile[MAX_CHUNK_SIZE];
    Uint8 tmpbuff[MAX_UNCOMPRESSED_SIZE];
    *data = outBuf;

    /* allocate some space we can read the compressed data to */
    len = 0;

    sndfile->seekSet(offset);

    /* decode 8 samples */
    for( i = 0; i < 8; i++ ) {
        /* abort if we don't have a whole header left */
        if( (offset+8) >= sndfile->fileSize() ) {
            return len;
        }

        /* read the size, output size and id */
        sndfile->readWord(&size, 1);
        sndfile->readWord(&outSize, 1);
        sndfile->readDWord(&ID, 1);
        /* abort if id was wrong */
        if( (ID & 0xDEAF) != 0xDEAF ) {
            logger->warning("wrong ID? %d (%x)\n",ID,ID);
            return len;
        }
        /* read all of the compressed data */
        sndfile->readByte( dataFile, size );
        /* decompress it and change to offset and length */
        if (wsadpcm) {
            Compression::WSADPCM_Decode(dataFile, tmpbuff, size, outSize);
            Convert11to22(tmpbuff,size);
            memcpy(outBuf+len,tmpbuff,outSize);
        } else {
            Compression::IMADecode(dataFile, outBuf + len, size, &index, &sample);
        }
        len += outSize;
        offset += size + 8;
    }
    return len;
}

void Sound::Convert11to22(Uint8* buf, Uint16 len)
{
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, AUDIO_U8, 1, 22050, AUDIO_S16, 1, 22050) < 0) {
        logger->warning("Could not build conversion filter\n");
        return;
    }
    cvt.buf = buf;
    cvt.len = len;
    if (SDL_ConvertAudio(&cvt) < 0) {
        logger->warning("Could not run conversion filter\n");
        return;
    }
}

