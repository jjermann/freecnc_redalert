#ifndef __VFSPLUGMAN
#define __VFSPLUGMAN

#ifdef _MSC_VER
# pragma warning (disable: 4503)
# pragma warning (disable: 4786)
#endif

#include <vector>
#include "archive.h"
class VFile;

struct LoadedVFSPlugin;

class VFSPlugMan
{
public:
    VFSPlugMan(const char *binpath);
    ~VFSPlugMan();
    bool loadArchive(const char *archname);
    VFile *openFile(const char *fname);
private:
    char *binpath;
    std::vector<struct LoadedVFSPlugin> loadedPlugins;
};

#endif

