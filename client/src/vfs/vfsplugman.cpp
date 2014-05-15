#include <cstdio>
#include "common.h"
#include "dllibrary.h"
#include "logger.h"
#include "vfsplugman.h"
#include "vfs.h"

struct LoadedVFSPlugin
{
    char *archtype;
    DLLibrary *lib;
    Archive *arch;
};


VFSPlugMan::VFSPlugMan(const char *binpath)
{
    this->binpath = cppstrdup(binpath);
}

VFSPlugMan::~VFSPlugMan()
{
    unsigned int i;
    for( i = 0; i < loadedPlugins.size(); i++ ) {
        loadedPlugins[i].arch->unloadArchives();
    }
    for( i = 0; i < loadedPlugins.size(); i++ ) {
        delete[] loadedPlugins[i].archtype;
        delete loadedPlugins[i].arch;
        delete loadedPlugins[i].lib;
    }
    delete[] binpath;
}

bool VFSPlugMan::loadArchive(const char *archname)
{
    unsigned int i;
    char *extension, *libname;
    struct LoadedVFSPlugin newPlug;
    PFDLLOADARCHIVE loadArchive;

    extension = strrchr(archname, '.');
    if( extension == NULL ) {
        return false;
    }
    extension++;

    for( i = 0; i < loadedPlugins.size(); i++ ) {
        if( !strcasecmp(loadedPlugins[i].archtype, extension) ) {
            return loadedPlugins[i].arch->loadArchive(archname);
        }
    }

    libname = new char[strlen(extension)+strlen(binpath)+6];
    sprintf(libname, "%s/%s.vfs", binpath, extension);
    try {
        newPlug.lib = new DLLibrary(libname);
    } catch( int ) {
        logger->error("Was unable to load plugin \"%s\"\nAre you sure it has been built?\n",libname);
        delete[] libname;
        return false;
    }
    delete[] libname;
    loadArchive = (PFDLLOADARCHIVE)newPlug.lib->getFunction("loadArchive");
    if( loadArchive == NULL ) {
        return false;
    }
    newPlug.arch = loadArchive(VFS_Open, VFS_Close);
    newPlug.archtype = cppstrdup(extension);
    loadedPlugins.push_back(newPlug);
    return newPlug.arch->loadArchive(archname);
}

VFile *VFSPlugMan::openFile(const char *fname)
{
    unsigned int i;
    Uint32 fnum;
    for( i = 0; i < loadedPlugins.size(); i++ ) {
        if( (fnum = loadedPlugins[i].arch->getFile(fname))!=(Uint32)-1 ) {
            return new VFile(fnum, loadedPlugins[i].arch);
        }
    }
    return NULL;
}

