#ifndef MIX_H
#define MIX_H
#include "SDL.h"

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

typedef struct
{
    Uint32	id;
    Uint32	offset;
    Uint32	size;
}
mix_record;



/* only 256 mixfiles can be loaded */
typedef struct
{
    Uint8 filenum;
    Uint32 offset;
    Uint32 size;
}
MIXEntry;


class MIXFiles
{
public:
    MIXFiles();
    ~MIXFiles();
    void loadMix(FILE *file, Uint32 startpos, bool external);
    FILE *getOffsetAndSize(char *fname, Uint32 *offset, Uint32 *size);
    Uint32 calc_id( char *fname );
private:
    void readMIXHeader(FILE *mix, Uint32 startpos, Uint8 fileno);
    mix_record *decodeHeader(FILE *mix, mix_header *, int mod);

    std::vector<FILE *> mixfiles;
public:
    std::map<Uint32, MIXEntry> mixheaders;

};
#endif

