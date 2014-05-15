#ifndef TGZFILES_H
#define TGZFILES_H

#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif

#ifndef NOZLIB

#include <map>
#include <vector>
#include <string>
#include "archive.h"
#include "SDL_types.h"
#include "SDL_endian.h"
#include "common.h"
#include <zlib.h>

struct tgzfile
{
    Uint8 filenum;
    Uint32 offset;
    Uint32 size;
};

struct openFile
{
    tgzfile* file;
    Uint32 pos;
};

class TGZFiles : public Archive
{
public:
    TGZFiles(VFSOPEN openf, VFSCLOSE closef);
    ~TGZFiles();
    const char* getArchiveType() {
        return "tgz archive";
    }
    bool loadArchive(const char* fname);
    void unloadArchives();
    int getSize(char* sizestr, int len);
    void readHeader(gzFile gf);
    Uint32 getFile(const char* fname);
    void releaseFile(Uint32 file);

    Uint32 readByte(Uint32 file, Uint8* databuf, Uint32 numBytes);
    Uint32 readWord(Uint32 file, Uint16* databuf, Uint32 numWords);
    Uint32 readThree(Uint32 file, Uint32* databuf, Uint32 numThrees);
    Uint32 readDWord(Uint32 file, Uint32* databuf, Uint32 numDWords);
    char* readLine(Uint32 file, char* databuf, Uint32 buflen);

    void seekSet(Uint32 file, Uint32 pos);
    void seekCur(Uint32 file, Sint32 pos);

    Uint32 getPos(Uint32 file);
    Uint32 getSize(Uint32 file);
    const char* getPath(Uint32 file);
private:
    std::vector<gzFile> archList;
    std::map<std::string, tgzfile> fileList;
    std::map<Uint32, openFile> openFiles;
};

#endif /* NOZLIB */

#endif /* TGZFILES_H */
