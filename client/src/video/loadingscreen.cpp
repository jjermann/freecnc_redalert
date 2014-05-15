#include "common.h"
#include "config.h"
#include "cpsimage.h"
#include "graphicsengine.h"
#include "loadingscreen.h"
#include "logger.h"

/// @TODO Included only for ImageNotFound.
#include "shpimage.h"

/** @TODO Abstract away the dependencies on video stuff so we can use
 * LoadingScreen as part of the server's console output.
 */
LoadingScreen::LoadingScreen()
{
    done = false;
    thingsDone = 0;
    lsmutex = SDL_CreateMutex();
    memset(buff,0,128);
    try {
        logo = new CPSImage("title.cps",1);
    } catch (ImageNotFound&) {
        logger->error("Couldn't load startup graphic\n");
        logo = 0;
    }
    renderThread = SDL_CreateThread(LoadingScreen::runRenderThread, this);
}

LoadingScreen::~LoadingScreen()
{
    int stat;
    while(SDL_mutexP(lsmutex)==-1) {
        logger->warning("Couldn't lock mutex\n");
    }

    done = true;

    while(SDL_mutexV(lsmutex)==-1) {
        logger->warning("Couldn't unlock mutex\n");
    }
    SDL_WaitThread(renderThread, &stat);
    SDL_DestroyMutex(lsmutex);
    delete logo;
}

int LoadingScreen::runRenderThread(void *inst)
{
    bool isDone = false;
    SDL_Event event;
    LoadingScreen *instance = (LoadingScreen*)inst;

    while( !isDone ) {
        while(SDL_mutexP(instance->lsmutex)==-1) {
            logger->warning("Couldn't lock mutex\n");
        }

        //render the frame here
        if (instance->logo == 0) {
            pc::gfxeng->renderLoading(instance->buff,0);
        } else {
            pc::gfxeng->renderLoading(instance->buff,instance->logo->getImage());
        }
        isDone = instance->done;

        while(SDL_mutexV(instance->lsmutex)==-1) {
            logger->warning("Couldn't unlock mutex\n");
        }
        // Limit fps to ~10
        while ( SDL_PollEvent(&event) ) {}
        SDL_Delay(500);
    }
    return 0;
}

void LoadingScreen::increaseThingsDone()
{
    while(SDL_mutexP(lsmutex)==-1) {
        logger->warning("Couldn't lock mutex\n");
    }
    thingsDone++;
    while(SDL_mutexV(lsmutex)==-1) {
        logger->warning("Couldn't unlock mutex\n");
    }
}

void LoadingScreen::setCurrentTask(const char* task)
{
    while(SDL_mutexP(lsmutex)==-1) {
        logger->warning("Couldn't lock mutex\n");
    }
    strncpy(buff,task,128);
    thingsDone = 0;
    while(SDL_mutexV(lsmutex)==-1) {
        logger->warning("Couldn't unlock mutex\n");
    }
}

