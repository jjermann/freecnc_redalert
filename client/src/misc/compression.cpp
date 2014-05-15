#include <cstring>
#include <cctype>
#include "SDL_types.h"
#include "SDL_endian.h"
#include "compression.h"
#include "logger.h"

static const char Base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

const int Compression::Steps[89]=
    {
        7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,
        34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,
        157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,
        724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,
        2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,
        9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
        27086,29794,32767
    };
const int Compression::Indexes[8] =
    {
        -1,-1,-1,-1,2,4,6,8
    };

/* Decode Westwood's ADPCM format.  Original code from ws-aud.txt by Asatur V. Nazarian */
#define HIBYTE(word) ((word) >> 8)
#define LOBYTE(word) ((word) & 0xFF)

const int Compression::WSTable2bit[4] =
    {
        -2,-1,0,1
    };
const int Compression::WSTable4bit[16] =
    {
        -9, -8, -6, -5, -4, -3, -2, -1,
        0,  1,  2,  3,  4,  5,  6,  8
    };

/** Decodes IMA-ADPCM Sample
 * @param pointer to compressed sample
 * @param pointer to malloc'd buffer ready to accept decompressed sample
 * @param size in bytes of compressed sample
 * @param current index.
 * @param current samplevalue.
 */
void Compression::IMADecode(Uint8 *InBuf, Uint8 *OutBuf, Uint16 Size, Sint32 *index, Sint32 *sample)
{
    int  Samples;
    Uint8 Code;
    int  Sign;
    int  Delta;
    Uint8 *InP;
    Sint16 *OutP;
    Uint16 decSize = Size<<1;

    if (Size==0)
        return;

    InP=(Uint8 *)InBuf;
    OutP=(Sint16 *)OutBuf;

    for (Samples=0; Samples<decSize; Samples++) {
        if (Samples&1)          // If Samples is odd
            Code=(*InP++)>>4;    // Extract upper 4 bits
        else                         // Samples is even
            Code=(*InP) & 0x0F;  // Extract lower 4 bits

        Sign=(Code & 0x08)!=0;  // If topmost bit is set, Sign=true
        Code&=0x07;             // Keep lower 3 bits

        Delta=0;
        if ((Code & 0x04)!=0)
            Delta+=Steps[*index];
        if ((Code & 0x02)!=0)
            Delta+=Steps[*index]>>1;
        if ((Code & 0x01)!=0)
            Delta+=Steps[*index]>>2;
        Delta+=Steps[*index]>>3;

        if (Sign)
            Delta=-Delta;

        *sample+=Delta;

        if (*sample>32767)
            *sample=32767;
        else if (*sample<-32768)
            *sample=-32768;

        *OutP++=(Sint16)*sample;

        *index+=Indexes[Code];
        if (*index<0)
            *index=0;
        else if (*index>88)
            *index=88;
    }
}

/* Decode Westwood's ADPCM format.  Original code from ws-aud.txt by Asatur V. Nazarian */

Uint8 Compression::Clip8BitSample(Sint16 sample)
{
    if (sample > 255)
        return 255;
    else if (sample < 0)
        return 0;
    else
        return (unsigned char)sample;
}

