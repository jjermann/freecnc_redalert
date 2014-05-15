#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "SDL.h"
#include "vfs.h"
#include "font.h"
#include "shpimage.h"
#include "inifile.h"
#include "minigfx.h"
#include "shpview.h"
#include "common.h"
#include "logger.h"

#define SCALER -1

/* SHP view
 * Based on Tim's Template INI Editor
 */

Logger* logger;

void loadPal(char* palname);

void usage(char* biname)
{
    printf("Usage: %s <shpname.shp> <type> [framenum] [ [endframe] ]\n\n"
           "type is 0: shp 1: template 2: dune2\n"
           "Keys: right\tincrease frame by one\n"
           "      left\tdecrease frame by one\n"
           "      up\tincrease frame by ten\n"
           "      down\tdecrease frame by ten\n"
           "    escape/q\tquit\n",biname);
}

int main(int argc, char **argv)
{
    SHPView* sview;
    char* binpath, *lf;

    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }
    if ((strcasecmp(argv[1],"-h")==0)||(strcasecmp(argv[1],"-help")==0)||
            (strcasecmp(argv[1],"--help")==0)) {
        usage(argv[0]);
        exit(1);
    }
    binpath = determineBinaryLocation(argv[0]);
    lf = new char[strlen(binpath)+strlen("shpview.log")+2];
    sprintf(lf, "%s/shpview.log", binpath);
    VFS_PreInit(binpath);
    logger = new Logger(lf,0);
    delete[] lf;
    VFS_Init(binpath);
    delete[] binpath;
    loadPal("temperat.pal");
    try {
        sview = new SHPView(argv[1],atoi(argv[2]));
    } catch (int) {
        delete logger;
        VFS_Destroy();
        SDL_Quit();
        exit(1);
    }
    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        logger->error("Couldn't initialize SDL: %s\n", SDL_GetError() );
        delete sview;
        delete logger;
        VFS_Destroy();
        SDL_Quit();
        exit(1);
    }
    switch (argc) {
    case 3:
        sview->run(0);
        break;
    case 4:
        sview->run(atoi(argv[3]));
        break;
    case 5:
        sview->run_anim(atoi(argv[3]),atoi(argv[4]));
        break;
    default:
        usage(argv[0]);
        break;
    }
    delete sview;
    delete logger;
    VFS_Destroy();
    SDL_Quit();
    exit(0);
}

void loadPal(char *palname)
{
    VFile *mixfile;
    SDL_Color palette[256];
    int i;

    mixfile = VFS_Open(palname);
    /* Load the palette */
    for (i = 0; i < 256; i++) {
        mixfile->readByte(&palette[i].r, 1);
        mixfile->readByte(&palette[i].g, 1);
        mixfile->readByte(&palette[i].b, 1);

        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }
    SHPBase::setPalette(palette);

}

SHPView::SHPView(char* shpname,int umode)
{
    try {
        gfx = new MiniGFX();
    } catch (int x) {
        throw 0;
    }
    fnt = new Font("scorefnt.fnt");
    logger->note("Attempting to load shape \"%s\"...",shpname);
    shp = NULL;
    tem = NULL;
    d2i = NULL;
    switch (umode) {
    case 0:
        try {
            shp = new SHPImage(shpname,SCALER);
            mode = 0;
            max = shp->getNumImg();
        } catch (ImageNotFound&) {
            logger->error("Could not open \"%s\" as a SHP\n",shpname);
            throw 0;
        }
        break;
    case 1:
        try {
            tem = new TemplateImage(shpname,SCALER);
            mode = 1;
            max = 100;
        } catch (ImageNotFound&) {
            logger->error("Could not open \"%s\" as a template\n",shpname);
            throw 0;
        }
        break;
    case 2:
        try {
            d2i = new Dune2Image(shpname,SCALER);
            mode = 2;
            max = 500;
        } catch (ImageNotFound&) {
            logger->error("Could not open \"%s\" as a dune2\n",shpname);
            throw 0;
        }
        break;
    default:
        logger->error("invalid mode\n");
        throw 0;
        break;
    }
    logger->note("Done\n");
}

SHPView::~SHPView()
{
    delete fnt;
    delete gfx;
    delete shp;
}

void SHPView::run_anim(Uint32 sframe, Uint32 eframe)
{
    SDL_Event event;
    Uint8 done = 0;

    Uint32 frame = sframe;
    if (frame > max) {
        logger->warning("Supplied start frame was greater than number of frames\n");
        sframe = frame = 0;
    }
    if (eframe > max) {
        logger->warning("Supplied end frame was greater than number of frames\n");

        eframe = max-1;
    }
    if (eframe < sframe) {
        eframe ^= sframe;
        sframe ^= eframe;
        eframe ^= sframe;
        frame = sframe;
    }
    while(!done) {
        if(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_KEYDOWN:
                if( event.key.state != SDL_PRESSED )
                    break;
                switch( event.key.keysym.sym ) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    done = 1;
                    break;
                default:
                    break;
                }
                break;
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
        }
        if (frame < eframe) {
            ++frame;
        } else {
            frame = sframe;
        }
        switch (mode) {
        case 0:
            gfx->draw(shp, fnt, frame,max);
            break;
        case 1:
            gfx->draw(tem, fnt, frame,max);
            break;
        case 2:
            gfx->draw(d2i, fnt, frame,max);
            break;
        default:
            logger->error("Unknown mode: %i\n",mode);
            done = 1;
            break;
        }

        SDL_Delay(100);
    }
}


void SHPView::run(Uint32 iframe)
{
    SDL_Event event;
    Uint8 done = 0;
    bool image_updated = true;
    Uint32 frame = iframe;
    if (frame > max) {
        logger->note("Supplied frame was greater than number of frames\n");
        frame = 0;
    }
    while(!done) {
        if(SDL_PollEvent(&event)) {
            switch(event.type) {
            case SDL_KEYDOWN:
                if( event.key.state != SDL_PRESSED )
                    break;
                switch( event.key.keysym.sym ) {
                case SDLK_RIGHT:
                    if (frame < ((Uint32)(max-1))) {
                        frame += 1;
                        image_updated = true;
                    }
                    break;
                case SDLK_LEFT:
                    if (frame > 0) {
                        frame -= 1;
                        image_updated = true;
                    }
                    break;
                case SDLK_UP:
                    if (max > 10) {
                        if (frame < (Uint32((max-11)))) {
                            frame += 10;
                        } else {
                            frame = (Uint32)(max - 1);
                        }
                    } else {
                        frame = (Uint32)(max - 1);
                    }
                    image_updated = true;
                    break;
                case SDLK_DOWN:
                    if (frame > 10) {
                        frame -= 10;
                    } else {
                        frame = 0;
                    }
                    image_updated = true;
                    break;
                case SDLK_ESCAPE:
                case SDLK_q:
                    done = 1;
                    break;
                default:
                    break;
                }
                break;
            case SDL_QUIT:
                done = 1;
                break;
            default:
                break;
            }
            if( image_updated ) {
                switch (mode) {
                case 0:
                    gfx->draw(shp, fnt, frame,max);
                    break;
                case 1:
                    gfx->draw(tem, fnt, frame,max);
                    break;
                case 2:
                    gfx->draw(d2i, fnt, frame,max);
                    break;
                default:
                    logger->error("Unknown mode: %i\n",mode);
                    done = 1;
                    break;
                }
                image_updated = false;
            }
        }
    }
}


