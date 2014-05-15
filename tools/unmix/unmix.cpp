#include "SDL.h"

#include "common.h"
#include "mixvfs.h"
#include "filename.h"

using namespace std;

Uint32 loff;

void pusage()
{
    printf("Usage:\nunmix filename.mix\n");
}

int main(int argc, char **argv)
{
    MIXFiles *mixes;
    Uint32 offset, size;
    FILE *mf;
    if( argc < 2 ) {
        pusage();
        exit(1);
    }

    loff = 0;

    mf = fopen(argv[1], "r");

    if( mf == NULL ) {
        pusage();
        exit(1);
    }

    mixes = new MIXFiles();
    mixes->loadMix(mf, 0, true);

    FData file;     
    map<Uint32, MIXEntry>::iterator walker;
    for( walker = mixes->mixheaders.begin(); walker != mixes->mixheaders.end(); walker++ ) {
        file = translateID(walker->first);
      if( mixes->getOffsetAndSize(file.filename, &offset, &size) != NULL ) {
        Uint8 *buff = new Uint8[size];
        FILE *of;
        fseek(mf, offset, SEEK_SET);
        fread(buff, size, 1, mf);
        of = fopen(file.filename, "w");
        fwrite(buff, size, 1, of);
        fclose(of);
        delete[] buff;
      }
    }

    delete mixes;
    return 0;
}
