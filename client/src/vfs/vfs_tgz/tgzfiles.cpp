#include "tgzfiles.h"

#ifdef _WIN32
#define EXPORT __declspec( dllexport )
#else
#define EXPORT
#endif

#ifdef _WIN32
#define EXPORT __declspec( dllexport )
#else
#define EXPORT
#endif

#ifdef NOZLIB

#include "stub_archive.h"

extern "C" Archive EXPORT *loadArchive(VFSOPEN openf, VFSCLOSE closef)
{
    return new StubArchive(0, 0);
}

#else /* NOZLIB */

#include "vfs.h"
#include <stdio.h>

extern "C" Archive EXPORT *loadArchive(VFSOPEN openf, VFSCLOSE closef)
{
    return new TGZFiles(openf, closef);
}

/* Values used in typeflag field.  */

#define REGTYPE  '0'  /* regular file */
#define AREGTYPE '\0'  /* regular file */
#define LNKTYPE  '1'  /* link */
#define SYMTYPE  '2'  /* reserved */
#define CHRTYPE  '3'  /* character special */
#define BLKTYPE  '4'  /* block special */
#define DIRTYPE  '5'  /* directory */
#define FIFOTYPE '6'  /* FIFO special */
#define CONTTYPE '7'  /* reserved */

#define BLOCKSIZE 512

struct tar_header
{    /* byte offset */
    char name[100];  /*   0 */
    char mode[8];   /* 100 */
    char uid[8];   /* 108 */
    char gid[8];   /* 116 */
    char size[12];  /* 124 */
    char mtime[12];  /* 136 */
    char chksum[8];  /* 148 */
    char typeflag;  /* 156 */
    char linkname[100];  /* 157 */
    char magic[6];  /* 257 */
    char version[2];  /* 263 */
    char uname[32];  /* 265 */
    char gname[32];  /* 297 */
    char devmajor[8];  /* 329 */
    char devminor[8];  /* 337 */
    char prefix[155];  /* 345 */
    /* 500 */
};

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::map;
using std::string;
#endif

TGZFiles::TGZFiles(VFSOPEN openf, VFSCLOSE closef)
{
    vfopen = openf;
    vfclose = closef;
}

TGZFiles::~TGZFiles()
{
    unloadArchives();
}

bool TGZFiles::loadArchive(const char *fname)
{
    VFile *file;
    //FILE *f;
    gzFile gf;
    file = vfopen(fname);
    if( file == NULL ) {
        return false;
    }
    gf = gzopen(file->getPath()/*dup(fileno(f))*/, "rb");
    vfclose(file);

    if( gf == NULL ) {
        return false;
    }
    archList.push_back(gf);
    readHeader(gf);
    return true;
}

void TGZFiles::unloadArchives()
{
    unsigned int i;
    for( i = 0; i < archList.size(); i++ ) {
        gzclose(archList[i]);
    }
    archList.resize(0);
    fileList.clear();
    openFiles.clear();
}

Uint32 TGZFiles::getFile(const char *fname)
{
    map<string, tgzfile>::iterator file;
    openFile newfile;
    Uint32 filenum;

    newfile.pos = 0;

    file = fileList.find((string)fname);
    if( file == fileList.end() ) {
        return (Uint32)-1;
    }

    newfile.file = &file->second;

    filenum = (Uint32)rand();
    while( openFiles.find(filenum) != openFiles.end() ) {
        filenum++;
    }

    openFiles[filenum] = newfile;

    return filenum;
}

void TGZFiles::releaseFile(Uint32 file)
{
    openFiles.erase(file);
}

int TGZFiles::getSize(char *sizestr, int len)
{
    int result = 0;
    char c;

    while(len--) {
        c = *sizestr++;
        if( c == ' ' )
            continue;
        if( c == 0 )
            break;
        result = result * 8 + (c - '0');
    }
    return result;
}

void TGZFiles::readHeader(gzFile gf)
{
    Uint8 databuf[BLOCKSIZE];
    Uint32 bRead, curpos;
    int fsize;
    //int i;
    //FILE *tgzlog;
    tgzfile curfile;
    gzseek(gf, 0, SEEK_SET);

    //tgzlog = fopen("tgz.log", "w");

    //fprintf(tgzlog, "Reading headers\n");
    curpos = 0;
    while( (bRead = gzread(gf, databuf, BLOCKSIZE)) != 0 ) {
        //fprintf(tgzlog, "Reading h1\n");
        if( bRead < BLOCKSIZE ) {
            // this chunk is incomplete
            //fprintf(tgzlog, "Incomplete chunk\n");
            //fclose(tgzlog);
            return;
        }
        curpos += bRead;
        /* add the info in this header to the file list */
        //sscanf(((struct tar_header *)databuf)->size, "%d", &fsize);
        fsize = getSize(((struct tar_header *)databuf)->size, 12);
        switch( ((struct tar_header *)databuf)->typeflag ) {
        case REGTYPE:
        case AREGTYPE:
            curfile.filenum = archList.size()-1;
            curfile.offset = curpos;
            curfile.size = fsize;
            /*for(i = 0;((struct tar_header *)databuf)->name[i] != '\0'; i++ ){
            ((struct tar_header *)databuf)->name[i] = toupper(((struct tar_header *)databuf)->name[i]);
            }*/
            if( fsize > 0 )
            {
                //fprintf(tgzlog, "trying to add file: %s (%d bytes) to tgzlist\n", ((struct tar_header *)databuf)->name, fsize);
                fileList[(string)((struct tar_header *)databuf)->name] = curfile;
            }
            break;
        default:
            //fprintf(tgzlog, "skipping file: %s (%d bytes) to tgzlist\n", ((struct tar_header *)databuf)->name, fsize);
            break;
        }
        /* skip the filedata itself */
        bRead = BLOCKSIZE*((fsize+BLOCKSIZE-1)/BLOCKSIZE);
        gzseek(gf, bRead, SEEK_CUR);
        curpos += bRead;
    }
    //fprintf(tgzlog, "Headers read\n");
    //fclose(tgzlog);
}

