// mode: -*- C++ -*-
/** Various decompression routines */
#ifndef COMPRESSION_H
#define COMPRESSION_H

class Compression
{
public:
    static void IMADecode( Uint8 *InBuf, Uint8 *OutBuf, Uint16 Size, Sint32 *index, Sint32 *sample);
    static void WSADPCM_Decode(Uint8 *InBuf, Uint8 *OutBuf, Uint16 Size, Uint16 OutSize);
    static int decode80(const Uint8 image_in[], Uint8 image_out[]);
    static int decode40(const Uint8 image_in[], Uint8 image_out[]);
    static int decode20(const Uint8* s, Uint8* d, int cb_s);
    static int dec_base64(const Uint8* src, Uint8* target, size_t length);
    static Uint8 Clip8BitSample(Sint16 sample);
private:
    static const int Steps[89];/* = {
                                                  7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,
                                                    34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,
                                                    157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,
                                                    724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,
                                                    2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,
                                                    9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,
                                                    27086,29794,32767 };
                                               */
    static const int Indexes[8];/* = {-1,-1,-1,-1,2,4,6,8};*/
    static const int WSTable2bit[4];
    static const int WSTable4bit[16];
};

#endif
