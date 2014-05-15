// mode: -*- C++ -*-
/*****************************************************************************
 * sidebar.h - header file for sidebar
 ****************************************************************************/
#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <vector>
#include "SDL.h"
#include "actioneventqueue.h"
#include "font.h"

class Player;

class Sidebar
{
public:
    Sidebar(Player *pl, Uint16 height, const char* theatre);
    ~Sidebar();

    bool getVisChanged();
    bool getVisible() {return visible;}
    void toggleVisible();

    SDL_Surface* getTabImage() {return tab;}
    SDL_Rect* getTabLocation() {return &tablocation;}

    void drawPowerbar();
    SDL_Surface* getSidebarImage(SDL_Rect location);
    bool isOriginalType() {return isoriginaltype;}


    Uint8 getButton(Uint16 x, Uint16 y);
    void clickButton(Uint8 index, char* unitname, Uint8* createmode);
    void resetButton();
    void redrawButton();
    void scrollSidebar(bool scrollup);
    void updateSidebar();

    void startRadarAnim(Uint8 mode, bool* minienable);

    Font* getFont() {return gamefnt;}

    class SidebarError {};

    enum sidebarop {sbo_null = 0, sbo_build = 1, sbo_scroll = 2, sbo_unit = 4,
        sbo_structure = 8, sbo_up = 16, sbo_down = 32, sbo_repair = 64,
        sbo_sell = 128, sbo_map = 256};

    struct SidebarGeometry {
        Uint32 bw,bh;
    };
    const SidebarGeometry& getGeom() {return geom;}
private:
    Sidebar() {};
    Sidebar(const Sidebar&) {};
    Sidebar& operator=(const Sidebar&) {return *this;}

    class RadarAnimEvent : public ActionEvent {
    public:
        RadarAnimEvent(Uint8 mode, bool* minienable);
        void run();
    private:
        Uint8 mode, frame, framend;
        bool* minienable;
    };

    friend class RadarAnimEvent;

    class SidebarButton {
    public:
        SidebarButton(Sint16 x, Sint16 y, const char* fname, Uint8 f,
                const char* theatre, Uint8 palnum);
        ~SidebarButton();
        void ChangeImage(const char* fname);
        void ChangeImage(const char* fname, Uint8 number);
//        SDL_Surface* getSurface() const {
//            return pic;
//        }
        void blit(SDL_Surface *dest);
        SDL_Rect getRect() const {
            return picloc;
        }
        Uint8 getFunction() const {
            return function;
        }
        static SDL_Surface* fallback(const char* fname);
        void setStatus(Uint8 num, bool grayed, bool stopped, bool frozen, bool ready, double angle);
    private:
        /* Create a surface indicating progress of a build.
           This surface has an alpha layer and will be blitted
           over the icon. */
        void drawText(SDL_Surface *dest, const char *s);
        void drawNumber(SDL_Surface *dest);
        SDL_Surface *grayButton(Uint32 color);
        SDL_Surface* pic;
        SDL_Rect picloc;
        Uint8 function, palnum;
        const char* theatre;
        Uint8 num;
        bool grayed;
        bool stopped;
        bool frozen;
        bool ready;
        double angle;
    };

    // 1 - DOS, 0 - GOLD
    bool isoriginaltype;
    Uint8 spalnum;

    void setupbuttons(Uint16 height);
    void scrollbuildlist(Uint8 dir, Uint8 type);
    void build(Uint8 index, Uint8 type, char* unitname, Uint8* createmode);
    void updateicons();
    void updateAvailableLists();
    void downbutton(Uint8 index);
    void addButton(Uint16 x, Uint16 y, const char* fname, Uint8 f, Uint8 pal);

    SDL_Surface *tab;
    SDL_Rect tablocation;

    SDL_Surface *sbar, *pbar;
    SDL_Rect sbarlocation;

    SDL_Surface *radarlogo,*radarbg;

    Font *gamefnt;

    bool visible, vischanged;

    const char* theatre;

    Uint8 buttondown,startbuildbutton;
    bool bd;

    Uint8 buildbut;
    std::vector<SidebarButton *> buttons;

    std::vector<char*> uniticons;
    std::vector<char*> structicons;

    const char* radarname;
    const char* radarback;
    RadarAnimEvent* radaranim;
    bool radaranimating;

    Uint8 unitoff,structoff; // For scrolling

    Player *player;
    int scaleq;

    INIFile *structureini;
    INIFile *unitsini;	  

    Uint8 sidebar_busy_by_vehicle;
    Uint8 sidebar_busy_by_soldier;
    Uint8 sidebar_busy_by_building;

    SidebarGeometry geom,startoffs,miniboffs;
    Uint16 scrollb,mapheight;
};

#endif /* SIDEBAR_H */
