/* Command-line aud player
 * used to debug the aud decoding and playing code
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SDL.h"
#include "stubs.h"
#include "inifile.h"
#include "soundengine.h"
#include "vfs.h"
#include "common.h"
#include "logger.h"

void usage(char* biname)
{
    printf("Usage: %s <audname.aud>\n",biname);
}

Logger* logger;
bool done;
int main(int argc, char* argv[])
{
    SoundEngine* snd;
    char* binpath, *lf;

    if (argc != 2) {
        usage(argv[0]);
        exit(1);
    }
    if ((strcasecmp(argv[1],"-h")==0)||(strcasecmp(argv[1],"-help")==0)||
            (strcasecmp(argv[1],"--help")==0)) {
        usage(argv[0]);
        exit(1);
    }
    binpath = determineBinaryLocation(argv[0]);
    lf = new char[strlen(binpath)+strlen("audplay.log")+2];
    sprintf(lf, "%s/audplay.log", binpath);
    VFS_PreInit(binpath);
    logger = new Logger(lf,0);
    delete[] lf;
    VFS_Init(binpath);
    done = false;
    if( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_NOPARACHUTE) < 0) {
        logger->error("Couldn't initialize SDL: %s\n", SDL_GetError() );
        exit(1);
    }
    try {
        snd = new SoundEngine();
    } catch (SoundError&) {
        delete logger;
        VFS_Destroy();
        SDL_Quit();
        exit(1);
    }
    try {
        snd->queueSound(argv[1]);
    } catch (SoundError&) {
        logger->error("File not found! %s\n",argv[1]);
        delete snd;
        delete logger;
        VFS_Destroy();
        SDL_Quit();
        exit(1);
    }
    while (!done) {
        // TODO: some form of text based ui (r to restart etc.)
        //SDL_Delay(10);
    }
    delete snd;
    delete logger;
    VFS_Destroy();
    SDL_Quit();
    exit(0);
}
