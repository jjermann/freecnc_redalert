// mode: -*- C++ -*-
/*****************************************************************************
 * inifile.h - header file for inifile
 ****************************************************************************/

#ifndef INIFILE_H
#define INIFILE_H

#ifdef _MSC_VER
#pragma warning (disable: 4503)
#pragma warning (disable: 4786)
#endif

#include "SDL_types.h"
#include <cstdio>
#include <map>
#include <string>

#define MAXLINELENGTH 1024
#define MAXSTRINGLENGTH 128
#define INIERROR 0x7fffffff

typedef std::map<std::string,std::string> INISection;
typedef INISection::const_iterator INIKey;

/** @brief Parses inifiles.
 * @TODO zx64 has a parser written using a few bits from boost.  It's much
 * faster and uses templates to simplify a lot of the code.  The only problem is
 * integrating the bits of boost it uses into the tree.
 * For the interested: http://freecnc.sf.net/parse.tar.bz2
 * @TODO It's probably worth pooling INIFile instances so we only need to parse
 * them once.
 */
class INIFile
{
public:
    explicit INIFile(const char* filename);
    ~INIFile();

    char* readString(const char* section, const char* value);
    char* readString(const char* section, const char* value, const char* deflt);

    int readInt(const char* section, const char* value, Uint32 deflt);
    int readInt(const char* section, const char* value);

    INIKey readKeyValue(const char* section, Uint32 keynum);
    INIKey readIndexedKeyValue(const char* section, Uint32 keynum, const char* prefix=0);
    std::string readSection(Uint32 secnum);
private:
    std::map<std::string, INISection> inidata;
};

#endif
