#include <cctype>
#include "common.h"
#include "mixvfs.h"
#include "blowfish.h"
#include "ws-key.h"
#include "fcnc_endian.h"
#include "vfs.h"

// this is the coded needed to create a vfs plugin
#ifdef _WIN32
#define EXPORT __declspec( dllexport )
#else
#define EXPORT
#endif

extern "C" Archive EXPORT *loadArchive(VFSOPEN openf, VFSCLOSE closef)
{
    return new MIXFiles(openf, closef);
}

MIXFiles::MIXFiles(VFSOPEN openf, VFSCLOSE closef)
{
    vfopen = openf;
    vfclose = closef;
}

MIXFiles::~MIXFiles()
{
    unloadArchives();
}

bool MIXFiles::loadArchive(const char *fname)
{
    VFile *file;
    file = vfopen(fname);
    if( file == NULL ) {
        return false;
    }
    mixfiles.push_back(file);
    readMIXHeader(file);
    return true;
}

void MIXFiles::unloadArchives()
{
    Uint32 i;
    for( i = 0; i < mixfiles.size(); i++ ) {
        vfclose(mixfiles[i]);
    }
    mixfiles.resize(0);
    mixheaders.clear();
}

Uint32 MIXFiles::getFile(const char *fname)
{
    VFile *myvfile;
    std::map<Uint32, MIXEntry>::iterator epos;
    std::map<Uint32, openFile>::iterator of;
    openFile newFile;
    Uint32 id;
    id = calc_id(fname);
    epos = mixheaders.find(id);
    if( epos == mixheaders.end() ) {
        return (Uint32)-1;
    }
    myvfile = mixfiles[epos->second.filenum];

    newFile.id = id;
    newFile.pos = 0;

    do {
        of = openFiles.find(id++);
    } while( of != openFiles.end() );
    id--;

    openFiles[id] = newFile;

    return id;
}

void MIXFiles::releaseFile(Uint32 file)
{
    openFiles.erase(file);
}

/*
void MIXFiles::loadMix(FILE *file, Uint32 startpos, bool external) {
    Uint8 fileno;
    if( external ) {
        fileno = mixfiles.size();
        mixfiles.push_back(file);
    } else {
        for( fileno = 0; fileno < mixfiles.size(); fileno++ ) {
            if( file == mixfiles[fileno] ) {
                break;
            }
        }
    }
*/
/* here is where we read the header and put all crc's with a headerentry
   in the map mixheaders */
/*    readMIXHeader(file, startpos, fileno);
}
 
FILE *MIXFiles::getOffsetAndSize(char *fname, Uint32 *offset, Uint32 *size) {
    std::map<Uint32, MIXEntry>::iterator epos;
    Uint32 id;
    id = calc_id(fname);
    epos = mixheaders.find(id);
    if( epos == mixheaders.end() ) {
        return NULL;
    }
    (*offset) = epos->second.offset;
    (*size) = epos->second.size;
    return mixfiles[epos->second.filenum];
}*/

/** Function to calculate a idnumber from a filename
 * @param the filename
 * @return the id.
 */
Uint32 MIXFiles::calc_id(const char *fname)
{
    Uint32 calc;
    int i;
    char buffer[13];
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 tmpswap;
#endif

    for (i=0; *fname!='\0' && i<12; i++)
        buffer[i]=toupper(*(fname++));
    while(i<13)
        buffer[i++]=0;

    calc=0;
    for(i=0;buffer[i]!=0;i+=4) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        tmpswap = SDL_Swap32(*(long *)(buffer+i));
        calc=ROL(calc)+tmpswap;
#else

        calc=ROL(calc)+(*(long *)(buffer+i));
#endif

    }
    return calc;
}



/** Decodes RA/TS Style MIX headers. Assumes you have already checked if
 *  header is encrypted and that mix is seeked to the start of the WSKey
 *  
 * @param pointer to allocated header which to fill correctly
 * @param mod, if 1 decodeHeader will attempt to check whether 
 * mixfile is from Tiberian Sun
 * @return pointer to mix_record
 */
