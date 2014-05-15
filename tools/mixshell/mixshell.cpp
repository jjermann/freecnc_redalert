#include "SDL.h"

#include "common.h"
#include "mixvfs.h"
#include "filename.h"

using namespace std;

Uint32 loff;

void pusage()
{
    printf("Usage:\nmixshell filename.mix\n");
}

void ls(MIXFiles *mixes)
{
    FData file;
    int i = 0;
    map<Uint32, MIXEntry>::iterator walker;
    for( walker = mixes->mixheaders.begin();
            walker != mixes->mixheaders.end(); walker++ ) {
        file = translateID(walker->first);
        printf("%s", file.filename);
        i++;
        if( i & 4 ) {
            printf("\n");
            i = 0;
        } else {
            printf("\t");
        }
    }
    if( i!=0 ) {
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    MIXFiles *mixes;
    Uint32 offset, size;
    FILE *mf;
    char line[1024];
    char cmd[128];
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

    while( strcmp(cmd, "quit") ) {
        printf("> ");
        fgets(line, 1024, stdin);
        sscanf(line, "%s", cmd);
        if( !strcmp(cmd, "ls") ) {
            ls(mixes);
        } else if( !strcmp(cmd, "cd") ) {
            sscanf(line, "%s %s", cmd, line);
            if( strcasecmp(line+strlen(line)-4, ".mix") ) {
                printf("You can only \"cd\" into mixfiles\n");
            } else if( mixes->getOffsetAndSize(line, &offset, &size) != NULL ) {
                delete mixes;
                mf = fopen(argv[1], "r");
                mixes = new MIXFiles();
                mixes->loadMix(mf, offset, true);
                loff = offset;
            }
        } else if( !strcmp(cmd, "get") ) {
            sscanf(line, "%s %s", cmd, line);
            if( mixes->getOffsetAndSize(line, &offset, &size) != NULL ) {
                Uint8 *buff = new Uint8[size];
                FILE *of;
                fseek(mf, offset, SEEK_SET);
                fread(buff, size, 1, mf);
                of = fopen(line, "w");
                fwrite(buff, size, 1, of);
                fclose(of);
                delete[] buff;
            }
        } else if( strcmp(cmd, "quit") ) {
            printf("Unknow command. Valid commands are:\n\tls\n\tcd\n\tget\n\tquit\n");
        }
    }
    delete mixes;
    return 0;
}
