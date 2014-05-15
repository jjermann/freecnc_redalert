#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <cstdarg>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "SDL_endian.h"
#include "archive.h"
#include "common.h"
#include "externalvfs.h"
#include "inifile.h"
#include "logger.h"
#include "vfsplugman.h"
#include "vfs.h"

using std::string;
using std::vector;
using std::runtime_error;

ExternalFiles *externals;
VFSPlugMan *plugins;

/** Sets up externals so that the logger can work
 */
void VFS_PreInit(const char* binpath)
{
    externals = new ExternalFiles(binpath);
}

/** @todo install prefix
 */
void VFS_Init(const char* binpath)
{
    INIFile *filesini;
    INIFile *generalini;
    INIKey key;
    string tempstr;
    Uint32 keynum;

    //logger->debug("Assuming binary is installed in \"%s\"\n", binpath);

    if (strcasecmp(".",binpath)!=0) {
        externals->loadArchive("./");
    }
    tempstr = binpath; tempstr += "/";
    externals->loadArchive(tempstr.c_str());

    try {
        generalini = new INIFile("conf/general.ini");
    } catch(int) {
        logger->error("Unable to locate general.ini.\n");
        return;
    }

    // TODO: Try all specified gamedirs (atm: try first)
    key = generalini->readIndexedKeyValue("GENERAL",1,"GAME");
    logger->note("Trying to load \"%s\"...\n",key->second.c_str());
    string gamedir = "conf/";
    gamedir += key->second;
    if (gamedir[gamedir.length()-1] != '/' &&
        gamedir[gamedir.length()-1] != '\\') {
        gamedir += "/";
    }
    externals->loadArchive(gamedir.c_str());
    delete generalini;

    try {
        filesini = new INIFile("files.ini");
    } catch(runtime_error& e) {
        logger->error("Unable to locate files.ini.\n");
        return;
    }

    // Load the mix directory
    string mixdir = gamedir;
    mixdir += "mix/";
    externals->loadArchive(mixdir.c_str());

    plugins = new VFSPlugMan(binpath);
    try {
        // First check we have all the required mixfiles.
        for (keynum = 1; ;keynum++) {
            INIKey key2;
            try {
                key2 = filesini->readIndexedKeyValue("FILES",keynum,
                        "REQUIRED");
            } catch(int) {
                break;
            }
            if( !plugins->loadArchive(key2->second.c_str()) ) {
                logger->warning("Missing required file \"%s\"\n",
                        key2->second.c_str());
                throw 0;
            }

        }
    } catch(int) {
        delete plugins;
        delete filesini;
        logger->error("Unable to find mixes for any of the supported games!\n"
            "Check your configuration and try again.\n");
        #ifdef _WIN32
        MessageBox(0,"Unable to find mixes for any of the supported games!\n"
            "Check your configuration and try again.","Error",0);
        #endif
        exit(1);
    }

    // Now load as many of the optional mixfiles as we can.
    for (keynum = 1; ;keynum++) {
        INIKey key2;
        try {
            key2 = filesini->readIndexedKeyValue("FILES",keynum,
                    "OPTIONAL");
        } catch(int) {
            break;
        }
        plugins->loadArchive(key2->second.c_str());
    }
    delete filesini;
    return;
}

void VFS_Destroy()
{
    delete plugins;
    delete externals;
}

VFile *VFS_Open(const char *fname)
{
    return VFS_Open(fname,"rb");
}

VFile *VFS_Open(const char *fname, const char* mode)
{
    Uint32 fnum;
    fnum = externals->getFile(fname, mode);
    if( fnum != (Uint32)-1 ) {
        return new VFile(fnum, externals);
    }
    // Won't attempt to write/create files in real archives
    if (mode[0] != 'r') {
        return NULL;
    }
    if (plugins != NULL) {
        return plugins->openFile(fname);
    }
    return NULL;
}

void VFS_Close(VFile *file)
{
    delete file;
}

const char* VFS_getFirstExisting(const vector<const char*>& files)
{
    VFile* tmp;
    for (Uint32 i=0;i<files.size();++i) {
        tmp = VFS_Open(files[i],"r");
        if (tmp != NULL) {
            VFS_Close(tmp);
            return files[i];
        }
    }
    return NULL;
}

const char* VFS_getFirstExisting(Uint32 count, ...)
{
    VFile* tmp;
    va_list ap;
    char* name;
    va_start(ap,count);
    while (count--) {
        name = (char*)va_arg(ap,char*);
        tmp = VFS_Open(name);
        if (tmp != NULL) {
            VFS_Close(tmp);
            va_end(ap);
            return name;
        }
    }
    va_end(ap);
    return NULL;
}
