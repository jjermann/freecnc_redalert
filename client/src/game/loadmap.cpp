/*****************************************************************************
 * loadmap.cpp - map class, the functions to load the map goes here
 ****************************************************************************/
#include <cctype>
#include "ccmap.h"
#include <stdexcept>
#include "common.h"
#include "compression.h"
#include "config.h"
#include "fcnc_endian.h"
#include "freecnc.h"
#include "game.h"
#include "inifile.h"
#include "imagecache.h"
#include "logger.h"
#include "netconnection.h"
#include "playerpool.h"
#include "shpimage.h"
#include "unitandstructurepool.h"
#include "vfs.h"

#if _MSC_VER && _MSC_VER < 1300
using namespace std;
#else
using std::map;
using std::runtime_error;
using std::string;
#endif

/** Loads the maps ini file containing info on dimensions, units, trees
 * and so on.
 */

void CnCMap::loadIni()
{
    ConfigType config;
    INIFile *inifile;
    string tmpname = missionData.mapname;
    config = getConfig();
    tmpname += ".INI";
    // Load the INIFile
    try {
        inifile = new INIFile(tmpname.c_str());
    } catch (runtime_error& e) {
        logger->error("Map \"%s\" not found.  Check your installation.\n", tmpname.c_str());
        throw LoadMapError();
    }

    p::ppool = new PlayerPool(inifile, gamemode);

    simpleSections(inifile);

    try {
        p::uspool = new UnitAndStructurePool();
    } catch (int) {
        throw LoadMapError();
    }

    if (gamemode == 0) {
        p::ppool->setLPlayer(missionData.player);
    }
    terraintypes.resize(width*height, 0);
    resourcematrix.resize(width*height, 0);

    advancedSections(inifile);

    if (maptype == GAME_RA)
        unMapPack(inifile);

    // spawn player starts.
    if (gamemode > 0) {
        for (Uint8 i=0;i<config.totalplayers;++i) {
            tmpname = "multi" + fcnc_lexical_cast<string>(i+1);
            p::ppool->getPlayerNum(tmpname.c_str());
            if ((i+1) == config.playernum) {
                p::ppool->setLPlayer(config.playernum,config.nick.c_str(),config.side_colour.c_str(),config.mside.c_str());
            }
        }
        p::ppool->placeMultiUnits();
    }
    try {
        pips = new SHPImage("hpips.shp",mapscaleq);
    } catch(ImageNotFound&) {
        try {
            pips = new SHPImage("pips.shp", mapscaleq);
        } catch(ImageNotFound&) {
            logger->error("Unable to load the pips graphics!\n");
            throw LoadMapError();
        }
    }
    pipsnum = pc::imagepool->size()<<16;
    pc::imagepool->push_back(pips);
    if (maptype == GAME_RA) {
        try {
            char moveflsh[13] = "moveflsh.";
            strncat( moveflsh, missionData.theater, 3 );
            moveflash = new SHPImage(moveflsh,mapscaleq);
        } catch (ImageNotFound&) {
            logger->error("Unable to load the movement acknowledgement pulse graphic\n");
            throw LoadMapError();
        }
    } else {
        try {
            moveflash = new SHPImage("moveflsh.shp",mapscaleq);
        } catch (ImageNotFound&) {
            logger->error("Unable to load the movement acknowledgement pulse graphic\n");
            throw LoadMapError();
        }
    }
    flashnum = pc::imagepool->size()<<16;
    pc::imagepool->push_back(moveflash);
}


/** Function to load all vars in the simple sections of the inifile
 * @param pointer to the inifile
 */
void CnCMap::simpleSections(INIFile* inifile) {
    const char* key;
    Uint8 iter = 0;

    // The strings in the basic section
    static const char* strreads[] = {
        "BRIEF", "ACTION", "PLAYER", "THEME", "WIN", "LOSE", 0
    };
    static char** strvars[] = {
        &missionData.brief, &missionData.action, &missionData.player,
        &missionData.theme, &missionData.winmov, &missionData.losemov,
    };
    static const char* intreads[] = {
        "HEIGHT", "WIDTH", "X", "Y", 0,
    };
    static Uint16* intvars[]  = {&height, &width, &x, &y};

    while (strreads[iter] != 0) {
        char** variable;
        key = strreads[iter];
        variable = strvars[iter];
        *variable = inifile->readString("BASIC", key);
        if (0 == *variable) {
            logger->error("Error loading map: missing \"%s\"\n", key);
            throw LoadMapError();
        }
        ++iter;
    }

    iter = 0;
    while (intreads[iter] != 0) {
        int temp;
        Uint16* variable;
        key      = intreads[iter];
        variable = intvars[iter];

        temp = inifile->readInt("MAP", key);
        if (INIERROR == temp) {
            logger->error("Error loading map: unable to find \"%s\"\n",key);
            throw LoadMapError();
        }

        *variable = temp;
        ++iter;
    }

    missionData.theater = inifile->readString("MAP", "THEATER");
    if (0 == missionData.theater) {
        logger->error("Error loading map: unable to find \"THEATER\"\n");
        throw LoadMapError();
    }

    missionData.buildlevel = inifile->readInt("BASIC", "BUILDLEVEL",1);
}

