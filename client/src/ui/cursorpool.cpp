#include <cctype>
#include "cursorpool.h"
#include "inifile.h"
#include "logger.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::string;
using std::map;
#endif

CursorPool::CursorPool(const char* ininame)
{
    cursorini = new INIFile(ininame);
}

CursorPool::~CursorPool()
{
    delete cursorini;
    for (Uint32 i = 0; i < cursorpool.size(); ++i) {
        delete cursorpool[i];
    }
}

cursorinfo* CursorPool::getCursorByName(const char* name)
{
    map<string, Uint16>::iterator cursorentry;
    cursorinfo* datum;
    Uint16 index;

    string cname = (string)name;
    string::iterator p = cname.begin();
    while (p!=cname.end()) {
        *p = toupper(*p);
        ++p;
    }

    cursorentry = name2index.find(cname);
    if (cursorentry != name2index.end() ) {
        index = cursorentry->second;
        datum = cursorpool[index];
    } else {
        index = cursorpool.size();
        datum = new cursorinfo;
        //cursorini->seekSection(name);
        datum->anstart = cursorini->readInt(name,"start",0);
        datum->anend = cursorini->readInt(name,"end",0);
        cursorpool.push_back(datum);
        name2index[cname] = index;
    }

    return datum;
}
