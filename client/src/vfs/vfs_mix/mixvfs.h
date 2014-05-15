#ifndef MIX_H
#define MIX_H

#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif

#include "archive.h"

#include "SDL_types.h"

#include <vector>
#include <map>
/* Function to rotate left */
#define ROL(n) ((n<<1) | ((n>>31) & 1))
#define TS_ID 0x763c81dd

/* d = first 4 bytes of mix file */
#define which_game(d) (d == 0 || d == mix_checksum || d == mix_encrypted || d == (mix_encrypted | mix_checksum)) ? game_ra : game_td


const unsigned long int mix_checksum = 0x00010000;
const unsigned long int mix_encrypted = 0x00020000;

/* data type for which game the specific
 * mix file is from. Different from the
 * game definitions in freecnc.h
 */
enum game_t {game_td, game_ra, game_ts};

union mix_header {
    struct {
        Uint16 c_files;
        Uint32 size;
    }
    t;
    Uint32 flags;
};

struct mix_record
{
    Uint32 id;
    Uint32 offset;
    Uint32 size;
};


/* only 256 mixfiles can be loaded */
struct MIXEntry
{
    Uint8 filenum;
    Uint32 offset;
    Uint32 size;
};

class VFile;

struct openFile
{
    Uint32 id;
    Uint32 pos;
};

class MIXFiles : public Archive
{
public:
    MIXFiles(VFSOPEN openf, VFSCLOSE closef);
    ~MIXFiles();
    const char *getArchiveType()
    {
        return "mix archive";
    }
    bool loadArchive(const char *fname);
    void unloadArchives();
    Uint32 getFile(const char *fname);
    void releaseFile(Uint32 file);

    Uint32 readByte(Uint32 file, Uint8 *databuf, Uint32 numBytes);
    Uint32 readWord(Uint32 file, Uint16 *databuf, Uint32 numWords);
    Uint32 readThree(Uint32 file, Uint32 *databuf, Uint32 numThrees);
    Uint32 readDWord(Uint32 file, Uint32 *databuf, Uint32 numDWords);
    char *readLine(Uint32 file, char *databuf, Uint32 buflen);

    void seekSet(Uint32 file, Uint32 pos);
    void seekCur(Uint32 file, Sint32 pos);

    //Uint32 getStartpos(Uint32 file){return 0;}
    Uint32 getPos(Uint32 file)
    {
        return openFiles[file].pos;
    }
    Uint32 getSize(Uint32 file);

    const char* getPath(Uint32 file);
private:
    Uint32 calc_id(const char *fname );
    void readMIXHeader(VFile *mix);
    mix_record *decodeHeader(VFile *mix, mix_header *, int mod);

    std::vector<VFile *> mixfiles;
    std::map<Uint32, MIXEntry> mixheaders;

    std::map<Uint32, openFile> openFiles;
};

#endif