/** Function to load all the advanced sections in the inifile.
 * @param a pointer to the inifile
 */
void CnCMap::advancedSections(INIFile *inifile) {
    Uint32 keynum;
    int i;
    //char *line;
    char shpname[128];
    char trigger[128];
    char action[128];
    char type[128];
    char owner[128];
    int facing, health, subpos;
    int linenum, smudgenum, tmpval;
    Uint16 tx, ty, xsize, ysize, tmp2;
    std::map<std::string, Uint32> imagelist;
    std::map<std::string, Uint32>::iterator imgpos;
    SHPImage *image;
    TerrainEntry tmpterrain;
    INIKey key;

    Uint16 xwalk, ywalk, ttype;

    try {
        for (keynum = 0; ; keynum++) {
            key = inifile->readKeyValue("WAYPOINTS", keynum);
            if (sscanf(key->first.c_str(), "%d", &tmpval) == 1) {
                if (tmpval == 26) { /* waypoint 26 is the startpos of the map */
                    tmp2 = (Uint16)atoi(key->second.c_str());
                    //waypoints.push_back(tmp2);
                    translateCoord(tmp2, &tx, &ty);
                    scrollbookmarks[0].x = tx-x;
                    scrollbookmarks[0].y = ty-y;
                }
                if (tmpval < 8) {
                    tmp2 = (Uint16)atoi(key->second.c_str());
                    waypoints.push_back(tmp2);
                }
            }
        }
    } catch(int) {}
    p::ppool->setWaypoints(waypoints);

    INIFile *arts = new INIFile("art.ini");

    /* load the shadowimages */
    try {
        image = new SHPImage("SHADOW.SHP", mapscaleq);
        numShadowImg = image->getNumImg();
        shadowimages.resize(numShadowImg);
        for( i = 0; i < 48; i++ ) {
            image->getImageAsAlpha(i, &shadowimages[i]);
        }
        delete image;
    } catch(ImageNotFound&) {
        logger->warning("Unable to load \"shadow.shp\"\n");
        numShadowImg = 0;
    }
    /* load the smudge marks and the tiberium to the imagepool */
    if (strncasecmp(missionData.theater, "INT", 3) != 0) {
        string sname;
        if (maptype == GAME_TD) {
            sname = "TI1";
        } else if (maptype == GAME_RA) {
            sname = "GOLD01";
        } else {
            logger->error("Unsuported maptype\n");
            throw LoadMapError();
        }

        resourcenames[sname] = 0;
        sname += "." + string(missionData.theater,3);
        try {
            image = new SHPImage(sname.c_str(), mapscaleq);
            resourcebases.push_back(pc::imagepool->size());
            pc::imagepool->push_back(image);
        } catch (ImageNotFound&) {
            logger->error("Could not load \"%s\"\n",sname.c_str());
            throw LoadMapError();
        }
        // No craters or scorch marks for interior?
        for (i = 1; i <= 6; i++) {
            sprintf(shpname, "SC%d.", i);
            strncat(shpname, missionData.theater, 3);
            try {
                image = new SHPImage(shpname, mapscaleq);
            } catch (ImageNotFound&) {continue;}
            pc::imagepool->push_back(image);
        }
        for (i = 1; i <= 6; i++) {
            sprintf(shpname, "CR%d.", i);
            strncat(shpname, missionData.theater, 3);
            try {
                image = new SHPImage(shpname, mapscaleq);
            } catch (ImageNotFound&) {continue;}
            pc::imagepool->push_back(image);
        }
    }

    overlaymatrix.resize(width*height, 0);

    try {
        for( keynum = 0; ;keynum++ ) {
            bool bad = false;
            key = inifile->readKeyValue("TERRAIN", keynum);
            /* , is the char which separate terraintype from action. */

            if( sscanf(key->first.c_str(), "%d", &linenum) == 1 &&
                    sscanf(key->second.c_str(), "%[^,],", shpname) == 1 ) {
                /* Set the next entry in the terrain vector to the correct values.
                 * the map-array and shp files vill be set later */
                translateCoord(linenum, &tx, &ty);

                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }

                if( shpname[0] == 't' || shpname[0] == 'T' )
                    ttype = t_tree;
                else if( shpname[0] == 'r' || shpname[0] == 'R' )
                    ttype = t_rock;
                else
                    ttype = t_other_nonpass;

                /* calculate the new pos based on size and blocked */
                xsize = arts->readInt(shpname, "XSIZE",1);
                ysize = arts->readInt(shpname, "YSIZE",1);

                for( ywalk = 0; ywalk < ysize && ywalk + ty < height+y; ywalk++ ) {
                    for( xwalk = 0; xwalk < xsize && xwalk + tx < width + x; xwalk++ ) {
                        sprintf(type, "NOTBLOCKED%d", ywalk*xsize+xwalk);
                        if( arts->readInt(shpname, type) == INIERROR ) {
                            terraintypes[(ywalk+ty-y)*width+xwalk+tx-x] = ttype;
                        }
                    }
                }

                linenum = xsize*ysize;
                do {
                    if (linenum == 0) {
                        logger->error("BUG: Could not find an entry in art.ini for %s\n",shpname);
                        bad = true;
                        break;
                    }
                    linenum--;
                    sprintf(type, "NOTBLOCKED%d", linenum);
                } while(arts->readInt(shpname, type) == INIERROR);
                if (bad)
                    continue;

                tmpterrain.xoffset = -(linenum%ysize)*24;
                tmpterrain.yoffset = -(linenum/ysize)*24;

                tx += linenum%ysize;
                if( tx >= width+x ) {
                    tmpterrain.xoffset += 1+tx-(width+x);
                    tx = width+x-1;
                }
                ty += linenum/ysize;
                if( ty >= height+y ) {
                    tmpterrain.yoffset += 1+ty-(height+y);
                    ty = height+y-1;
                }

                linenum = normaliseCoord(tx, ty);
                strcat(shpname, ".");
                strncat(shpname, missionData.theater, 3);

                /* search the map for the image */
                imgpos = imagelist.find(shpname);

                /* set up the overlay matrix and load some shps */
                if( imgpos != imagelist.end() ) {
                    /* this tile already has a number */
                    overlaymatrix[linenum] |= HAS_TERRAIN;
                    tmpterrain.shpnum = imgpos->second << 16;
                    terrains[linenum] = tmpterrain;
                } else {
                    /* a new tile */
                    imagelist[shpname] = pc::imagepool->size();
                    overlaymatrix[linenum] |= HAS_TERRAIN;
                    tmpterrain.shpnum = pc::imagepool->size()<<16;
                    terrains[linenum] = tmpterrain;
                    try {
                        image = new SHPImage(shpname, mapscaleq);
                    } catch (ImageNotFound&) {
                        logger->error("Could not load \"%s\"\n", shpname);
                        throw LoadMapError();
                    }
                    pc::imagepool->push_back(image);
                }
            }
        }
    } catch(int) {}

    if (maptype == GAME_RA){
        unOverlayPack(inifile);
    } else {
        loadOverlay(inifile);
    }