mix_record *MIXFiles::decodeHeader(VFile *mix, mix_header *d_header, int mod)
{
    Uint8 WSKey[80];        /* 80-byte Westwood key */
    Uint8 BFKey[56];        /* 56-byte blow fish key */
    Uint8 Block[8];         /* 8-byte block to store blowfish stuff in */
    Cblowfish bf;
    Uint8 *e;
    mix_record *d_mindex;
    //bool aligned = true;

    //fread(&WSKey, 80, 1, mix);
    mix->readByte(WSKey, 80);
    get_blowfish_key((const Uint8 *)&WSKey, (Uint8 *)&BFKey);
    bf.set_key((const Uint8 *)&BFKey, 56);
    //fread(&Block, 8, 1, mix);
    mix->readByte(Block, 8);
    bf.decipher(&Block, &Block, 8);

    /* Extract the header from Block */
    memcpy(&d_header->t.c_files, &Block[0], sizeof(Uint16));
    memcpy(&d_header->t.size, &Block[sizeof(Uint16)], sizeof(Uint32));
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    d_header->t.c_files = SDL_Swap32(d_header->t.c_files);
    d_header->t.size = SDL_Swap32(d_header->t.size);
#endif
    /* Decrypt all indexes */
    const int m_size = sizeof(mix_record) * d_header->t.c_files;
    const int m_f = m_size + 5 & ~7;
    d_mindex = new mix_record[d_header->t.c_files];
    e = new Uint8[m_f];
    //fread(e, m_f, 1, mix);
    mix->readByte(e, m_f);
    memcpy(d_mindex, &Block[6], 2);
    bf.decipher(e, e, m_f);
    memcpy(reinterpret_cast<Uint8 *>(d_mindex) + 2, e, m_size - 2);
    delete[] e;

    for (int i = 0; i < d_header->t.c_files; i++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        d_mindex[i].id = SDL_Swap32(d_mindex[i].id);
        d_mindex[i].offset = SDL_Swap32(d_mindex[i].offset);
        d_mindex[i].size = SDL_Swap32(d_mindex[i].size);
#endif
        /* if mod is set, lets try to find out if this is
        * ts or not */
        /*if (mod) {
         if (d_mindex[i].offset & 0xf) 
          aligned = false;
         if (d_mindex[i].id == TS_ID)
          game = game_ts;
        }
              */
        /* 92 = 4 byte flag + 6 byte header + 80 byte key + 2 bytes (?) */
        d_mindex[i].offset += 92 + m_f; /* re-center offset to be absolute offset */
    }
    /*
     if (aligned) game = game_ts;
    */
    return d_mindex;
}


/** read the mixheader */
void MIXFiles::readMIXHeader(VFile *mix)
{
    MIXEntry mentry;
    mix_header header;
    mix_record *m_index = NULL;
    game_t game;
    int i;

    /* Read header */
    //fseek(mix, startpos, SEEK_SET);
    //fread(&header, 6, 1, mix);
    mix->readByte((Uint8 *)&header, 6);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    header.flags = SDL_Swap32(header.flags);
#endif

    game = which_game(header.flags);
    if (game == game_ra) {
        //fseek(mix, -2, SEEK_CUR);
        mix->seekCur(-2);
        if (header.flags & mix_encrypted) {
            m_index = decodeHeader(mix, &header, 1);
        } else { /* mix is not encrypted */
            bool aligned = true;
            //fseek(mix, startpos+4, SEEK_SET); /* seek past flag */
            mix->seekSet(4);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

            //header.t.c_files = freadword(mix);
            mix->readWord(&header.t.c_files, 1);
            //header.t.size = freadlong(mix);
            mix->readDWord(&header.t.size, 1);
#else

            //fread(&header, 6, 1, mix);
            mix->readByte((Uint8 *)&header, 6);
#endif

            const int m_size = sizeof(mix_record) * header.t.c_files;
            m_index = new mix_record[header.t.c_files];
            //fread(reinterpret_cast<Uint8 *>(m_index), m_size, 1, mix);
            mix->readByte((Uint8 *)m_index, m_size);
            for (i = 0; i < header.t.c_files; i++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
                m_index[i].id = SDL_Swap32(m_index[i].id);
                m_index[i].size = SDL_Swap32(m_index[i].size);
                m_index[i].offset = SDL_Swap32(m_index[i].offset);
#endif

                if (m_index[i].offset & 0xf)
                    aligned = false;
                if (m_index[i].id == TS_ID)
                    game = game_ts;
                m_index[i].offset += 4 + sizeof(mix_header) + m_size;
            }
            if (aligned)
                game = game_ts;
        }
    } else if ( game == game_td ) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        //fseek(mix, startpos, SEEK_SET);
        mix->seekSet(0);
        //header.t.c_files = freadword(mix);
        mix->readWord(&header.t.c_files, 1);
        //header.t.size = freadlong(mix);
        mix->readDWord(&header.t.size, 1);
#endif

        const int m_size = sizeof(mix_record) * header.t.c_files;
        m_index = new mix_record[header.t.c_files];
        //fread(reinterpret_cast<Uint8 *>(m_index), m_size, 1, mix);
        mix->readByte((Uint8 *)m_index, m_size);
        for (i = 0; i < header.t.c_files; i++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            m_index[i].id = SDL_Swap32(m_index[i].id);
            m_index[i].offset = SDL_Swap32(m_index[i].offset);
            m_index[i].size = SDL_Swap32(m_index[i].size);
#endif
            /* 6 = 6 byte header - no other header/flags or keys in TD mixes */
            m_index[i].offset += 6 + m_size;
        }
    }
    for( i = 0; i < header.t.c_files; i++ ) {
        mentry.filenum = mixfiles.size()-1;
        mentry.offset = m_index[i].offset;
        mentry.size = m_index[i].size;
        mixheaders[m_index[i].id] = mentry;
    }
    delete[] m_index;
}

