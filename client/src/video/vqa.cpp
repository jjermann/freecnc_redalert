#include <cctype>
#include "common.h"
#include "compression.h"
#include "fcnc_endian.h"
#include "graphicsengine.h"
#include "imageproc.h"
#include "inifile.h"
#include "logger.h"
#include "soundengine.h"
#include "vfs.h"
#include "vqa.h"

/** Constructor, loads file and prepares to play a vqa movie.
 * @param the graphicsengine.
 * @param the soundengine.
 * @param the name of the vqamovie.
 */
VQAMovie::VQAMovie(const char *filename)
{
    INIFile *inif;
    char *fname = new char[strlen(filename)+5];

    if( toupper(filename[0]) == 'X' )
        vqafile = NULL;
    else {
        strcpy( fname, filename );
        strcat( fname, ".VQA" );
        vqafile = VFS_Open(fname);
    }

    delete[] fname;
    if (vqafile == NULL) {
        throw VQAError();
    }
    // Get header information for the vqa.  If the header is corrupt, we can die now.
    vqafile->seekSet(0);
    if (DecodeFORMChunk() == 1) {
        logger->error("VQA: Invalid FORM chunk\n");
        throw VQAError();
    }

    offsets = new Uint32[header.NumFrames];
    if (DecodeFINFChunk() == 1) {
        delete[] offsets;
        logger->error("VQA: Invalid FINF chunk\n");
        throw VQAError();
    }

    CBF_LookUp = new Uint8[0x0ff00 << 3];
    CBP_LookUp = new Uint8[0x0ff00 << 3];
    VPT_Table = new Uint8[lowoffset<<1];
    CBPOffset = 0; /* Starting offset of CBP Look up table must be zero */
    CBPChunks = 0; /* Number of CBPChunks */

    // FIXME: Use global config data
    inif = new INIFile("freecnc.ini");
    scaleVideo = inif->readInt("video", "fullscreenMovies", 0);
    videoScaleQuality = inif->readInt("video", "movieQuality", 0);
    delete inif;
}

/** Destructor, free all memory used by the vqa. */
VQAMovie::~VQAMovie()
{
    delete[] CBF_LookUp;
    delete[] CBP_LookUp;
    delete[] VPT_Table;
    delete[] offsets;
    VFS_Close(vqafile);
}

/** Play the vqamovie */
void VQAMovie::play()
{
    SDL_Surface *frame, *cframe;
    SDL_Rect dest;
    SDL_Event esc_event;
    Uint8 *sndbuf;
    Uint16 sndlen;
    int i;
    static ImageProc scaler;

    if( vqafile == NULL )
        return;

    dest.w = header.Width<<1;
    dest.h = header.Height<<1;
    dest.x = (pc::gfxeng->getWidth()-(header.Width<<1))>>1;
    dest.y = (pc::gfxeng->getHeight()-(header.Height<<1))>>1;

    /* clear the screen */
    pc::gfxeng->clearScreen();

    /* Seek to first frame/snd information of vqa */
    vqafile->seekSet(offsets[0]);

    /* init the sound - if there is no sound, DecodeSNDChunk returns 0 for sndlen and we continue*/
    /* Is it safe to continue with sndlen set to zero? */
    sndindex = 0;
    sndsample = 0;
    sndbuf = new Uint8[MAX_UNCOMPRESSED_SIZE];
    memset(sndbuf, 0, MAX_UNCOMPRESSED_SIZE);
    sndlen = DecodeSNDChunk(sndbuf);
    pc::sfxeng->initVQAsound(sndbuf, sndlen);

    /* create the frame to store the image in. */
    frame = SDL_CreateRGBSurface(SDL_SWSURFACE, header.Width, header.Height, 8, 0, 0, 0, 0);
    /* Initialise the scaler */
    if( scaleVideo )
        scaler.initVideoScale(frame, videoScaleQuality);
    for( i = 0; i < header.NumFrames; i++ ) {
        /* decode SND Chunk first */
        sndlen = DecodeSNDChunk(sndbuf);

        if(DecodeVQFRChunk(frame) == 1) {
            /* Error decoding frame! We cannot continue, we must bail out */
            break;
        }

        /* add the sound, when this function returns it's time to add the picture */
        pc::sfxeng->addVQAsound(sndbuf, sndlen);

        /* draw the frame */
        if( scaleVideo ) {
            cframe = scaler.scaleVideo(frame);
            pc::gfxeng->drawVQAFrame(cframe);
        } else {
            pc::gfxeng->drawVQAFrame(frame);
        }
        //SDL_FreeSurface(cframe);

        while ( SDL_PollEvent(&esc_event) ) {
            if (esc_event.type == SDL_KEYDOWN) {
                if (esc_event.key.state != SDL_PRESSED)
                    break;
                if (esc_event.key.keysym.sym == SDLK_ESCAPE) {
                    i = header.NumFrames; /* set i high  to break for loop*/
                    break;
                }
            }
        } /* while */
    } /* for */


    pc::sfxeng->closeVQAsound();
    SDL_FreeSurface(frame);
    if( scaleVideo ) {
        scaler.closeVideoScale();
    }

    delete[] sndbuf;
}

