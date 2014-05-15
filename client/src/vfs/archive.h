#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <cstdarg>
#include "SDL_types.h"

class VFile;

typedef VFile* (*VFSOPEN)(const char*);
typedef void (*VFSCLOSE)(VFile*);

class Archive
{
public:
    virtual ~Archive() {}
    virtual const char *getArchiveType() = 0;
    virtual bool loadArchive(const char *fname) = 0;
    virtual void unloadArchives() {};
    virtual Uint32 getFile(const char *fname) = 0;
    virtual void releaseFile(Uint32 file) = 0;

    virtual Uint32 readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes) = 0;
    virtual Uint32 readWord(Uint32 file, Uint16 *databuf, Uint32 numWords) = 0;
    virtual Uint32 readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees) = 0;
    virtual Uint32 readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords) = 0;
    virtual char *readLine(Uint32 file, char *databuf, Uint32 buflen) = 0;

    /* Since only external files can write, we provide stubs here for the other
     * plugins.
     */
    virtual Uint32 writeByte(Uint32 file, const Uint8* databuf, Uint32 numBytes) {return 0;}
    virtual Uint32 writeWord(Uint32 file, const Uint16* databuf, Uint32 numWords) {return 0;}
    virtual Uint32 writeThree(Uint32 file, const Uint32* databuf, Uint32 numThrees) {return 0;}
    virtual Uint32 writeDWord(Uint32 file, const Uint32* databuf, Uint32 numDWords) {return 0;}
    virtual void writeLine(Uint32 file, const char* databuf) {}
    virtual int vfs_printf(Uint32 file, const char* fmt, va_list ap) {return 0;}
    virtual void flush(Uint32 file) {}

    virtual void seekSet(Uint32 file, Uint32 pos) = 0;
    virtual void seekCur(Uint32 file, Sint32 pos) = 0;

    virtual Uint32 getPos(Uint32 file) = 0;
    virtual Uint32 getSize(Uint32 file) = 0;
    virtual const char* getPath(Uint32 filenum) = 0;
protected:
    VFSOPEN vfopen;
    VFSCLOSE vfclose;
};

typedef Archive* ( *PFDLLOADARCHIVE )( VFSOPEN, VFSCLOSE );

#endif /* ARCHIVE_H */
