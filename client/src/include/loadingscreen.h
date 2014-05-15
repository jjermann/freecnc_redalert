#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include "SDL.h"
#include "SDL_thread.h"

class GraphicsEngine;
class CPSImage;

class LoadingScreen
{
public:
    LoadingScreen();
    ~LoadingScreen();
    void setCurrentTask(const char* task);
    void increaseThingsDone();
    int getThingsDone()
    {
        return thingsDone;
    }
    char* getCurrentTask()
    {
        return buff;
    }
private:
    // Non-copyable
    LoadingScreen(const LoadingScreen&) {}
    LoadingScreen& operator=(const LoadingScreen&) {return *this;}

    static int runRenderThread(void *inst);
    SDL_Thread *renderThread;
    SDL_mutex *lsmutex;
    CPSImage* logo;
    bool done;
    int thingsDone;
    char buff[128];
};

#endif /* LOADINGSCREEN_H */