/** Decodes "FORM" Chunk
 * @return 0 on success, 1 on failure
 */
int VQAMovie::DecodeFORMChunk()
{
    char ChunkID[4];

    vqafile->readByte((Uint8*)ChunkID, 4);

    if (strncmp(ChunkID, "FORM", 4)) {
        fprintf(stderr, "Error decoding FORM Chunk - Expected \"FORM\", got \"%c%c%c%c\"\n",
                ChunkID[0], ChunkID[1], ChunkID[2], ChunkID[3]);
        return 1;
    }

    /* skip chunklen */
    vqafile->seekCur(4);
#if SDL_BYTEORDER == SDL_LIL_ENDIAN

    vqafile->readByte(reinterpret_cast<Uint8 *>(&header), header_size);
#else

    vqafile->readByte((Uint8*)&header.Signature, sizeof(header.Signature));
    vqafile->readDWord(&header.RStartPos, 1);
    vqafile->readWord(&header.Version, 1);
    vqafile->readWord(&header.Flags, 1);
    vqafile->readWord(&header.NumFrames, 1);
    vqafile->readWord(&header.Width, 1);
    vqafile->readWord(&header.Height, 1);
    vqafile->readByte(&header.BlockW, 1);
    vqafile->readByte(&header.BlockH, 1);
    vqafile->readByte(&header.FrameRate, 1);
    vqafile->readByte(&header.CBParts, 1);
    vqafile->readWord(&header.Colors, 1);
    vqafile->readWord(&header.MaxBlocks, 1);
    vqafile->readWord(&header.Unknown1, 1);
    vqafile->readDWord(&header.Unknown2, 1);
    vqafile->readWord(&header.Freq, 1);
    vqafile->readByte(&header.Channels, 1);
    vqafile->readByte(&header.Bits, 1);
    vqafile->readByte((Uint8*)&header.Unknown3, sizeof(header.Unknown3));
#endif
    // Weird: need to byteswap on both BE and LE
    // readDWord probably swaps back on BE.
    header.RStartPos = SDL_Swap32(header.RStartPos);
    /* Check if header is valid */
    if (strncmp((const char*)header.Signature, "WVQAVQHD", 8) == 1 || header.Version != 2) {
        fprintf(stderr, "Invalid Header. Either signature is invalid or it is in a version that we do not support\n");
        return 1;
    }

    /* set some constants based on the header */
    lowoffset = (header.Width/header.BlockW)*(header.Height/header.BlockH);
    modifier = header.BlockH == 2 ? 0x0f : 0xff;
    return 0;
}

/** Decodes "FINF" Chunk
 * @param pointer to store the offsets in.
 * @return 0 on success, 1 on failure
 */
