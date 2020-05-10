// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//  AutoMap module.
//
//-----------------------------------------------------------------------------

#pragma once

#include "p_local.h"
#include "m_cheat.h"
#include "event.h"

// Used by ST StatusBar stuff.
#define MSGHEADER (('a' << 24) + ('m' << 16))
#define MSGENTERED (MSGHEADER | ('e' << 8))
#define MSGEXITED (MSGHEADER | ('x' << 8))

// scale on entry
#define INITSCALEMTOF (.2 * FRACUNIT)
#define AM_NUMMARKPOINTS 10

extern bool viewactive;
//extern byte screens[][SCREENWIDTH*SCREENHEIGHT];

class AutoMap
{

public:
    // Called by main loop.
    bool Responder(Event *ev);

    // Called by main loop.
    void Ticker();

    // Called by main loop,
    // called instead of view drawer if automap active.
    void Drawer();

    // Called to force the automap to quit
    // if the level is completed while it is up.
    void Stop();

private:
    struct FPoint
    {
        int x, y;
    };
    struct FLine
    {
        FPoint a, b;
    };
    struct MPoint
    {
        fixed_t x, y;
    };
    struct MLine
    {
        MPoint a, b;
    };
    struct ISlope
    {
        fixed_t slp, islp;
    };

private:
//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8 * PLAYERRADIUS) / 7)
    MLine player_arrow[7] = {
        {{-R + R / 8, 0}, {R, 0}},    // -----
        {{R, 0}, {R - R / 2, R / 4}}, // ----->
        {{R, 0}, {R - R / 2, -R / 4}},
        {{-R + R / 8, 0}, {-R - R / 8, R / 4}}, // >---->
        {{-R + R / 8, 0}, {-R - R / 8, -R / 4}},
        {{-R + 3 * R / 8, 0}, {-R + R / 8, R / 4}}, // >>--->
        {{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 4}}};
#undef R
#define R ((8 * PLAYERRADIUS) / 7)
    MLine cheat_player_arrow[16] = {
        {{-R + R / 8, 0}, {R, 0}},    // -----
        {{R, 0}, {R - R / 2, R / 6}}, // ----->
        {{R, 0}, {R - R / 2, -R / 6}},
        {{-R + R / 8, 0}, {-R - R / 8, R / 6}}, // >----->
        {{-R + R / 8, 0}, {-R - R / 8, -R / 6}},
        {{-R + 3 * R / 8, 0}, {-R + R / 8, R / 6}}, // >>----->
        {{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 6}},
        {{-R / 2, 0}, {-R / 2, -R / 6}}, // >>-d--->
        {{-R / 2, -R / 6}, {-R / 2 + R / 6, -R / 6}},
        {{-R / 2 + R / 6, -R / 6}, {-R / 2 + R / 6, R / 4}},
        {{-R / 6, 0}, {-R / 6, -R / 6}}, // >>-dd-->
        {{-R / 6, -R / 6}, {0, -R / 6}},
        {{0, -R / 6}, {0, R / 4}},
        {{R / 6, R / 4}, {R / 6, -R / 7}}, // >>-ddt->
        {{R / 6, -R / 7}, {R / 6 + R / 32, -R / 7 - R / 32}},
        {{R / 6 + R / 32, -R / 7 - R / 32}, {R / 6 + R / 10, -R / 7}}};
#undef R
#define R (FRACUNIT)
    MLine triangle_guy[3] = {
        {{INT(-.867 * R), INT(-.5 * R)}, {INT(.867 * R), INT(-.5 * R)}},
        {{INT(.867 * R), INT(-.5 * R)}, {INT(0), INT(R)}},
        {{INT(0), INT(R)}, {INT(-.867 * R), INT(-.5 * R)}}};
#undef R
#define R (FRACUNIT)
    MLine thintriangle_guy[3] = {
        {{INT(-.5 * R), INT(-.7 * R)}, {INT(R), INT(0)}},
        {{INT(R), INT(0)}, {INT(-.5 * R), INT(.7 * R)}},
        {{INT(-.5 * R), INT(.7 * R)}, {INT(-.5 * R), INT(-.7 * R)}}};
#undef R
private:
    int cheating = 0;
    int grid = 0;
    int leveljuststarted = 1; // kluge until AM_LevelInit() is called
    bool automapactive = false;
    int finit_width = SCREENWIDTH;
    int finit_height = SCREENHEIGHT - 32;

