#ifndef EXTERNALVFS_H
#define EXTERNALVFS_H

#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif

#include <vector>
#include <map>
#include <string>
#include "common.h"
#include "archive.h"

struct openFileExt
{
    FILE *file;
    Uint32 size;
    std::string path;
};

class ExternalFiles : public Archive
{
public:
    ExternalFiles(const char *defpath);
    const char *getArchiveType()
    {
        return "external file";
    }
    bool loadArchive(const char *fname);
    // Can't use default argument as we need exact type signature for
    // inheritence.
    Uint32 getFile(const char* fname) {return getFile(fname, "rb");}
    Uint32 getFile(const char *fname, const char* mode);
    void releaseFile(Uint32 file);

    Uint32 readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes);
    Uint32 readWord(Uint32 file, Uint16 *databuf, Uint32 numWords);
    Uint32 readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees);
    Uint32 readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords);
    char *readLine(Uint32 file, char *databuf, Uint32 buflen);

    Uint32 writeByte(Uint32 file, const Uint8 *databuf, Uint32 numBytes);
    Uint32 writeWord(Uint32 file, const Uint16 *databuf, Uint32 numWords);
    Uint32 writeThree(Uint32 file, const Uint32 *databuf, Uint32 numThrees);
    Uint32 writeDWord(Uint32 file, const Uint32 *databuf, Uint32 numDWords);
    void writeLine(Uint32 file, const char *databuf);
    int vfs_printf(Uint32 file, const char* fmt, va_list ap);
    void flush(Uint32 file)
    {
        fflush(openFiles[file].file);
    }

    void seekSet(Uint32 file, Uint32 pos);
    void seekCur(Uint32 file, Sint32 pos);

    Uint32 getPos(Uint32 file);
    Uint32 getSize(Uint32 file);
    const char* getPath(Uint32 file);
private:
    std::string defpath;
    std::vector<std::string> path;
    std::map<Uint32, openFileExt> openFiles;
};

#endif /* EXTERNAL_VFS */