int VQAMovie::DecodeFINFChunk()
{
    Uint8 ChunkID[4];
    int i;

    vqafile->readByte(ChunkID, 4);

    if (strncmp((const char*)ChunkID, "FINF", 4)) {
        fprintf(stderr, "Error decoding FINF chunk - Expected \"FINF\", got \"%c%c%c%c\"\n",
                ChunkID[0], ChunkID[1], ChunkID[2], ChunkID[3]);
        return 1;
    }

    /* Skip chunk len. its not important yet? */
    vqafile->seekCur(4);

    vqafile->readByte((Uint8*)offsets, header.NumFrames<<2);
    for (i = 0; i < header.NumFrames; i++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        offsets[i] = SDL_Swap32(offsets[i]);
#endif

        offsets[i] &= 0x3FFFFFFF;
        offsets[i] <<= 1;
    }

    return 0;
}

/** Decodes SND Chunk
 * @param pointer to decoded chunk
 * @return length of chunk
 */
Uint32 VQAMovie::DecodeSNDChunk(Uint8 *outbuf)
{
    Uint32 ChunkLen;
    Uint32 ChunkID;
    Uint8 *inbuf;

    /* header Flags tells us that this VQA does not support sounds.. so lets quit */
    if (!(header.Flags & 1))
        return 0;

    /* seek to correct offset */
    if (vqafile->tell() & 1)
        vqafile->seekCur(1);

    vqafile->readDWord(&ChunkID, 1);
    if ((ChunkID & vqa_t_mask) != vqa_snd_id) {
        fprintf(stderr, "Error decoding SND chunk - Expected 0x%X, got 0x%X\n",
                vqa_snd_id, ChunkID & vqa_t_mask);
        return 0; /* Returning zero here, to set length of sound chunk to zero */
    }

    vqafile->readDWord( &ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);

    inbuf = new Uint8[ChunkLen];

    vqafile->readByte(inbuf, ChunkLen);

    switch (VQA_HI_BYTE(ChunkID)) {
    case '0': /* Raw uncompressed wave data */
        memcpy(outbuf, inbuf, ChunkLen);
        break;
    case '1': /* Westwoods own algorithm */
        /* TODO: Add support for this algorithm */
        fprintf(stderr, "Error decoding SND chunk - sound compressed using unsupported westwood algorithm\n");
        ChunkLen = 0;
        break;
    case '2': /* IMA ADPCM algorithm */
        Compression::IMADecode(inbuf, outbuf, ChunkLen, &sndindex, &sndsample);
        ChunkLen <<= 2; /* IMA ADPCM decompresses sound to a size 4 times larger than the compressed size */
        break;
    default:
        fprintf(stderr, "Error decoding SND chunk - sound in unknown format\n");
        ChunkLen = 0;
        break;
    }

    delete[] inbuf;
    return ChunkLen;
}

/** Decodes VQFR Chunk and gives you 1 decoded frame at end
 * @param pointer to decoded frame
 * @param Current Frame to decode
 * @return 0 success, 1 on failure 
 */
