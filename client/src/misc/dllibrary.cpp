#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "common.h"
#include "dllibrary.h"
#include "logger.h"

/*=====================Windows======================================*/
#ifdef _WIN32 //.dll
#include <windows.h>
DLLibrary::DLLibrary(const char* libname) : name(libname) {
    library = LoadLibrary(libname);
    if( !library ) {
        logger->warning("Unable to load library \"%s\"\n",
                        libname);
        throw 0;
    }
}

DLLibrary::~DLLibrary() {
    FreeLibrary(library);
}

void* DLLibrary::getFunction(const char* funcname) {
    void *retval = (void*)GetProcAddress(library, funcname);
    if( retval == NULL ) {
        logger->warning("Unable to extract function \"%s\" from library \"%s\"\n", funcname, name.c_str());
    }
    return retval;
}

/*=====================BeOS======================================*/
#elif defined(__BEOS__) //no extention
#include <kernel/image.h>
DLLibrary::DLLibrary(const char* libname) : name(libname) {
    library = load_add_on(libname);
    if( !library ) {
        logger->warning("Unable to load library \"%s\"\n", libname);
        throw 0;
    }
}

DLLibrary::~DLLibrary() {
    unload_add_on(library);
}

void* DLLibrary::getFunction(const char* funcname) {
    void *symbol;
    if(B_OK == get_image_symbol(library,funcname,B_SYMBOL_TYPE_ANY,&symbol))
        return symbol;
    logger->warning("Unable to extract function \"%s\" from library \"%s\"\n", funcname, name.c_str());
    return NULL;
}

/*=====================HP(s)UX======================================*/
#elif defined(HPUX) //.sl
#include <dl.h>
DLLibrary::DLLibrary(const char* libname) : name(libname) {
    library = shl_load(libname,BIND_DEFERRED,0);
    if( !library ) {
        logger->warning("Unable to load library \"%s\"\n", libname);
        throw 0;
    }
}

DLLibrary::~DLLibrary() {
    shl_unload(library);
}

void* DLLibrary::getFunction(const char* funcname) {
    void *symbol;
    if(shl_findsym(&library,funcname,TYPE_PROCEDURE,&symbol)!=0)
        return symbol;
    logger->warning("Unable to extract function \"%s\" from library \"%s\"\n", funcname, name.c_str());
    return NULL;
}

/*=====================MacOS preX======================================*/
#elif defined(macintosh) //no extention
DLLibrary::DLLibrary(const char* libname) : name(libname) {
    OSErr err;
    Ptr   mainPtr;
    Str255 errName;
    Str255 lib_name_p;

    lib_name_p[0] = name.length();
    memcpy( lib_name_p + 1, libname.c_str(), lib_name_p[0]);

    err = GetSharedLibrary( lib_name_p, kPowerPCCFragArch,
                            kReferenceCFrag, &library, &mainPtr, errName );
    if( err != noErr )
        library = NULL;

    if( !library ) {
        logger->warning("Unable to load library \"%s\"\n", libname);
        throw 0;
    }
}

DLLibrary::~DLLibrary() {
    CloseConnection(library);
}

void *DLLibrary::getFunction(const char* funcname) {
    Ptr addrPtr;
    CFragSymbolClass symClass;
    Str255 sym_name_p;

    sym_name_p[0] = strlen( funcname );
    memcpy( sym_name_p + 1, funcname, sym_name_p[0]);

    if(noErr == FindSymbol( library, sym_name_p, &addrPtr, &symClass ))
        return (void*)addrPtr;
    logger->warning("Unable to extract function \"%s\" from library \"%s\"\n", funcname, name.c_str());
    return NULL;
}

/*=====================MacOS X======================================*/
#elif defined(__APPLE__)
DLLibrary::DLLibrary(const char* libname) : name(libname) {
    NSObjectFileImage image;

    if(NSCreateObjectFileImageFromFile(libname, &image) == NSObjectFileImageSuccess) {
        library = NSLinkModule(image, libname,
                               NSLINKMODULE_OPTION_RETURN_ON_ERROR|
                               NSLINKMODULE_OPTION_PRIVATE);
        if (!library) {
            NSLinkEditErrors ler;
            int lerno;
            const char* errstr;
            const char* file;
            NSLinkEditError(&ler,&lerno,&file,&errstr);
            logger->warning("Unable to load library \"%s\": %s\n", libname, errstr);
            throw 0;
        }
        NSDestroyObjectFileImage(image);
    } else {
        logger->warning("Unable to load library \"%s\"\n", libname);
        throw 0;
    }
}

DLLibrary::~DLLibrary() {
    NSUnLinkModule(library, 0);
}

void* DLLibrary::getFunction(const char* funcname) {
    char* fsname = new char[strlen(funcname)+2];
    sprintf(fsname, "_%s", funcname);
    NSSymbol sym = NSLookupSymbolInModule(library, fsname);
    delete[] fsname;
    if (sym) {
        return NSAddressOfSymbol(sym);
    }
    logger->warning("Unable to extract function \"%s\" from library \"%s\"\n", funcname, name.c_str());
    return NULL;
}

/*=====================Some form of standard unix======================================*/
#else // .so
#include <dlfcn.h>
#ifdef RTLD_NOW
#define LIBOPTION RTLD_NOW
#elif defined(RTLD_LAZY)
#define LIBOPTION RTLD_LAZY
#elif defined(DL_LAZY)
#define LIBOPTION DL_LAZY
#else
#define LIBOPTION 0
#endif

DLLibrary::DLLibrary(const char* libname) : name(libname) {
    library = dlopen(libname, LIBOPTION);
    if( !library ) {
        logger->warning("Unable to load library \"%s\": %s\n", libname, dlerror());
        throw 0;
    }
}

DLLibrary::~DLLibrary() {
    dlclose(library);
}

void* DLLibrary::getFunction(const char* funcname) {
    void *retval = (void*)dlsym(library, funcname);
    //FIXME: Some dynamic loaders require a prefix on the shared names.
    //       the only one I've encountered is "_" which is required on openBSD
    //       and possible some other platform. This should be detected at
    //       compiletime but libs are loaded so seldom this will have to do
    //       for now.
    if (retval == NULL) {
        char* fsname;
        fsname = new char[strlen(funcname)+2];
        sprintf(fsname, "_%s", funcname);
        retval = (void*)dlsym(library, fsname);
        delete[] fsname;
    }
    if (retval == NULL) {
        logger->warning("Unable to extract function \"%s\" from library \"%s\": %s\n", funcname, name.c_str(), dlerror());
    }
    return retval;
}

#endif

