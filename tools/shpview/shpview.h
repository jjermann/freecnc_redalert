#include "SDL.h"
class SHPImage;
class Font;
class SHPImage;

class SHPView
{
private:
    Font* fnt;
    MiniGFX* gfx;
    SHPImage* shp;
    Dune2Image* d2i;
    TemplateImage* tem;
    Uint16 max;
    Uint8 mode;
public:
    void run(Uint32 iframe);
    void run_anim(Uint32 sframe, Uint32 eframe);
    SHPView(char* shpname,int umode);
    ~SHPView();
};