// @TODO: Fix multiplayer map loading (at the moment it returns 1 all the time)
//    p::uspool->preloadUnitAndStructures(missionData.buildlevel);
    p::uspool->preloadUnitAndStructures(98);
    p::uspool->generateProductionGroups();

    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("SMUDGE", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &linenum) == 1 &&
                    sscanf(key->second.c_str(), "SC%d", &smudgenum) == 1 ) {
                translateCoord(linenum, &tx, &ty);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                overlaymatrix[linenum] |= (smudgenum<<4);
            } else if( sscanf(key->first.c_str(), "%d", &linenum) == 1 &&
                       sscanf(key->second.c_str(), "CR%d", &smudgenum) == 1 ) {
                //} else if( sscanf(line, "%d=CR%d", &linenum, &smudgenum) == 2 ) {
                translateCoord(linenum, &tx, &ty);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }

                linenum = (ty-y)*width + tx - x;
                overlaymatrix[linenum] |= ((smudgenum+6)<<4);
            }
        }
    } catch(int) {}

    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("STRUCTURES", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &tmpval) == 1 &&
                    sscanf(key->second.c_str(), "%[^,],%[^,],%d,%d,%d,%s", owner, type,
                           &health, &linenum, &facing, trigger ) == 6  ) {
                translateCoord(linenum, &tx, &ty);
                facing = min(31,facing>>3);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                p::uspool->createStructure(type, linenum, p::ppool->getPlayerNum(owner),
                        health, facing, false);
            }
        }
    } catch(int) {}

    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("UNITS", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &tmpval) == 1 &&
                    sscanf(key->second.c_str(), "%[^,],%[^,],%d,%d,%d,%[^,],%s", owner, type,
                           &health, &linenum, &facing, action, trigger ) == 7  ) {
                translateCoord(linenum, &tx, &ty);
                facing = min(31,facing>>3);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                p::uspool->createUnit(type, linenum, 5, p::ppool->getPlayerNum(owner),
                        health, facing);
            }
        }
    } catch(int) {}

    /*infantry*/
    try {
        for( keynum = 0;;keynum++ ) {
            key = inifile->readKeyValue("INFANTRY", keynum);
            /* , is the char which separate terraintype from action. */
            if( sscanf(key->first.c_str(), "%d", &tmpval ) == 1  &&
                    sscanf(key->second.c_str(), "%[^,],%[^,],%d,%d,%d,%[^,],%d,%s", owner, type,
                           &health, &linenum, &subpos, action, &facing, trigger ) == 8  ) {
                translateCoord(linenum, &tx, &ty);
                facing = min(31,facing>>3);
                if( tx < x || ty < y || tx > x+width || ty > height+y ) {
                    continue;
                }
                linenum = (ty-y)*width + tx - x;
                p::uspool->createUnit(type, linenum, subpos, p::ppool->getPlayerNum(owner),
                        health, facing);
            }
        }
    } catch(int) {}
    delete arts;
}