    // location of window on screen
    int f_x;
    int f_y;

    // size of window on screen
    int f_w;
    int f_h;

    int lightlev; // used for funky strobing effect
    byte *fb;     // pseudo-frame buffer
    int amclock;

    MPoint m_paninc;      // how far the window pans each tic (map coords)
    fixed_t mtof_zoommul; // how far the window zooms in each tic (map coords)
    fixed_t ftom_zoommul; // how far the window zooms in each tic (fb coords)

    fixed_t m_x, m_y;   // LL x,y where the window is on the map (map coords)
    fixed_t m_x2, m_y2; // UR x,y where the window is on the map (map coords)

    //
    // width/height of window on map (map coords)
    //
    fixed_t m_w;
    fixed_t m_h;

    // based on level size
    fixed_t min_x;
    fixed_t min_y;
    fixed_t max_x;
    fixed_t max_y;

    fixed_t max_w; // max_x-min_x,
    fixed_t max_h; // max_y-min_y

    // based on player size
    fixed_t min_w;
    fixed_t min_h;

    fixed_t min_scale_mtof; // used to tell when to stop zooming out
    fixed_t max_scale_mtof; // used to tell when to stop zooming in

    // old stuff for recovery later
    fixed_t old_m_w, old_m_h;
    fixed_t old_m_x, old_m_y;

    // old location used by the Follower routine
    MPoint f_oldloc;

    // used by MTOF to scale from map-to-frame-buffer coords
    fixed_t scale_mtof = INITSCALEMTOF;
    // used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
    fixed_t scale_ftom;

    player_t *plr; // the player represented by an arrow

    patch_t *marknums[10];               // numbers used for marking by the automap
    MPoint markpoints[AM_NUMMARKPOINTS]; // where the points are
    int markpointnum = 0;                // next point to be assigned

    int followplayer = 1; // specifies whether to follow the player around

    unsigned char cheat_amap_seq[5] = {0xb2, 0x26, 0x26, 0x2e, 0xff};
    cheatseq_t cheat_amap = {cheat_amap_seq, 0};

    bool stopped = true;
    Event st_notify;
private:
    void getIslope(MLine *ml, ISlope *is) const noexcept;
    void activateNewScale() noexcept;
    void saveScaleAndLoc() noexcept;
    void restoreScaleAndLoc() noexcept;
    void addMark() noexcept;
    void findMinMaxBoundaries() noexcept;
    void changeWindowLoc() noexcept;
    void initVariables() noexcept;
    void loadPics() noexcept;
    void unloadPics();
    void clearMarks() noexcept;
    void levelInit();
    void Start ();
    /*
     * window scale functions
     */
    void minOutWindowScale();
    void maxOutWindowScale();
    void changeWindowScale();

    void doFollowPlayer();
    void updateLightLev();
    void clearFB(int color);

    bool clipMline(MLine *ml, FLine *fl);
    void rotate(fixed_t *x, fixed_t *y, angle_t a);
    /*
     * Draw functions
     */
    void drawFline(FLine *fl, int color);
    void drawMline(MLine *ml, int color);
    void drawWalls();
    void drawGrid(int color);
    void drawLineCharacter(MLine *lineguy, int lineguylines,
                          fixed_t scale, angle_t angle, int color,
                          fixed_t x, fixed_t y);
    void drawPlayers();
    void drawThings(int colors, int colorrange);
    void drawMarks();
    void drawCrosshair(int color);
public: 
    static AutoMap& get() { 
        static AutoMap* instance = new AutoMap(); 
        return *instance; 
    }
    bool isAutoMapActive() const{
        return automapactive;
    }
    void setAutoMapActive(bool automapactive)
    {
        this->automapactive = automapactive;
    }
private:
    AutoMap() {};


};