void Compression::WSADPCM_Decode(Uint8 *InBuf, Uint8 *OutBuf, Uint16 Size, Uint16 OutSize)
{
    Sint16 CurSample;
    Uint8  code;
    Sint8  count; // this is a signed char!
    Uint16  i; // index into InputBuffer
    Uint16  input; // shifted input

    if (Size==OutSize) // such chunks are NOT compressed
    {
        for (i=0;i<OutSize;i++)
            OutBuf[i]=InBuf[i]; // send to output stream
        return; // chunk is done!
    }

    // otherwise we need to decompress chunk





    CurSample=0x80; // unsigned 8-bit
    i=0;

    // note that OutSize value is crucial for decompression!

    while (OutSize>0) // until OutSize is exhausted!
    {
        input=InBuf[i++];
        input<<=2;
        code=HIBYTE(input);
        count=LOBYTE(input)>>2;
        switch (code) // parse code
        {
        case 2: // no compression...
            if (count & 0x20)
            {
                count<<=3;  // here it's significant that (count) is signed:
                CurSample+=count>>3; // the sign bit will be copied by these shifts!
                *OutBuf++ = Clip8BitSample(CurSample);
                OutSize--; // one byte added to output
            } else // copy (count+1) bytes from input to output
            {
                for (count++;count>0;count--,OutSize--,i++)
                    *OutBuf++ = (InBuf[i]);
                CurSample=InBuf[i-1]; // set (CurSample) to the last byte sent to output
            }
            break;
        case 1: // ADPCM 8-bit -> 4-bit
            for (count++;count>0;count--) // decode (count+1) bytes
            {
                code=InBuf[i++];
                CurSample+=WSTable4bit[(code & 0x0F)]; // lower nibble
                *OutBuf++ = Clip8BitSample(CurSample);
                CurSample+=WSTable4bit[(code >> 4)]; // higher nibble
                *OutBuf++ = Clip8BitSample(CurSample);
                OutSize-=2; // two bytes added to output
            }
            break;
        case 0: // ADPCM 8-bit -> 2-bit
            for (count++;count>0;count--) // decode (count+1) bytes
            {
                code=InBuf[i++];
                CurSample+=WSTable2bit[(code & 0x03)]; // lower 2 bits
                *OutBuf++ = Clip8BitSample(CurSample);
                CurSample+=WSTable2bit[((code>>2) & 0x03)]; // lower middle 2 bits
                *OutBuf++ = Clip8BitSample(CurSample);
                CurSample+=WSTable2bit[((code>>4) & 0x03)]; // higher middle 2 bits
                *OutBuf++ = Clip8BitSample(CurSample);
                CurSample+=WSTable2bit[((code>>6) & 0x03)]; // higher 2 bits
                *OutBuf++ = Clip8BitSample(CurSample);
                OutSize-=4; // 4 bytes sent to output
            }
            break;
        default: // just copy (CurSample) (count+1) times to output
            for (count++;count>0;count--,OutSize--)
                *OutBuf++ = Clip8BitSample(CurSample);
        }
    }
}
/*****************************************************************************
 * decode.c - Decoding routines for format80, format40, and format20 type
 *       graphics
 * Author: Olaf van der spek
 * Modified for FreeCNC by Kareem Dana
 ****************************************************************************/

/** decompress format 80 compressed data.
 * @param compressed data.
 * @param pointer to output uncompressed data.
 * @returns size of uncompressed data.
 */
int Compression::decode80(const Uint8 image_in[], Uint8 image_out[])
{
    /*
    0 copy 0cccpppp p
    1 copy 10cccccc
    2 copy 11cccccc p p
    3 fill 11111110 c c v
    4 copy 11111111 c c p p
    */

    const Uint8* copyp;
    const Uint8* readp = image_in;
    Uint8* writep = image_out;
    Uint32 code;
    Uint32 count;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint16 bigend; /* temporary big endian var */
#endif

    while (1) {
        code = *readp++;
        if (~code & 0x80) {
            //bit 7 = 0
            //command 0 (0cccpppp p): copy
            count = (code >> 4) + 3;
            copyp = writep - (((code & 0xf) << 8) + *readp++);
            while (count--)
                *writep++ = *copyp++;
        } else {
            //bit 7 = 1
            count = code & 0x3f;
            if (~code & 0x40) {
                //bit 6 = 0
                if (!count)
                    //end of image
                    break;
                //command 1 (10cccccc): copy
                while (count--)
                    *writep++ = *readp++;
            } else {
                //bit 6 = 1
                if (count < 0x3e) {
                    //command 2 (11cccccc p p): copy
                    count += 3;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

                    memcpy(&bigend, readp, 2);
                    copyp = &image_out[SDL_Swap16(bigend)];
#else

                    copyp = &image_out[*(Uint16*)readp];
#endif

                    readp += 2;
                    while (count--)
                        *writep++ = *copyp++;
                } else if (count == 0x3e) {
                    //command 3 (11111110 c c v): fill
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                    memset(&count, 0, sizeof(Uint32));
                    memcpy(&count, readp, 2);
                    count = SDL_Swap32(count);
#else

                    count = *(Uint16*)readp;
#endif

                    readp += 2;
                    code = *readp++;
                    while (count--)
                        *writep++ = code;
                } else {
                    //command 4 (copy 11111111 c c p p): copy
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                    memset(&count, 0, sizeof(Uint32));
                    memcpy(&count, readp, 2);
                    count = SDL_Swap32(count);
#else

                    count = *(Uint16*)readp;
#endif

                    readp += 2;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

                    memcpy(&bigend, readp, 2);
                    copyp = &image_out[SDL_Swap16(bigend)];
#else

                    copyp = &image_out[*(Uint16*)readp];
#endif

                    readp += 2;
                    while (count--)
                        *writep++ = *copyp++;
                }
            }
        }
    }

    return (writep - image_out);
}

/** decompress format 40 compressed data.
 * @param compressed data.
 * @param pointer to pu uncompressed data in.
 * @returns size of uncompressed data.
 */
