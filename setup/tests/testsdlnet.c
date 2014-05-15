#include <stdlib.h>
#include "SDL_net.h"

int main(int argc, char** argv)
{
    SDLNet_Init();
    SDLNet_Quit();
    if (getenv("TEST_FAKENONET")) {
        return 1;
    }
    return 0;
}

