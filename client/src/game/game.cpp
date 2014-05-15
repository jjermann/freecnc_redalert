/*****************************************************************************
 * game.cpp - General game loop class
 *****************************************************************************/
#include "SDL.h"
#include "actioneventqueue.h"
#include "aiplugman.h"
#include "ccmap.h"
#include "common.h"
#include "config.h"
#include "cursor.h"
#include "dispatcher.h"
#include "freecnc.h"
#include "game.h"
#include "graphicsengine.h"
#include "input.h"
#include "loadingscreen.h"
#include "logger.h"
#include "netconnection.h"
#include "playerpool.h"
#include "sidebar.h"
#include "snprintf.h"
#include "soundengine.h"
#include "vqa.h"

/** Constructor, loads the map, sidebar and such. plays briefing and actionmovie
 */
Game::Game() {
    ConfigType config;
    VQAMovie *mov;
    char* message,*tmp;
    INIFile* fileini;
    LoadingScreen *loadscreen;
    config = getConfig();
    /* set the pointer to the gfx engine */
    // We let the runtime_error propagate upwards.
    fileini = new INIFile("files.ini");
    asprintf(&tmp,"play%i",config.gamenum);
    message = fileini->readString("general",tmp,"TD");
    free(tmp);
    if (!pc::sfxeng->createPlayList(message)) {
        logger->error("Could not create playlist!\n");
        throw GameError();
    }
    delete[] message;
    delete fileini;
    loadscreen = new LoadingScreen();
    gamemode = config.gamemode;
    if (gamemode == 2) {
        try {
            NetConnection::initMessages();
        } catch(int) {
            throw GameError();
        }
        tmp = new char[64];
        sprintf(tmp,"Connecting to server: %s",config.serveraddr.c_str());
        loadscreen->setCurrentTask(tmp);
        delete[] tmp;

        try {
            pc::conn = new NetConnection(config.serveraddr.c_str(), config.serverport);
        } catch(int) {
            delete loadscreen;
            throw GameError();
        }
        // after connection sending login data
        loadscreen->setCurrentTask("Sending Login Data");
        pc::conn->login(VERSION, config.nick.c_str(), config.mside.c_str(), config.side_colour.c_str());
    }
    /* reset the tickcounter, should be a function in the class conatining the
     * counter */
    loadscreen->setCurrentTask("Creating the ActionEventQueue");
    p::aequeue = new ActionEventQueue();
    /* load the map */
    loadscreen->setCurrentTask("Loading the map.");
    try {
        p::ccmap = new CnCMap();
        p::ccmap->loadMap(config.mapname.c_str(), loadscreen);
    } catch (CnCMap::LoadMapError&) {
        delete loadscreen;
        // loadmap will have printed the error
        throw GameError();
    }
    p::dispatcher = new Dispatcher();
    switch (config.dispatch_mode) {
        case 0:
            break;
        case 1:
            // Record
            break;
        case 2:
            // Playback
            break;
        default:
            logger->error("Invalid dispatch mode: %i\n",config.dispatch_mode);
            throw GameError();
            break;
    }

    ps::aiplugman = new AI::AIPlugMan(getBinaryLocation());

    delete loadscreen;
    switch (gamemode) {
    case 0:
        /* play briefing */
        try {
            mov = new VQAMovie(p::ccmap->getMissionData().brief);
            mov->play();
            delete mov;
        } catch (VQAError&) {
        }
        try {
            mov = new VQAMovie(p::ccmap->getMissionData().action);
            mov->play();
            delete mov;
        } catch (VQAError&) {
        }
        break;
    case 1:
        p::ppool->setupAIs();
        break;
    case 2:
        break;
    default:
        break;
    }
    /* init sidebar */
    try {
        pc::sidebar = new Sidebar(p::ppool->getLPlayer(), pc::gfxeng->getHeight(),
                p::ccmap->getMissionData().theater);
    } catch (Sidebar::SidebarError&) {
        throw GameError();
    }
    /* init cursor */
    pc::cursor = new Cursor();
    /* init the input functions */
    pc::input = new Input(pc::gfxeng->getWidth(), pc::gfxeng->getHeight(),
                          pc::gfxeng->getMapArea());
}

/** Destructor, frees up some memory */
Game::~Game()
{
    if (gamemode == 2) {
        logger->note("Disconnecting from server...");
        pc::conn->logout();
        delete pc::conn;
        logger->note("done\n");
    }
    NetConnection::quit();
    delete p::dispatcher;
    delete p::aequeue;
    delete p::ccmap;
    delete pc::input;
    delete pc::sidebar;
    delete pc::cursor;
    delete ps::aiplugman;
}

/** Play the mission. */
void Game::play()
{
    VQAMovie *mov;
    // Start the music
    SDL_LockAudio();
    pc::sfxeng->playSong(p::ccmap->getMissionData().theme);
    SDL_UnlockAudio();
    pc::gfxeng->setupCurrentGame();
    // Jump to start location
    // @TODO: Jump to correct start location in multiplayer games.
    p::ccmap->restoreLocation(0);
    /* main gameloop */
    while (!pc::input->shouldQuit()) {
        // Draw the scene
        pc::gfxeng->renderScene();
        // Run scheduled events
        p::aequeue->runEvents();
        // Handle the input
        pc::input->handle();
        // Run the AI scripts
        p::ppool->runAIs();
        if (gamemode == 2) {
            // Synchronise events with server
        }
    }
    // Stop the music
    pc::sfxeng->stopSong();

    if (gamemode == 0) {
        if (p::ppool->hasWon() ) {
            try {
                mov = new VQAMovie(p::ccmap->getMissionData().winmov);
                mov->play();
                delete mov;
            } catch (VQAError&) {
            }
        } else if (p::ppool->hasLost() ) {
            try {
                mov = new VQAMovie(p::ccmap->getMissionData().losemov);
                mov->play();
                delete mov;
            } catch (VQAError&) {
            }
        }
    }
    dumpstats();
}

void Game::dumpstats()
{
    Player* pl;
    Uint8 h,m,s,i;
    Uint32 uptime = p::aequeue->getElapsedTime();
    uptime /= 1000;
    h = uptime/3600;
    uptime %= 3600;
    m = uptime/60;
    s = uptime%60;
    logger->renderGameMsg(false);
    logger->gameMsg("Time wasted: %i hour%s%i minute%s%i second%s",
                    h,(h!=1?"s ":" "),m,(m!=1?"s ":" "),s,(s!=1?"s ":" "));
    for (i = 0; i < p::ppool->getNumPlayers(); i++) {
        pl = p::ppool->getPlayer(i);
        logger->gameMsg("%s\nUnit kills:  %i\n     losses: %i\n"
                        "Structure kills:  %i\n          losses: %i\n",
                        pl->getName(),pl->getUnitKills(),pl->getUnitLosses(),
                        pl->getStructureKills(),pl->getStructureLosses());
    }
}