/////// Bin loading routines

struct tiledata
{
    Uint32 image;
    Uint8 type;
};

void CnCMap::loadBin()
{
    Uint32 index = 0;
    //    Uint8 templ, tile;
    int xtile, ytile;
    VFile *binfile;
    char *binname = new char[strlen(missionData.mapname)+5];

    TileList *mapdata;

    mapdata = new TileList[width*height];

    /* Calculate name of bin file ( mapname.bin ). */
    strcpy(binname, missionData.mapname);
    strcat(binname, ".BIN");

    /* get the offset and size of the binfile along with a pointer to it */
    //binfile = mixes->getOffsetAndSize(binname, &offset, &size);
    binfile = VFS_Open(binname);
    delete[] binname;

    if(binfile == NULL) {
        logger->error("Unable to locate BIN file!\n");
        throw LoadMapError();
    }

    /* Seek the beginning of the map.
     * It's at begining of bin + maxwidth * empty y cells + empty x cells
     * times 2 sinse each entry is 2 bytes
     */
    binfile->seekSet( (64*y + x) * 2 );

    for( ytile = 0; ytile < height; ytile++ ) {
        for( xtile = 0; xtile < width; xtile++ ) {
            Uint16 tmpread = 0;
            /* Read template and tile */
            mapdata[index].templateNum = 0;
            binfile->readByte((Uint8 *)&(tmpread), 1);
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            mapdata[index].templateNum = SDL_Swap16(tmpread);
#else
            mapdata[index].templateNum = tmpread;
#endif
            binfile->readByte(&(mapdata[index].tileNum), 1);

            index++;
        }
        /* Skip til the end of the line and the onwards to the
         * beginning of usefull data on the next line 
         */
        binfile->seekCur( 2*(64-width) );
    }
    VFS_Close(binfile);
    parseBin(mapdata);
    delete[] mapdata;
}

