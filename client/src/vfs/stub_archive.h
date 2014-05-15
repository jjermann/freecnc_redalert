#ifndef STUB_ARCHIVE_H
#define STUB_ARCHIVE_H

#include "archive.h"

/** @class Plugins that depend on external libraries that might not exist on
 * the system that is building, so can be stubbed out by privately inheriting
 * from this class as part of an ifdef.
 */
class StubArchive : public Archive
{
public:
    StubArchive(void*, void*)           {}
    virtual ~StubArchive()              {}
    const char *getArchiveType()        {return "stub archive";}
    bool loadArchive(const char *fname) {return false;}
    void unloadArchives()               {}
    Uint32 getFile(const char *fname)   {return (Uint32)-1;}
    void releaseFile(Uint32 file)       {}

    Uint32 readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes)    {return 0;}
    Uint32 readWord(Uint32 file, Uint16 *databuf, Uint32 numWords)   {return 0;}
    Uint32 readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees) {return 0;}
    Uint32 readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords) {return 0;}
    char *readLine(Uint32 file, char *databuf, Uint32 buflen)        {return 0;}

    void seekSet(Uint32 file, Uint32 pos) {}
    void seekCur(Uint32 file, Sint32 pos) {}

    Uint32 getPos(Uint32 file)    {return 0;}
    Uint32 getSize(Uint32 file)   {return 0;}
    const char *getPath(Uint32 filenum) {return "";}
};

#endif