Uint32 MIXFiles::readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes)
{
    Uint32 numRead;
    Uint32 id, pos;
    MIXEntry me;

    id = openFiles[file].id;
    pos = openFiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numBytes, (me.size-pos));
    numRead = mixfiles[me.filenum]->readByte(databuf, numRead);
    openFiles[file].pos += numRead;
    return numRead;
}

Uint32 MIXFiles::readWord(Uint32 file, Uint16 *databuf, Uint32 numWords)
{
    Uint32 numRead;
    Uint32 id, pos;
    MIXEntry me;

    id = openFiles[file].id;
    pos = openFiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numWords, ((me.size-pos)>>1));
    numRead = mixfiles[me.filenum]->readWord(databuf, numRead);
    openFiles[file].pos += numRead<<1;
    return numRead;
}

Uint32 MIXFiles::readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees)
{
    Uint32 numRead;
    Uint32 id, pos;
    MIXEntry me;

    id = openFiles[file].id;
    pos = openFiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numThrees, ((me.size-pos)/3));
    numRead = mixfiles[me.filenum]->readThree(databuf, numRead);
    openFiles[file].pos += numRead*3;
    return numRead;
}

Uint32 MIXFiles::readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords)
{
    Uint32 numRead;
    Uint32 id, pos;
    MIXEntry me;

    id = openFiles[file].id;
    pos = openFiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(numDWords, ((me.size-pos)>>2));
    numRead = mixfiles[me.filenum]->readDWord(databuf, numRead);
    openFiles[file].pos += numRead<<2;
    return numRead;
}

char *MIXFiles::readLine(Uint32 file, char *databuf, Uint32 buflen)
{
    Uint32 numRead;
    Uint32 id, pos;
    MIXEntry me;
    char *retval;

    id = openFiles[file].id;
    pos = openFiles[file].pos;

    me = mixheaders[id];

    mixfiles[me.filenum]->seekSet(me.offset+pos);

    numRead = min(buflen-1, me.size-pos);
    if( numRead == 0 ) {
        return NULL;
    }
    retval = mixfiles[me.filenum]->getLine(databuf, numRead+1);
    openFiles[file].pos += strlen(databuf);
    return retval;
}

void MIXFiles::seekSet(Uint32 file, Uint32 pos)
{
    openFiles[file].pos = pos;
    if( openFiles[file].pos > mixheaders[openFiles[file].id].size ) {
        openFiles[file].pos = mixheaders[openFiles[file].id].size;
    }
    mixfiles[mixheaders[openFiles[file].id].filenum]->seekSet(openFiles[file].pos+mixheaders[openFiles[file].id].offset);
}

void MIXFiles::seekCur(Uint32 file, Sint32 pos)
{
    openFiles[file].pos += pos;
    if( openFiles[file].pos > mixheaders[openFiles[file].id].size ) {
        openFiles[file].pos = mixheaders[openFiles[file].id].size;
    }
    mixfiles[mixheaders[openFiles[file].id].filenum]->seekSet(openFiles[file].pos+mixheaders[openFiles[file].id].offset);
}

Uint32 MIXFiles::getSize(Uint32 file)
{
    return mixheaders[openFiles[file].id].size;
}

const char* MIXFiles::getPath(Uint32 file)
{
    return NULL;
}