int Compression::decode40(const Uint8 image_in[], Uint8 image_out[])
{
    /*
    0 fill 00000000 c v
    1 copy 0ccccccc
    2 skip 10000000 c 0ccccccc
    3 copy 10000000 c 10cccccc
    4 fill 10000000 c 11cccccc v
    5 skip 1ccccccc
    */

    const Uint8* readp = image_in;
    Uint8* writep = image_out;
    Uint32 code;
    Uint32 count;

    while (1) {
        code = *readp++;
        if (~code & 0x80) {
            //bit 7 = 0
            if (!code) {
                //command 0 (00000000 c v): fill
                count = *readp++;
                code = *readp++;
                while (count--)
                    *writep++ ^= code;
            } else {
                //command 1 (0ccccccc): copy
                count = code;
                while (count--)
                    *writep++ ^= *readp++;
            }

        } else {
            //bit 7 = 1
            if (!(count = code & 0x7f)) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                memset(&count, 0, sizeof(Uint32));
                memcpy(&count, readp, 2);
                count = SDL_Swap32(count);
#else

                count = *(Uint16*)readp;
#endif

                readp += 2;
                code = count >> 8;
                if (~code & 0x80) {
                    //bit 7 = 0
                    //command 2 (10000000 c 0ccccccc): skip
                    if (!count)
                        // end of image
                        break;
                    writep += count;
                } else {
                    //bit 7 = 1
                    count &= 0x3fff;
                    if (~code & 0x40) {
                        //bit 6 = 0
                        //command 3 (10000000 c 10cccccc): copy
                        while (count--)
                            *writep++ ^= *readp++;
                    } else {
                        //bit 6 = 1
                        //command 4 (10000000 c 11cccccc v): fill
                        code = *readp++;
                        while (count--)
                            *writep++ ^= code;
                    }
                }
            } else {
                //command 5 (1ccccccc): skip
                writep += count;
            }
        }
    }
    return (writep - image_out);
}

/** decompress format 20 compressed data.
 * @param compressed data.
 * @param pointer to pu uncompressed data in.
 * @param size of compressed data?
 * @returns size of uncompressed data?
 */
int Compression::decode20(const Uint8* s, Uint8* d, int cb_s)
{
    const Uint8* r = s;
    const Uint8* r_end = s + cb_s;
    Uint8* w = d;
    while (r < r_end) {
        int v = *r++;
        if (v)
            *w++ = v;
        else {
            v = *r++;
            memset(w, 0, v);
            w += v;
        }
    }
    return w - d;

}

/** decodes base64 data
 *  @param compressed data
 *  @param pointer to put uncompressed data
 *  @param size of compressed data
 *  @returns -1 if error
 */
int Compression::dec_base64(const unsigned char* src, unsigned char* target, size_t length)
{
    int i;
    unsigned char a, b, c, d;
    static unsigned char dtable[256];
    int bits_to_skip = 0;

    for( i = length-1; src[i] == '='; i-- ) {
        bits_to_skip += 2;
        length--;
    }
    if( bits_to_skip >= 6 ) {
        logger->warning("Error in base64 (too many '=')\n");
        return -1;
    }

    for(i= 0;i<255;i++) {
        dtable[i]= 0x80;
    }
    for(i= 'A';i<='Z';i++) {
        dtable[i]= i-'A';
    }
    for(i= 'a';i<='z';i++) {
        dtable[i]= 26+(i-'a');
    }
    for(i= '0';i<='9';i++) {
        dtable[i]= 52+(i-'0');
    }
    dtable[(Uint8)'+']= 62;
    dtable[(Uint8)'/']= 63;
    dtable[(Uint8)'=']= 0;


    while (length >= 4) {
        a = dtable[src[0]];
        b = dtable[src[1]];
        c = dtable[src[2]];
        d = dtable[src[3]];
        if( a == 0x80 || b == 0x80 ||
                c == 0x80 || d == 0x80 ) {
            logger->warning("Illegal character\n");
        }
        target[0] = a << 2 | b >> 4;
        target[1] = b << 4 | c >> 2;
        target[2] = c << 6 | d;
        target+=3;
        length-=4;
        src += 4;
    }
    if( length > 0 ) {
        if( bits_to_skip == 4 && length == 2 ) {
            a = dtable[src[0]];
            b = dtable[src[1]];

            target[0] = a << 2 | b >> 4;
        } else if( bits_to_skip == 2 && length == 3 ) {
            a = dtable[src[0]];
            b = dtable[src[1]];
            c = dtable[src[2]];

            target[0] = a << 2 | b >> 4;
            target[1] = b << 4 | c >> 2;
        } else {
            logger->warning("Error in base64. #bits to skip doesn't match length\n");
            logger->warning("skip %d bits, %d chars left\n\"%s\"\n", bits_to_skip, (Uint32)length, src);
            return -1;
        }
    }

    return 0;
}

