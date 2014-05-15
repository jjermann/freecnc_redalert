#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include <cctype>
#include <string>
#include <errno.h>
#include "SDL_endian.h"
#include "externalvfs.h"
#include "common.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::string;
#endif

static FILE* fcaseopen(string* path, const char* mode, bool* isdir = NULL, Uint32 caseoffset = 0);

ExternalFiles::ExternalFiles(const char *defpath) : defpath(defpath)
{
}

bool ExternalFiles::loadArchive(const char *fname)
{
    FILE* tmp;
    string pth;
    bool dir = false;
    if (isRelativePath(fname)) {
        pth = defpath + "/" + fname;
    } else {
        pth = fname;
    }
    tmp = fcaseopen(&pth, "rb", &dir);
    if (!dir) {
        if (NULL == tmp)
            return false;
        fclose(tmp);
    }
    path.push_back(fname);
    return true;
}

Uint32 ExternalFiles::getFile(const char *fname, const char* mode)
{
    openFileExt newFile;
    FILE *f;
    Uint32 i;
    string filename;
    Uint32 size, fnum;

    if (mode[0] != 'r') {
        filename = fname;
        f = fopen(filename.c_str(), mode);
        if (f != NULL) {
            newFile.file = f;
            // We'll just ignore file sizes for files being written for now.
            newFile.size = 0;
            newFile.path = filename;
            fnum = (Uint32)f;
            openFiles[fnum] = newFile;
            return fnum;
        } // Error condition hanled at end of function
    }
    for (i = 0; i < path.size(); ++i) {
        filename = path[i] + fname;
        f = fcaseopen(&filename, mode, NULL, path[i].length());
        if (f != NULL) {
            fseek(f, 0, SEEK_END);
            size = ftell(f);
            fseek(f, 0, SEEK_SET);
            newFile.file = f;
            newFile.size = size;
            newFile.path = filename;

            fnum = (Uint32)f;
            openFiles[fnum] = newFile;
            return fnum;
        }
    }

    return (Uint32)-1;
}

void ExternalFiles::releaseFile(Uint32 file)
{
    fclose(openFiles[file].file);
    openFiles.erase(file);
}

Uint32 ExternalFiles::readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes)
{
    return fread(databuf, 1, numBytes, openFiles[file].file);
}

Uint32 ExternalFiles::readWord(Uint32 file, Uint16 *databuf, Uint32 numWords)
{
    Uint32 numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
#endif

    numRead = fread(databuf, 2, numWords, openFiles[file].file);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap16(databuf[i]);
    }
#endif

    return numRead;
}

Uint32 ExternalFiles::readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees)
{
    Uint32 numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
#endif

    numRead = fread(databuf, 3, numThrees, openFiles[file].file);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap32(databuf[i]);
        databuf[i]<<=8;
    }
#endif

    return numRead;
}

Uint32 ExternalFiles::readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords)
{
    Uint32 numRead;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
#endif

    numRead = fread(databuf, 4, numDWords, openFiles[file].file);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    for( i = 0; i < numRead; i++ ) {
        databuf[i] = SDL_Swap32(databuf[i]);
    }
#endif

    return numRead;
}

char *ExternalFiles::readLine(Uint32 file, char *databuf, Uint32 buflen)
{
    return fgets(databuf, buflen, openFiles[file].file);
}

Uint32 ExternalFiles::writeByte(Uint32 file, const Uint8* databuf, Uint32 numBytes)
{
    return fwrite(databuf, 1, numBytes, openFiles[file].file);
}

Uint32 ExternalFiles::writeWord(Uint32 file, const Uint16 *databuf, Uint32 numWords)
{
    Uint32 numWrote;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint16* tmp = new Uint16[numWords];
    Uint32 i;

    for( i = 0; i < numWords; i++ ) {
        tmp[i] = SDL_Swap16(databuf[i]);
    }

    numWrote = fwrite(tmp, 2, numWords, openFiles[file].file);
    delete[] tmp;
#else

    numWrote = fwrite(databuf, 2, numWords, openFiles[file].file);
#endif

    return numWrote;
}

Uint32 ExternalFiles::writeThree(Uint32 file, const Uint32 *databuf, Uint32 numThrees)
{
    Uint32 numWrote;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32* tmp = new Uint32[numThrees];
    Uint32 i;

    for( i = 0; i < numThrees; i++ ) {
        tmp[i] = SDL_Swap32(databuf[i]);
        tmp[i]<<=8;
    }
    numWrote = fwrite(tmp, 3, numThrees, openFiles[file].file);
    delete[] tmp;
#else

    numWrote = fwrite(databuf, 3, numThrees, openFiles[file].file);
#endif

    return numWrote;
}

Uint32 ExternalFiles::writeDWord(Uint32 file, const Uint32 *databuf, Uint32 numDWords)
{
    Uint32 numWrote;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN

    Uint32 i;
    Uint32* tmp = new Uint32[numDWords];

    for( i = 0; i < numDWords; i++ ) {
        tmp[i] = SDL_Swap32(databuf[i]);
    }
    numWrote = fwrite(tmp, 4, numDWords, openFiles[file].file);
    delete[] tmp;
#else

    numWrote = fwrite(databuf, 4, numDWords, openFiles[file].file);
#endif

    return numWrote;
}

void ExternalFiles::writeLine(Uint32 file, const char *databuf)
{
    fputs(databuf, openFiles[file].file);
}

int ExternalFiles::vfs_printf(Uint32 file, const char* fmt, va_list ap)
{
    int ret;
    ret = vfprintf(openFiles[file].file, fmt, ap);
    return ret;
}

void ExternalFiles::seekSet(Uint32 file, Uint32 pos)
{
    fseek(openFiles[file].file, pos, SEEK_SET);
}

void ExternalFiles::seekCur(Uint32 file, Sint32 pos)
{
    fseek(openFiles[file].file, pos, SEEK_CUR);
}

Uint32 ExternalFiles::getPos(Uint32 file)
{
    return ftell(openFiles[file].file);
}

Uint32 ExternalFiles::getSize(Uint32 file)
{
    return openFiles[file].size;
}

const char* ExternalFiles::getPath(Uint32 file)
{
    return openFiles[file].path.c_str();
}

static FILE* fcaseopen(string* name, const char* mode, bool* isdir, Uint32 caseoffset) {
    FILE* ret;
    string& fname = *name;
    ret = fopen(fname.c_str(), mode);
    if (NULL != isdir)
        *isdir = false;
    if (NULL != ret) {
        return ret;
    }
    if (EISDIR == errno) {
        if (NULL != isdir)
            *isdir = true;
        return NULL;
    }
    // Try all other case.  Assuming uniform casing.
    Uint32 i;
    // Skip over non-alpha chars.
    // @TODO These are the old style text munging routines that are a) consise
    // and b) doesn't work with UTF8 filenames.
    for (i=caseoffset;i<fname.length()&&!isalpha(fname[i]);++i);
    if (islower(fname[i])) {
        transform(fname.begin()+caseoffset, fname.end(), fname.begin()+caseoffset, toupper);
    } else {
        transform(fname.begin()+caseoffset, fname.end(), fname.begin()+caseoffset, tolower);
    }
    ret = fopen(fname.c_str(), mode);
    if (NULL != ret) {
        return ret;
    }
    if (EISDIR == errno) {
        if (NULL != isdir)
            *isdir = true;
        return NULL;
    }
    /// @TODO Try other tricks like "lower.EXT" or "UPPER.ext"
    return NULL;
}
