/*****************************************************************************
 * freecnc.cpp - Initialize SDL, initial startup of game, clean up at end
 *
 ****************************************************************************/
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <stdexcept>
#include "common.h"
#include "SDL.h"
#include "freecnc.h"
#include "game.h"
#include "vqa.h"
#include "wsa.h"
#include "vfs.h"
#include "soundengine.h"
#include "graphicsengine.h"
#include "logger.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::abort;
using std::set_terminate;
using std::runtime_error;
#endif

#if defined _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(macintosh)
#elif defined __BEOS__
#else
#include <unistd.h>
#include <sys/types.h>
#endif

Logger *logger;

void PrintUsage(); // In args.cpp
bool parse(int argc, char** argv); // In args.cpp
void cleanup();
void freeres_wrapper();
void fcnc_terminate_handler();

/** Main function, creates a instance of the FreeCNC class and runs it.
 * After parsing the commandline args. */
int main(int argc, char** argv)
{
    set_terminate(fcnc_terminate_handler);
#if defined __linux__ && defined DEBUG && __GNUC__ >= 3 && __GNUC_MINOR__ >= 2
    /* Library documentation recommends setting this in the environment before
     * running to cut down on false leak reports in the standard library.
     */
    if (getenv("GLIBCPP_FORCE_NEW") == NULL) {
        fprintf(stderr,"Note: GLIBCPP_FORCE_NEW wasn't set.\n");
    }
#endif

    if ((argc > 1) && ( (strcasecmp(argv[1],"-help")==0) || (strcasecmp(argv[1],"--help")==0)) ) {
        PrintUsage();
        return 1;
    }
#if defined _WIN32
    // Check if user is administrator
#elif defined(macintosh)
#elif defined(__BEOS__)
    // BeOS runs everything as root
#else
    if (getuid() == 0) {
        fprintf(stderr,"WARNING WARNING WARNING WARNING!\n"
                "\tYOU ARE RUNNING FREECNC AS ROOT.\n"
                "PLEASE DO NOT DO SO, ROOT PRIVILEGES ARE NOT NEEDED.\n");
        return 1;
    }
    if (geteuid() == 0) {
        fprintf(stderr,"WARNING WARNING WARNING WARNING!\n"
                "\tTHE FREECNC BINARY IS SET SUID ROOT.\n"
                "PLEASE DO NOT DO SO, ROOT PRIVILEGES ARE NOT NEEDED.\n");
        return 1;
    }
#endif
    char *binpath, *lf;
    binpath = determineBinaryLocation(argv[0]);
    lf = new char[strlen(binpath)+strlen("freecnc.log")+2];
    sprintf(lf, "freecnc.log");
    VFS_PreInit(binpath);
    // Log level is so that only errors are shown on stdout by default
    logger = new Logger(lf,0);
    delete[] lf;
    VFS_Init(binpath);
    delete[] binpath;
    if (!parse(argc, argv)) {
        delete logger;
        VFS_Destroy();
        return 1;
    }
    logger->note("Please wait, FreeCNC %s is starting\n",VERSION);
    // Create a new freecnc session, run it and remove it from memory
    atexit(cleanup);

    try {
        FreeCNC freecnc;
        freecnc.run();
    } catch (runtime_error& e) {
        logger->error("%s\n",e.what());
#if _WIN32
        MessageBox(0,e.what(),"Fatal error",0);
#endif
    }
    return 0;
}

/** Constructor, initiates sdl, sets up a timer and gets ready to run.
 * Also plays the intro movie
 * @param a class containing arguments.
 */
FreeCNC::FreeCNC()
{
    config = getConfig();
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) < 0) {
        logger->error("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    if (!config.debug) {
        /* Hide the cursor since we have our own.
         * Don't hide if we're debugging as the lag when running inside
         * valgrind is really irritating.  */
        SDL_ShowCursor(0);
    }
    // Init the video and sound
    try {
        logger->note("Initialising the graphics engine...");
        pc::gfxeng = new GraphicsEngine();
        logger->note("done\n");
    } catch (GraphicsEngine::VideoError&) {
        logger->note("failed.  exiting\n");
        throw runtime_error("Unable to initialise the graphics engine");
    }
    try {
        logger->note("Initialising the sound engine...");
        pc::sfxeng = new SoundEngine(config.nosound);
        if (!config.nosound) {
            logger->note("done\n");
        }
    } catch (SoundError&) {
        logger->note("failed. running without sound...");
        try {
            pc::sfxeng = new SoundEngine(true);
            logger->note("Ok\n");
        } catch (SoundError&) {
            logger->note("Not ok!\nThis is a bug.  Please try to reproduce this and let us know.\n");
            delete pc::gfxeng;
            throw runtime_error("Fallback version of soundengine failed");
        }
    }
    // "Standalone" VQA Player
    if (config.playvqa) {
        logger->note("Now playing %s\n",config.vqamovie.c_str());
        Uint8 ret = 0;
        try {
            VQAMovie mov(config.vqamovie.c_str());
            mov.play();
        } catch (VQAError&) {
            ret = 1;
        }
        delete pc::gfxeng;
        delete pc::sfxeng;
        exit(ret);
    }
    // Play the intro if requested
    if (config.intro) {
        try {
            const char* intro;
            if (config.gamenum == GAME_RA)
                intro = "prolog";
            else
                intro = "logo";
            VQAMovie mov(intro);
            mov.play();
        } catch (VQAError&) {
        }

        try {
            WSA choose("choose.wsa");
            choose.animate();
        } catch (WSA::WSAError&) {
        }
    }

    // Init the rand functions
    srand(time(NULL));
}

FreeCNC::~FreeCNC()
{
    delete pc::gfxeng;
    delete pc::sfxeng;
}

/// Run a freecnc session
void FreeCNC::run()
{
    logger->note("Initialising game engine:\n");
    try {
        Game gsession;
        logger->note("Starting game engine\n");
        gsession.play();
        logger->note("Shutting down\n");
    } catch (Game::GameError&) {
    }
}

/// Wraps around a libc internal function to force resource deallocation
#if defined __linux__ && defined DEBUG
extern "C" void __libc_freeres();
inline void freeres_wrapper()
{
    __libc_freeres();
}
#else
inline void freeres_wrapper() {}
#endif

/// Wraps around setting a more verbose default handler.
void fcnc_terminate_handler()
{
    // Be nice and try to reset the video mode.
    SDL_Quit();
#if __GNUC__ == 3 && __GNUC_MINOR__ >= 1
    // GCC 3.1+ feature, and is turned on by default for 3.4.
    using __gnu_cxx::__verbose_terminate_handler;
    __verbose_terminate_handler();
#else
    abort();
#endif
}

void cleanup()
{
    delete logger;
    VFS_Destroy();
    SDL_Quit();
    freeres_wrapper();
}