int VQAMovie::DecodeVQFRChunk(SDL_Surface *frame)
{
    Uint32 ChunkID;
    Uint32 ChunkLen;
    Uint8 HiVal, LoVal;
    Uint8 CmpCBP, compressed; /* Is CBP Look up table compressed or not */
    int cpixel, bx, by, fpixel;
    SDL_Color CPL[256];

    if (vqafile->tell() & 1)
        vqafile->seekCur(1);

    vqafile->readDWord(&ChunkID, 1);
    if (ChunkID != vqa_vqfr_id) {
        /* We cannot continue if VQFR is corrupted */
        fprintf(stderr, "Error Decoding VQFR Chunk - Expected 0x%X, got 0x%X\n",
                vqa_vqfr_id, ChunkID);
        return 1;
    }

    vqafile->readDWord( &ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);
    CmpCBP = 0;

    while (1) {
        if (vqafile->tell() & 1)
            vqafile->seekCur(1);

        vqafile->readDWord(&ChunkID, 1);
        compressed = VQA_HI_BYTE(ChunkID) == 'Z';
        ChunkID = ChunkID & vqa_t_mask;
        if (ChunkID == vqa_cpl_id) {
            DecodeCPLChunk(CPL);
            SDL_SetColors(frame, CPL, 0, header.Colors);
        } else if (ChunkID == vqa_cbf_id) {
            DecodeCBFChunk(compressed);
        } else if (ChunkID == vqa_cbp_id) {
            CmpCBP = compressed;
            DecodeCBPChunk();
        } else if (ChunkID == vqa_vpt_id) {
            /* This chunk is always the last one */
            DecodeVPTChunk(compressed);
            break;
        } else
            DecodeUnknownChunk();

    }
    /* Got all the chunks */

    /* This is all thats left that needs to be optimized if at all possible... */
    cpixel = 0;
    fpixel = 0;
    for( by = 0; by < header.Height; by += header.BlockH ) {
        for( bx = 0; bx < header.Width; bx += header.BlockW ) {
            LoVal = VPT_Table[fpixel]; /* formerly known as TopVal */
            HiVal = VPT_Table[fpixel + lowoffset]; /* formerly known as LowVal */
            if( HiVal == modifier ) {
                memset((Uint8 *)frame->pixels + cpixel, LoVal, header.BlockW);
                memset((Uint8 *)frame->pixels + cpixel + header.Width, LoVal, header.BlockW);
            } else {
                memcpy((Uint8 *)frame->pixels + cpixel, &CBF_LookUp[((HiVal<<8)|LoVal)<<3], header.BlockW);
                memcpy((Uint8 *)frame->pixels + cpixel + header.Width,
                       &CBF_LookUp[(((HiVal<<8)|LoVal)<<3)+4], header.BlockW);
            }
            cpixel += header.BlockW;
            fpixel++;
        }
        cpixel += header.Width;
    }

    //    if( !((CurFrame + 1) % header.CBParts) ) {
    if (CBPChunks & ~7) {
        if (CmpCBP == 1) {
            Uint8 CBPUNZ[0x0ff00 << 3];
            Compression::decode80(CBP_LookUp, CBPUNZ);
            memcpy(CBF_LookUp, CBPUNZ, 0x0ff00 << 3);
        } else
            memcpy(CBF_LookUp, CBP_LookUp, 0x0ff00 << 3);
        CBPOffset = 0;
        CBPChunks = 0;
    }
    return 0;
}

inline void VQAMovie::DecodeCBPChunk()
{
    Uint32 ChunkLen;

    vqafile->readDWord(&ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);

    vqafile->readByte(CBP_LookUp + CBPOffset, ChunkLen);
    CBPOffset += ChunkLen;
    CBPChunks++;
}

inline void VQAMovie::DecodeVPTChunk(Uint8 Compressed)
{
    Uint32 ChunkLen;

    vqafile->readDWord( &ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);

    if (Compressed) {
        Uint8 *VPTZ; /* Compressed VPT_Table */
        VPTZ = new Uint8[ChunkLen];
        vqafile->readByte(VPTZ, ChunkLen);
        Compression::decode80(VPTZ, VPT_Table);
        delete[] VPTZ;
    } else { /* uncompressed VPT chunk. never found any.. but might be some */
        vqafile->readByte(VPT_Table, ChunkLen);
    }
}

inline void VQAMovie::DecodeCBFChunk(Uint8 Compressed)
{
    Uint32 ChunkLen;

    vqafile->readDWord(&ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);

    if (Compressed) {
        Uint8 *CBFZ; /* Compressed CBF table */
        CBFZ = new Uint8[ChunkLen];
        vqafile->readByte(CBFZ, ChunkLen);
        Compression::decode80(CBFZ, CBF_LookUp);
        delete[] CBFZ;
    } else {
        vqafile->readByte(CBF_LookUp, ChunkLen);
    }
}

inline void VQAMovie::DecodeCPLChunk(SDL_Color *palette)
{
    Uint32 ChunkLen;
    int i;

    vqafile->readDWord(&ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);

    for (i = 0; i < header.Colors; i++) {
        vqafile->readByte((Uint8*)&palette[i], 3);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }
}

inline void VQAMovie::DecodeUnknownChunk()
{
    Uint32 ChunkLen;

    vqafile->readDWord(&ChunkLen, 1);
    ChunkLen = SDL_Swap32(ChunkLen);

    vqafile->seekCur(ChunkLen);
}
