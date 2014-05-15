#ifndef DLLIBRARY_H
#define DLLIBRARY_H

#include <string>

/* These defines are basicly taken from arianne since I can't test
 all platforms myself and I'm also very lazy =)  -- Tim */
#ifdef _WIN32
#include <windows.h>
#define LIBHAND HINSTANCE
#elif defined(__BEOS__)
#include <kernel/image.h>
#define LIBHAND image_id
#elif defined(HPUX)
#define LIBHAND shl_t*
#elif defined(macintosh)
#include <CodeFragments.h>
#define LIBHAND CFragConnectionID
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#define LIBHAND NSModule
#else
#define LIBHAND void*
#endif

class DLLibrary {
public:
    DLLibrary(const char *libname);
    ~DLLibrary();
    void* getFunction(const char *funcname);
private:
    LIBHAND library;
    std::string name;
};

#endif

