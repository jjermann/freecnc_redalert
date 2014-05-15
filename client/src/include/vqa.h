// mode: -*- C++ -*-
#ifndef VQA_H
#define VQA_H

#include "SDL.h"

/* size of vqa header - so i know how many bytes to read in the file */
#define header_size sizeof(Uint8) * 28 + sizeof(Uint16) * 9 + sizeof(Uint32) * 2

struct VQAHeader
{
    Uint8 Signature[8]; /* Always "WVQAVQHD" */
    Uint32 RStartPos; /* Size of header minus Signature (always 42 bytes) big endian */
    Uint16 Version; /* VQA Version. 2 = C&C TD, C&C RA 3 = TS */
    Uint16 Flags;  /* VQA Flags. If bit 1 is set, vqa has sound, otherwise it does not */
    Uint16 NumFrames; /* Number of frames */
    Uint16 Width;  /* Width of each frame */
    Uint16 Height;  /* Height of each frame */
    Uint8 BlockW;  /* Width of each image block (usually 4) */
    Uint8 BlockH;  /* Height of each image block (usually 2) */
    Uint8 FrameRate; /* Number of frames per second? */
    Uint8 CBParts; /* Number of frames that use the same lookup table (always 8 in TD and RA) */
    Uint16 Colors;  /* Number of colors used in Palette */
    Uint16 MaxBlocks; /* Maximum number of image blocks?? */
    Uint16 Unknown1;
    Uint32 Unknown2;
    Uint16 Freq;  /* Sound frequency */
    Uint8 Channels; /* 1 = mono; 2 = stereo (TD and RA always 1) (TS is always 2) */
    Uint8 Bits;  /* 8 or 16 bit sound */
    Uint8 Unknown3[14];
};


class VQAError
    {}
;
class VFile;
class VQAMovie
{
public:
    VQAMovie(const char *filename);
    ~VQAMovie();
    void play();
private:
    VQAMovie();

    int DecodeFORMChunk(); /* Decodes FORM Chunk - This has to return true to continue */
    int DecodeFINFChunk(); /* This has to return true to continue */
    Uint32 DecodeSNDChunk(Uint8 *outbuf);
    int DecodeVQFRChunk(SDL_Surface *frame);
    inline void DecodeCBPChunk();
    inline void DecodeVPTChunk(Uint8 Compressed);
    inline void DecodeCBFChunk(Uint8 Compressed);
    inline void DecodeCPLChunk(SDL_Color *palette);
    inline void DecodeUnknownChunk();

    /* General VQA File Related variables */
    VFile *vqafile;
    VQAHeader header;
    /* VQA Video Related Variables */
    Uint32 CBPOffset;
    Uint16 CBPChunks;
    Uint8 *CBF_LookUp;
    Uint8 *CBP_LookUp;
    Uint8 *VPT_Table;
    Uint32 *offsets;
    Uint8 modifier;
    Uint32 lowoffset;
    /* VQA Sound Related Variables */
    Sint32 sndindex;
    Sint32 sndsample;

    int scaleVideo, videoScaleQuality;
};

/* Copied from XCC Mixer (xcc.ra2.mods) by Olaf van der Spek */

/* This will probably require slight modification to work on
 * big-endian machines
 */
#define VQA_HI_BYTE(x)  (x & 0xff000000) >> 24
const Uint32 vqa_t_mask = 0x00ffffff;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const Uint32 vqa_cbf_id = 0x464243;
const Uint32 vqa_cbp_id = 0x504243;
const Uint32 vqa_cpl_id = 0x4C5043;
const Uint32 vqa_snd_id = 0x444E53;
const Uint32 vqa_vpt_id = 0x545056;
const Uint32 vqa_vpr_id = 0x525056;
const Uint32 vqa_vqfl_id = 0x4C465156;
const Uint32 vqa_vqfr_id = 0x52465156;
#else
const Uint32 vqa_cbf_id = *(Uint32*)"CBF\0";
const Uint32 vqa_cbp_id = *(Uint32*)"CBP\0";
const Uint32 vqa_cpl_id = *(Uint32*)"CPL\0";
const Uint32 vqa_snd_id = *(Uint32*)"SND\0";
const Uint32 vqa_vpt_id = *(Uint32*)"VPT\0";
const Uint32 vqa_vpr_id = *(Uint32*)"VPR\0";
const Uint32 vqa_vqfl_id = *(Uint32*)"VQFL";
const Uint32 vqa_vqfr_id = *(Uint32*)"VQFR";
#endif

#endif