void CnCMap::unMapPack(INIFile* inifile)
{
    int tmpval;
    Uint32 curpos;
    int xtile, ytile;
    TileList *bindata;
    Uint32 keynum;
    INIKey key;
    Uint8 *mapdata1 = new Uint8[49152]; // 48k
    Uint8 *mapdata2 = new Uint8[49152];

    // read packed data into array
    mapdata1[0] = 0;
    try {
        for (keynum = 1;;++keynum) {
            key = inifile->readIndexedKeyValue("MAPPACK", keynum);
            strcat(((char*)mapdata1), key->second.c_str());
        }
    } catch(int) {}

    Compression::dec_base64(mapdata1,mapdata2,strlen(((char*)mapdata1)));

    /* decode the format80 coded data (6 chunks) */
    curpos = 0;
    for( tmpval = 0; tmpval < 6; tmpval++ ) {
        //printf("first vals in data is %x %x %x %x\n", mapdata2[curpos],
        //mapdata2[curpos+1], mapdata2[curpos+2], mapdata2[curpos+3]);
        if( Compression::decode80((Uint8 *)mapdata2+4+curpos, mapdata1+8192*tmpval) != 8192 ) {
            logger->warning("A format80 chunk in the \"MapPack\" was of wrong size\n");
        }
        curpos = curpos + 4 + mapdata2[curpos] + (mapdata2[curpos+1]<<8) +
                 (mapdata2[curpos+2]<<16);
    }
    delete[] mapdata2;

    /* 128*128 16-bit template number followed by 128*128 8-bit tile numbers */
    bindata = new TileList[width*height];
    tmpval = y*128+x;
    curpos = 0;
    for( ytile = 0; ytile < height; ytile++ ) {
        for( xtile = 0; xtile < width; xtile++ ) {
            /* Read template and tile */
            bindata[curpos].templateNum = ((Uint16 *)mapdata1)[tmpval];
            bindata[curpos].tileNum = mapdata1[tmpval+128*128*2];
            curpos++;
            tmpval++;
            /*  printf("tile %d, %d\n", bindata[curpos-1].templateNum,
                 bindata[curpos-1].tileNum); */
        }
        /* Skip until the end of the line and the onwards to the
         * beginning of usefull data on the next line 
         */
        tmpval += (128-width);
    }
    delete[] mapdata1;
    parseBin(bindata);
    delete[] bindata;
}

void CnCMap::parseBin(TileList* bindata)
{
    Uint32 index;

    Uint16 templ;
    Uint8 tile;
    int xtile, ytile;
    INIFile *templini;

    SDL_Surface *tileimg;
    SDL_Color palette[256];

    std::map<Uint32, struct tiledata> tilelist;
    std::map<Uint32, struct tiledata>::iterator imgpos;
    //     char tempc[sizeof(Uint8)];

    struct tiledata tiledata;
    Uint32 tiletype;
    tilematrix.resize(width*height);

    loadPal(palette);
    SHPBase::setPalette(palette);
    SHPBase::calculatePalettes();

    /* Load the templates.ini */
    try {
        templini = new INIFile("templates.ini");
    } catch(int) {
        logger->error("Could not load the templates file\n");
        throw LoadMapError();
    }

    index = 0;
    for( ytile = 0; ytile < height; ytile++ ) {
        for( xtile = 0; xtile < width; xtile++ ) {
            /* Read template and tile */
            templ = bindata[index].templateNum;
            tile  = bindata[index].tileNum;
            index++;
            /* Template 0xff is an empty tile */
            if(templ == ((maptype == GAME_RA)?0xffff:0xff)) {
                templ = 0;
                tile = 0;
            }

            /* Code sugested by Olaf van der Spek to cause all tiles in template
             * 0 and 2 to be used */
            if( templ == 0)
                tile = xtile&3 | (ytile&3 << 2);
            else if( templ == 2 )
                tile = xtile&1 | (ytile&1 << 1);


            imgpos = tilelist.find(templ<<8 | tile);

            /* set up the tile matrix and load some tiles */
            if( imgpos != tilelist.end() ) {
                /* this tile already has a number */
                tilematrix[width*ytile+xtile] = imgpos->second.image;
                if( terraintypes[width*ytile+xtile] == 0 )
                    terraintypes[width*ytile+xtile] = imgpos->second.type;
            } else {
                /* a new tile */
                tileimg = loadTile(templini, templ, tile, &tiletype);

                tiledata.image = tileimages.size();
                tiledata.type = tiletype;
                tilelist[templ<<8 | tile] = tiledata;
                tilematrix[width*ytile+xtile] = tileimages.size();
                if( terraintypes[width*ytile+xtile] == 0 )
                    terraintypes[width*ytile+xtile] = tiletype;

                if( tileimg == NULL ) {
                    logger->error("Error loading tiles\n");
                    throw LoadMapError();
                }

                tileimages.push_back(tileimg);
            }

        }
    }
    delete templini;
}

