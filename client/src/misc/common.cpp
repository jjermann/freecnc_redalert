#include <string>
#include <cstdio>
#include <cctype>
#ifdef __BEOS__
#include <Entry.h>
#include <Path.h>
#endif
#include "common.h"

int mapscaleq = -1;
static std::string binloc;

namespace p {
    ActionEventQueue* aequeue = 0;
    CnCMap* ccmap = 0;
    UnitAndStructurePool* uspool = 0;
    PlayerPool* ppool = 0;
    WeaponsPool* weappool = 0;
    Dispatcher* dispatcher = 0;
}

// Client only
namespace pc {
    SoundEngine* sfxeng = 0;
    GraphicsEngine* gfxeng = 0;
    NetConnection* conn = 0;
    MessagePool* msg = 0;
    std::vector<SHPImage *>* imagepool = 0;
    ImageCache* imgcache = 0;
    Sidebar* sidebar = 0;
    Cursor* cursor = 0;
    Input* input = 0;
}

// Server only
namespace ps {
    AI::AIPlugMan* aiplugman = 0;
}

// Server only
std::vector<char*> splitList(char* line, char delim)
{
    std::vector<char*> retval;
    char* tmp;
    Uint32 i,i2;
    tmp = NULL;
    if (line != NULL) {
        tmp = new char[16];
        memset(tmp,0,16);
        for (i=0,i2=0;line[i]!=0x0;++i) {
            if ( (i2>=16) || (tmp != NULL && (line[i] == delim)) ) {
                retval.push_back(tmp);
                tmp = new char[16];
                memset(tmp,0,16);
                i2 = 0;
            } else {
                tmp[i2] = line[i];
                ++i2;
            }
        }
        retval.push_back(tmp);
    }
    return retval;
}

/// change "foo123" to "foo"
char* stripNumbers(const char* src)
{
    char* dest;
    Uint16 i;
    for (i=0;i<strlen(src);++i) {
        if (src[i] <= '9') {
            break;
        }
    }
    dest = new char[i+1];
    strncpy(dest,src,i);
    dest[i] = 0;
    return dest;
}

char* determineBinaryLocation(const char* launchcmd) {
    FILE *f;
    char *path, *delim, *envpath, *testcmd;
    const char* initial;

#ifdef __BEOS__
    // Thanks Caz
    entry_ref ref;
    BPath bpath;
    if(get_ref_for_path(launchcmd, &ref) == B_NO_ERROR) {
        BEntry entry(&ref);
        if(entry.GetPath(&bpath) == B_NO_ERROR) {
            // since we dup anything returned, this isn't a problem
            initial = bpath.Path();
        }
    }
#else
    // not changing the constant, changing what it points to
    initial = launchcmd;
#endif
    #ifdef _WIN32
    #define DIRLISTSEPARATOR ';'
    #else
    #define DIRLISTSEPARATOR ':'
    #endif

    f = fopen(initial, "rb");
    if( f != NULL ) {
        fclose(f);
        path = cppstrdup(initial);
        delim = strrchr(path, '/');
#ifdef _WIN32

        delim = (char*)max((unsigned int)delim, (unsigned int)strrchr(path, '\\'));
#endif

        if( delim == NULL ) {
            delete path;
            binloc = ".";
            return cppstrdup(".");
        }
        delim[0] = '\0';
        binloc = path;
        return path;
    }

    envpath = getenv("PATH");
    path = envpath;
    while( (delim = strchr(envpath, DIRLISTSEPARATOR)) != NULL ) {
        delim[0] = '\0';
        delim++;

        if( path[strlen(path)-1] != '/'
#ifdef _WIN32
                && path[strlen(path)-1] != '\\'
#endif
          ) {
            testcmd = new char[strlen(path)+strlen(initial)+1];
            sprintf(testcmd, "%s/%s", path, initial);
        } else {
            testcmd = new char[strlen(path)+strlen(initial)];
            sprintf(testcmd, "%s%s", path, initial);
        }
        if( (f = fopen(testcmd, "rb")) != NULL ) {
            fclose(f);
            path = cppstrdup(initial);
            delim = strrchr(path, '/');
#ifdef _WIN32

            delim = (char*)max((unsigned int)delim, (unsigned int)strrchr(path, '\\'));
#endif

            if( delim == NULL ) {
                delete[] path;
                delete[] testcmd;
                binloc = ".";
                return cppstrdup(".");
            }
            delim[0] = '\0';
            delete[] testcmd;
            binloc = path;
            return path;
        }
        path = delim;
        delete[] testcmd;
    }
    binloc = ".";
    return cppstrdup(".");
}

const char* getBinaryLocation() {
    return binloc.c_str();
}