Uint32 TGZFiles::readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes)
{
    Uint32 numRead;
    tgzfile *f;
    f = openFiles[file].file;
    gzseek(archList[f->filenum], f->offset+openFiles[file].pos, SEEK_SET);

    numRead = min(numBytes, f->size-openFiles[file].pos);
    /*if( openFiles[file].pos + numBytes > f->size ){
    numBytes = f->size-openFiles[file].pos;
    }*/
    numRead = gzread(archList[f->filenum], databuf, numRead);
    openFiles[file].pos += numRead;
    return numRead;
}

Uint32 TGZFiles::readWord(Uint32 file, Uint16 *databuf, Uint32 numWords)
{
    Uint32 numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
#endif

    tgzfile *f;
    f = openFiles[file].file;
    gzseek(archList[f->filenum], f->offset+openFiles[file].pos, SEEK_SET);

    numRead = min(numWords<<1, f->size-openFiles[file].pos);
    /*if( openFiles[file].pos + (numWords<<1) > f->size ){
    numWords = (f->size-openFiles[file].pos)>>1;
    }*/

    numRead = gzread(archList[f->filenum], databuf, numRead);

    numRead &= ~1;
    openFiles[file].pos += numRead;
    numRead >>= 1;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap16(databuf[i]);
    }
#endif

    return numRead;
}

Uint32 TGZFiles::readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees)
{
    Uint32 numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
#endif

    tgzfile *f;
    f = openFiles[file].file;
    gzseek(archList[f->filenum], f->offset+openFiles[file].pos, SEEK_SET);
    numRead = min(numThrees*3, f->size-openFiles[file].pos);
    /*if( openFiles[file].pos + numThrees*3 > f->size ){
    numThrees = (f->size-openFiles[file].pos)/3;
    }*/

    numRead = gzread(archList[f->filenum], databuf, numRead);

    while( numRead%3 != 0 ) {
        numRead--;
    }
    openFiles[file].pos += numRead;
    numRead /= 3;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap32(databuf[i]);
        databuf[i]<<=8;
    }
#endif

    return numRead;
}

Uint32 TGZFiles::readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords)
{
    Uint32 numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
#endif

    tgzfile *f;
    f = openFiles[file].file;
    gzseek(archList[f->filenum], f->offset+openFiles[file].pos, SEEK_SET);
    numRead = min(numDWords<<2, f->size-openFiles[file].pos);
    /*if( openFiles[file].pos + (numDWords<<2) > f->size ){
    numDWords = (f->size-openFiles[file].pos)>>2;
    }*/

    numRead = gzread(archList[f->filenum], databuf, numRead);

    numRead &= ~3;
    openFiles[file].pos += numRead;
    numRead >>= 2;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap32(databuf[i]);
    }
#endif

    return numRead;
}

char *TGZFiles::readLine(Uint32 file, char *databuf, Uint32 buflen)
{
    Uint32 numRead;
    char *rval;
    tgzfile *f;


    f = openFiles[file].file;
    gzseek(archList[f->filenum], f->offset+openFiles[file].pos, SEEK_SET);

    numRead = min(buflen-1, f->size-openFiles[file].pos);
    if( numRead == 0 ) {
        return NULL;
    }
    /*if( openFiles[file].pos + buflen > f->size ){
    buflen = (f->size-openFiles[file].pos);
    }*/

    rval = gzgets(archList[f->filenum], databuf, numRead+1);

    openFiles[file].pos += strlen(databuf);
    return rval;
    //return fgets(databuf, buflen, openFiles[file].file);
}



void TGZFiles::seekSet(Uint32 file, Uint32 pos)
{
    openFiles[file].pos = pos;

}

void TGZFiles::seekCur(Uint32 file, Sint32 pos)
{
    openFiles[file].pos += pos;
}

Uint32 TGZFiles::getPos(Uint32 file)
{
    return openFiles[file].pos;
}

Uint32 TGZFiles::getSize(Uint32 file)
{
    return 0;
}

const char* TGZFiles::getPath(Uint32 file)
{
    return NULL;
}

#endif /* NOZLIB */