/////// Overlay loading routines

void CnCMap::loadOverlay(INIFile* inifile)
{
    INIKey key;
    Uint32 linenum;
    Uint16 tx, ty;
    try {
        for (Uint32 keynum = 0;;keynum++) {
            key = inifile->readKeyValue("OVERLAY", keynum);
            if (sscanf(key->first.c_str(), "%u", &linenum) == 1) {
                translateCoord(linenum, &tx, &ty);
                if (!validCoord(tx, ty))
                    continue;
                linenum = normaliseCoord(tx, ty);

                parseOverlay(linenum, key->second);
            }
        }
    } catch(int) {}
}

const char* RAOverlayNames[] = { "SBAG", "CYCL", "BRIK", "FENC", "WOOD",
    "GOLD01", "GOLD02", "GOLD03", "GOLD04", "GEM01", "GEM02", "GEM03",
    "GEM04", "V12", "V13", "V14", "V15", "V16", "V17", "V18", "FPLS",
    "WCRATE", "SCRATE", "FENC", "SBAG" };

void CnCMap::unOverlayPack(INIFile* inifile)
{
    Uint32 curpos, tilepos;
    Uint8 xtile, ytile;
    Uint32 keynum;
    INIKey key;
    Uint8 mapdata[16384]; // 16k
    Uint8 temp[16384];

    // read packed data into array
    mapdata[0] = 0;
    try {
        for (keynum = 1;;++keynum) {
            key = inifile->readIndexedKeyValue("OVERLAYPACK", keynum);
            strcat(((char*)mapdata), key->second.c_str());
        }
    } catch(int) {}

    Compression::dec_base64(mapdata, temp, strlen(((char*)mapdata)));

    /* decode the format80 coded data (2 chunks) */
    curpos = 0;
    for (int tmpval = 0; tmpval < 2; tmpval++) {
        if (Compression::decode80((Uint8 *)temp+4+curpos, mapdata+8192*tmpval) != 8192) {
            logger->warning("A format80 chunk in the \"OverlayPack\" was of wrong size\n");
        }
        curpos = curpos + 4 + temp[curpos] + (temp[curpos+1]<<8) +
            (temp[curpos+2]<<16);
    }

    for (ytile = y; ytile <= y+height; ++ytile){
        for (xtile = x; xtile <= x+width; ++xtile){
            curpos = xtile+ytile*128;
            tilepos = xtile-x+(ytile-y)*width;
            if (mapdata[curpos] == 0xff) // No overlay
                continue;
            if (mapdata[curpos] > 0x17) // Unknown overlay type
                continue;
            parseOverlay(tilepos, RAOverlayNames[mapdata[curpos]]);
        }
    }
}


void CnCMap::parseOverlay(const Uint32& linenum, const string& name)
{
    Uint8 type, frame;
    Uint16 res;

    if (name == "BRIK" || name == "SBAG" || name == "FENC" || name == "WOOD" || name == "CYCL" || name == "BARB") {
        // Walls are structures.
        p::uspool->createStructure(name.c_str(), linenum, p::ppool->getPlayerNum("NEUTRAL"), 256, 0, false);
        return;
    }

    string shpname;
    shpname = name + '.' + string(missionData.theater, 3);
    try {
        /* Remember: imagecache's indexing format is different
         * (imagepool index << 16) | frame */
        frame = pc::imgcache->loadImage(shpname.c_str()) >> 16;
    } catch(ImageNotFound&) {
        shpname = name + ".SHP";
        try {
            frame = pc::imgcache->loadImage(shpname.c_str()) >> 16;
        } catch (ImageNotFound&) {
            logger->error("Unable to load overlay \"%s\" (or \"%s.SHP\")\n",
                    shpname.c_str(), name.c_str());
            throw LoadMapError();
        }
    }

    /// @TODO Generic resources?
    if (strncasecmp(name.c_str(),"TI",2) == 0 ||
            strncasecmp(name.c_str(),"GOLD",4) == 0 ||
            strncasecmp(name.c_str(),"GEM",3) == 0) {
        Uint32 i = 0;
        /* This is a hack to seed the map with semi-reasonable amounts of
         * resource growth.  This will hopefully become less ugly after the code
         * to manage resource growth has been written. */
        if (sscanf(name.c_str(), "TI%u", &i) == 0) {
//TODO: This needs to be done properly according to stingray's description...
//            i = atoi(name.c_str() + (name.length() - 2));
            i = 8+atoi(name.c_str() + (name.length() - 2));
            /* An even worse hack: number of frames in gems is less than the
             * number of different types of gem. */
            if ('E' == name[1]) i = 3;
        }
        if (0 == i) {
            logger->error("Resource hack for \"%s\" failed.", name.c_str());
            throw LoadMapError();
        }
        map<string, Uint8>::iterator t = resourcenames.find(name);
        if (resourcenames.end() == t) {
            type = resourcebases.size();
            /* Encode the type and amount data into the resource matrix's new
             * cell. */
            res = type | ((i-1) << 8);
            resourcenames[name] = type;
            resourcebases.push_back(frame);
        } else {
            res = t->second | ((i-1) << 8);
        }
        resourcematrix[linenum] = res;
    } else {
        overlaymatrix[linenum] |= HAS_OVERLAY;
        overlays[linenum] = frame;

        if (toupper(name[0]) == 'T')
            terraintypes[linenum] = t_tree;
        else if (toupper(name[0]) == 'R')
            terraintypes[linenum] = t_rock;
        else
            terraintypes[linenum] = t_other_nonpass;
    }
}

/** Load a palette
 * @param palette array of SDL_Colour into which palette is loaded.
 */

void CnCMap::loadPal(SDL_Color *palette)
{
    VFile *palfile;
    int i;
    string palname = missionData.theater;
    if (palname.length() > 8) {
        palname.insert(8, ".PAL");
    } else {
        palname += ".PAL";
    }
    /* Seek the palette file in the mix */
    palfile = VFS_Open(palname.c_str());

    if (palfile == NULL) {
        logger->error("Unable to locate palette (\"%s\").\n", palname.c_str());
        throw LoadMapError();
    }

    /* Load the palette */
    for (i = 0; i < 256; i++) {
        palfile->readByte(&palette[i].r, 1);
        palfile->readByte(&palette[i].g, 1);
        palfile->readByte(&palette[i].b, 1);
        palette[i].r <<= 2;
        palette[i].g <<= 2;
        palette[i].b <<= 2;
    }
    VFS_Close(palfile);
}



/** load a tile from the mixfile.
 * @param the mixfiles.
 * @param the template inifile.
 * @param the template number.
 * @param the tilenumber.
 * @returns a SDL_Surface containing the tile.
 */
SDL_Surface *CnCMap::loadTile(INIFile *templini, Uint16 templ, Uint8 tile, Uint32* tiletype)
{

    TemplateImage *theaterfile;

    SDL_Surface *retimage;


    char tilefilename[13];
    char tilenum[11];
    char *temname;


    /* The name of the file containing the template is something from
     * templates.ini . the three first
     * chars in the name of the theater eg. .DES .TEM .WIN */

    sprintf(tilefilename, "TEM%d", templ);
    sprintf(tilenum, "tiletype%d", tile);
    *tiletype = templini->readInt(tilefilename, tilenum, 0);

    temname = templini->readString(tilefilename, "NAME");

    if( temname == NULL ) {
        logger->warning("Error in templates.ini! (can't find \"%s\")\n", tilefilename);
        strcpy(tilefilename, "CLEAR1");
    } else {
        strcpy(tilefilename, temname);
        delete[] temname;
    }

    strcat( tilefilename, "." );
    strncat( tilefilename, missionData.theater, 3 );


    /* Find and seek the template file in the mix */

    try {
        if( maptype == GAME_RA ) {
            theaterfile = new TemplateImage(tilefilename, mapscaleq, 1);
        } else {
            theaterfile = new TemplateImage(tilefilename, mapscaleq);
        }
    } catch(ImageNotFound&) {
        logger->warning("Unable to locate template %d, %d (\"%s\") in mix! using tile 0, 0 instead\n", templ, tile, tilefilename);
        if (templ == 0 && tile == 0) {
            logger->error("Unable to load tile 0,0.  Can't proceed\n");
            return NULL;
        }
        return loadTile( templini, 0, 0, tiletype );
    }


    retimage = theaterfile->getImage(tile);
    if( retimage == NULL ) {
        logger->warning("Illegal template %d, %d (\"%s\")! using tile 0, 0 instead\n", templ, tile, tilefilename);
        if( templ == 0 && tile == 0 )
            return NULL;
        return loadTile( templini, 0, 0, tiletype );
    }
    delete theaterfile;
    return retimage;

}

