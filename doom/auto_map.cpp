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
//
// $Log:$
//
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------

#include <stdio.h>

#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "m_cheat.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"

#include "auto_map.h"

#include "event.h"

// For use if I do walls with outsides/insides
constexpr const int REDS = (256 - 5 * 16);
constexpr const int REDRANGE = 16;
constexpr const int BLUES = (256 - 4 * 16 + 8);
constexpr const int BLUERANGE = 8;
constexpr const int GREENS = (7 * 16);
constexpr const int GREENRANGE = 16;
constexpr const int GRAYS = (6 * 16);
constexpr const int GRAYSRANGE = 16;
constexpr const int BROWNS = (4 * 16);
constexpr const int BROWNRANGE = 16;
constexpr const int YELLOWS = (256 - 32 + 7);
constexpr const int YELLOWRANGE = 1;
constexpr const int BLACK = 0;
constexpr const int WHITE = (256 - 47);

// Automap colors
constexpr const int BACKGROUND = BLACK;
constexpr const int YOURCOLORS = WHITE;
constexpr const int YOURRANGE = 0;
constexpr const int WALLCOLORS = REDS;
constexpr const int WALLRANGE = REDRANGE;
constexpr const int TSWALLCOLORS = GRAYS;
constexpr const int TSWALLRANGE = GRAYSRANGE;
constexpr const int FDWALLCOLORS = BROWNS;
constexpr const int FDWALLRANGE = BROWNRANGE;
constexpr const int CDWALLCOLORS = YELLOWS;
constexpr const int CDWALLRANGE = YELLOWRANGE;
constexpr const int THINGCOLORS = GREENS;
constexpr const int THINGRANGE = GREENRANGE;
constexpr const int SECRETWALLCOLORS = WALLCOLORS;
constexpr const int SECRETWALLRANGE = WALLRANGE;
constexpr const int GRIDCOLORS = (GRAYS + GRAYSRANGE / 2);
constexpr const int GRIDRANGE = 0;
constexpr const int XHAIRCOLORS = GRAYS;

// drawing stuff
constexpr const int FB = 0;

constexpr const int PANDOWNKEY = KEY_DOWNARROW;
constexpr const int PANUPKEY = KEY_UPARROW;
constexpr const int PANRIGHTKEY = KEY_RIGHTARROW;
constexpr const int PANLEFTKEY = KEY_LEFTARROW;
constexpr const int ZOOMINKEY = '=';
constexpr const int ZOOMOUTKEY = '-';
constexpr const int STARTKEY = KEY_TAB;
constexpr const int ENDKEY = KEY_TAB;
constexpr const int GOBIGKEY = '0';
constexpr const int FOLLOWKEY = 'f';
constexpr const int GRIDKEY = 'g';
constexpr const int MARKKEY = 'm';
constexpr const int CLEARMARKKEY = 'c';

#define NUMMARKPOINTS 10

// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC 4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN ((int)(1.02 * FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT ((int)(FRACUNIT / 1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x) << 16), scale_ftom)
#define MTOF(x) (FixedMul((x), scale_mtof) >> 16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x) (f_x + MTOF((x)-m_x))
#define CYMTOF(y) (f_y + (f_h - MTOF((y)-m_y)))

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

#define NUMPLYRLINES (sizeof(player_arrow) / sizeof(MLine))

#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow) / sizeof(MLine))

#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy) / sizeof(MLine))
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy) / sizeof(MLine))

void V_MarkRect(int x,
                int y,
                int width,
                int height);

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.

void AutoMap::getIslope(AutoMap::MLine *ml, AutoMap::ISlope *is) const noexcept
{
    int32_t dx, dy;
    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (!dy)
    {
        is->islp = (dx < 0 ? -MAXINT : MAXINT);
    }
    else
    {
        is->islp = FixedDiv(dx, dy);
    }
    if (!dx)
    {
        is->slp = (dy < 0 ? -MAXINT : MAXINT);
    }
    else
    {
        is->slp = FixedDiv(dy, dx);
    }
}

//
//
//
void AutoMap::activateNewScale() noexcept
{
    m_x += m_w / 2;
    m_y += m_h / 2;
    m_w = FTOM(f_w);
    m_h = FTOM(f_h);
    m_x -= m_w / 2;
    m_y -= m_h / 2;
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

//
//
//
void AutoMap::saveScaleAndLoc() noexcept
{
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;
}

//
//
//
void AutoMap::restoreScaleAndLoc() noexcept
{
    m_w = old_m_w;
    m_h = old_m_h;
    if (!followplayer)
    {
        m_x = old_m_x;
        m_y = old_m_y;
    }
    else
    {
        m_x = plr->mo->x - m_w / 2;
        m_y = plr->mo->y - m_h / 2;
    }
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;

    // Change the scaling multipliers
    scale_mtof = FixedDiv(f_w << FRACBITS, m_w);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// adds a marker at the current location
//
void AutoMap::addMark() noexcept
{
    markpoints[markpointnum].x = m_x + m_w / 2;
    markpoints[markpointnum].y = m_y + m_h / 2;
    markpointnum = (markpointnum + 1) % NUMMARKPOINTS;
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AutoMap::findMinMaxBoundaries() noexcept
{
    int i;
    fixed_t a;
    fixed_t b;

    min_x = min_y = MAXINT;
    max_x = max_y = -MAXINT;

    for (i = 0; i < numvertexes; i++)
    {
        if (vertexes[i].x < min_x)
            min_x = vertexes[i].x;
        else if (vertexes[i].x > max_x)
            max_x = vertexes[i].x;

        if (vertexes[i].y < min_y)
            min_y = vertexes[i].y;
        else if (vertexes[i].y > max_y)
            max_y = vertexes[i].y;
    }

    max_w = max_x - min_x;
    max_h = max_y - min_y;

    min_w = 2 * PLAYERRADIUS; // const? never changed?
    min_h = 2 * PLAYERRADIUS;

    a = FixedDiv(f_w << FRACBITS, max_w);
    b = FixedDiv(f_h << FRACBITS, max_h);

    min_scale_mtof = a < b ? a : b;
    max_scale_mtof = FixedDiv(f_h << FRACBITS, 2 * PLAYERRADIUS);
}

//
//
//
void AutoMap::changeWindowLoc() noexcept
{
    if (m_paninc.x || m_paninc.y)
    {
        followplayer = 0;
        f_oldloc.x = MAXINT;
    }

    m_x += m_paninc.x;
    m_y += m_paninc.y;

    if (m_x + m_w / 2 > max_x)
        m_x = max_x - m_w / 2;
    else if (m_x + m_w / 2 < min_x)
        m_x = min_x - m_w / 2;

    if (m_y + m_h / 2 > max_y)
        m_y = max_y - m_h / 2;
    else if (m_y + m_h / 2 < min_y)
        m_y = min_y - m_h / 2;

    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

//
//
//
void AutoMap::initVariables() noexcept
{
    int pnum;
    static Event st_notify = {EventType::KeyUp, MSGENTERED};

    automapactive = true;
    fb = screens[0];

    f_oldloc.x = MAXINT;
    amclock = 0;
    lightlev = 0;

    m_paninc.x = m_paninc.y = 0;
    ftom_zoommul = FRACUNIT;
    mtof_zoommul = FRACUNIT;

    m_w = FTOM(f_w);
    m_h = FTOM(f_h);

    // find player to center on initially
    if (!playeringame[pnum = consoleplayer])
        for (pnum = 0; pnum < MAXPLAYERS; pnum++)
            if (playeringame[pnum])
                break;

    plr = &players[pnum];
    m_x = plr->mo->x - m_w / 2;
    m_y = plr->mo->y - m_h / 2;
    changeWindowLoc();

    // for saving & restoring
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;

    // inform the status bar of the change
    ST_Responder(&st_notify);
}

//
//
//
void AutoMap::loadPics() noexcept
{
    int i;
    char namebuf[9];

    for (i = 0; i < 10; i++)
    {
        sprintf(namebuf, "AMMNUM%d", i);
        marknums[i] = reinterpret_cast<patch_t *>(W_CacheLumpName(namebuf, PU_STATIC));
    }
}

void AutoMap::unloadPics()
{
    for (int i = 0; i < 10; i++)
    {
        Z_ChangeTag(marknums[i], PU_CACHE);
    }
}

void AutoMap::clearMarks() noexcept
{
    for (int i = 0; i < NUMMARKPOINTS; i++)
    {
        markpoints[i].x = -1; // means empty
    }
    markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AutoMap::levelInit()
{
    leveljuststarted = 0;
    f_x = f_y = 0;
    f_w = finit_width;
    f_h = finit_height;

    clearMarks();

    findMinMaxBoundaries();
    scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7 * FRACUNIT));
    if (scale_mtof > max_scale_mtof)
        scale_mtof = min_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
//
//
void AutoMap::Stop()
{
    st_notify = {static_cast<EventType>(0), INT(EventType::KeyUp), MSGEXITED};
    unloadPics();
    automapactive = false;
    ST_Responder(&st_notify);
    stopped = true;
}

//
//
//
void AutoMap::Start()
{
    static int lastlevel = -1, lastepisode = -1;

    if (!stopped)
    {
        Stop();
    }
    stopped = false;
    if (lastlevel != gamemap || lastepisode != gameepisode)
    {
        levelInit();
        lastlevel = gamemap;
        lastepisode = gameepisode;
    }
    initVariables();
    loadPics();
}

//
// set the window scale to the maximum size
//
void AutoMap::minOutWindowScale()
{
    scale_mtof = min_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    activateNewScale();
}

//
// set the window scale to the minimum size
//
void AutoMap::maxOutWindowScale()
{
    scale_mtof = max_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    activateNewScale();
}

//
// Handle events (user inputs) in automap mode
//
bool AutoMap::Responder(Event *ev)
{
    int rc;
    static int cheatstate = 0;
    static int bigstate = 0;
    static char buffer[20];

    rc = false;

    if (!automapactive)
    {
        if (ev->type == EventType::KeyDown && ev->data1 == STARTKEY)
        {
            Start();
            viewactive = false;
            rc = true;
        }
    }

    else if (ev->type == EventType::KeyDown)
    {

        rc = true;
        switch (ev->data1)
        {
        case PANRIGHTKEY: // pan right
            if (!followplayer)
                m_paninc.x = FTOM(F_PANINC);
            else
                rc = false;
            break;
        case PANLEFTKEY: // pan left
            if (!followplayer)
                m_paninc.x = -FTOM(F_PANINC);
            else
                rc = false;
            break;
        case PANUPKEY: // pan up
            if (!followplayer)
                m_paninc.y = FTOM(F_PANINC);
            else
                rc = false;
            break;
        case PANDOWNKEY: // pan down
            if (!followplayer)
                m_paninc.y = -FTOM(F_PANINC);
            else
                rc = false;
            break;
        case ZOOMOUTKEY: // zoom out
            mtof_zoommul = M_ZOOMOUT;
            ftom_zoommul = M_ZOOMIN;
            break;
        case ZOOMINKEY: // zoom in
            mtof_zoommul = M_ZOOMIN;
            ftom_zoommul = M_ZOOMOUT;
            break;
        case ENDKEY:
            bigstate = 0;
            viewactive = true;
            Stop();
            break;
        case GOBIGKEY:
            bigstate = !bigstate;
            if (bigstate)
            {
                saveScaleAndLoc();
                minOutWindowScale();
            }
            else
                restoreScaleAndLoc();
            break;
        case FOLLOWKEY:
            followplayer = !followplayer;
            f_oldloc.x = MAXINT;
            plr->message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;
            break;
        case GRIDKEY:
            grid = !grid;
            plr->message = grid ? AMSTR_GRIDON : AMSTR_GRIDOFF;
            break;
        case MARKKEY:
            sprintf(buffer, "%s %d", AMSTR_MARKEDSPOT, markpointnum);
            plr->message = buffer;
            addMark();
            break;
        case CLEARMARKKEY:
            clearMarks();
            plr->message = AMSTR_MARKSCLEARED;
            break;
        default:
            cheatstate = 0;
            rc = false;
        }
        if (!deathmatch && cht_CheckCheat(&cheat_amap, ev->data1))
        {
            rc = false;
            cheating = (cheating + 1) % 3;
        }
    }

    else if (ev->type == EventType::KeyUp)
    {
        rc = false;
        switch (ev->data1)
        {
        case PANRIGHTKEY:
            if (!followplayer)
                m_paninc.x = 0;
            break;
        case PANLEFTKEY:
            if (!followplayer)
                m_paninc.x = 0;
            break;
        case PANUPKEY:
            if (!followplayer)
                m_paninc.y = 0;
            break;
        case PANDOWNKEY:
            if (!followplayer)
                m_paninc.y = 0;
            break;
        case ZOOMOUTKEY:
        case ZOOMINKEY:
            mtof_zoommul = FRACUNIT;
            ftom_zoommul = FRACUNIT;
            break;
        }
    }

    return rc;
}

//
// Zooming
//
void AutoMap::changeWindowScale()
{

    // Change the scaling multipliers
    scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

    if (scale_mtof < min_scale_mtof)
        minOutWindowScale();
    else if (scale_mtof > max_scale_mtof)
        maxOutWindowScale();
    else
        activateNewScale();
}

//
//
//
void AutoMap::doFollowPlayer()
{

    if (f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
    {
        m_x = FTOM(MTOF(plr->mo->x)) - m_w / 2;
        m_y = FTOM(MTOF(plr->mo->y)) - m_h / 2;
        m_x2 = m_x + m_w;
        m_y2 = m_y + m_h;
        f_oldloc.x = plr->mo->x;
        f_oldloc.y = plr->mo->y;

        //  m_x = FTOM(MTOF(plr->mo->x - m_w/2));
        //  m_y = FTOM(MTOF(plr->mo->y - m_h/2));
        //  m_x = plr->mo->x - m_w/2;
        //  m_y = plr->mo->y - m_h/2;
    }
}

//
//
//
void AutoMap::updateLightLev()
{
    static int nexttic = 0;
    //static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
    static int litelevels[] = {0, 4, 7, 10, 12, 14, 15, 15};
    static int litelevelscnt = 0;

    // Change light level
    if (amclock > nexttic)
    {
        lightlev = litelevels[litelevelscnt++];
        if (litelevelscnt == sizeof(litelevels) / sizeof(int))
            litelevelscnt = 0;
        nexttic = amclock + 6 - (amclock % 6);
    }
}

//
// Updates on Game Tick
//
void AutoMap::Ticker()
{

    if (!automapactive)
        return;

    amclock++;

    if (followplayer)
        doFollowPlayer();

    // Change the zoom if necessary
    if (ftom_zoommul != FRACUNIT)
        changeWindowScale();

    // Change x,y location
    if (m_paninc.x || m_paninc.y)
        changeWindowLoc();

    // Update light level
    // updateLightLev();
}

//
// Clear automap frame buffer.
//
void AutoMap::clearFB(int color)
{
    memset(fb, color, f_w * f_h);
}

//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
bool AutoMap::clipMline(MLine *ml, FLine *fl)
{
    enum
    {
        LEFT = 1,
        RIGHT = 2,
        BOTTOM = 4,
        TOP = 8
    };

    register int outcode1 = 0;
    register int outcode2 = 0;
    register int outside;

    FPoint tmp;
    int dx;
    int dy;

#define DOOUTCODE(oc, mx, my) \
    (oc) = 0;                 \
    if ((my) < 0)             \
        (oc) |= TOP;          \
    else if ((my) >= f_h)     \
        (oc) |= BOTTOM;       \
    if ((mx) < 0)             \
        (oc) |= LEFT;         \
    else if ((mx) >= f_w)     \
        (oc) |= RIGHT;

    // do trivial rejects and outcodes
    if (ml->a.y > m_y2)
        outcode1 = TOP;
    else if (ml->a.y < m_y)
        outcode1 = BOTTOM;

    if (ml->b.y > m_y2)
        outcode2 = TOP;
    else if (ml->b.y < m_y)
        outcode2 = BOTTOM;

    if (outcode1 & outcode2)
        return false; // trivially outside

    if (ml->a.x < m_x)
        outcode1 |= LEFT;
    else if (ml->a.x > m_x2)
        outcode1 |= RIGHT;

    if (ml->b.x < m_x)
        outcode2 |= LEFT;
    else if (ml->b.x > m_x2)
        outcode2 |= RIGHT;

    if (outcode1 & outcode2)
        return false; // trivially outside

    // transform to frame-buffer coordinates.
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
        return false;

    while (outcode1 | outcode2)
    {
        // may be partially inside box
        // find an outside point
        if (outcode1)
            outside = outcode1;
        else
            outside = outcode2;

        // clip to each side
        if (outside & TOP)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
            tmp.y = 0;
        }
        else if (outside & BOTTOM)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y - f_h)) / dy;
            tmp.y = f_h - 1;
        }
        else if (outside & RIGHT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (f_w - 1 - fl->a.x)) / dx;
            tmp.x = f_w - 1;
        }
        else if (outside & LEFT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
            tmp.x = 0;
        }

        if (outside == outcode1)
        {
            fl->a = tmp;
            DOOUTCODE(outcode1, fl->a.x, fl->a.y);
        }
        else
        {
            fl->b = tmp;
            DOOUTCODE(outcode2, fl->b.x, fl->b.y);
        }

        if (outcode1 & outcode2)
            return false; // trivially outside
    }

    return true;
}
#undef DOOUTCODE

//
// Classic Bresenham w/ whatever optimizations needed for speed
//
void AutoMap::drawFline(FLine *fl, int color)
{
    register int x;
    register int y;
    register int dx;
    register int dy;
    register int sx;
    register int sy;
    register int ax;
    register int ay;
    register int d;

    static int fuck = 0;

    // For debugging only
    if (fl->a.x < 0 || fl->a.x >= f_w || fl->a.y < 0 || fl->a.y >= f_h || fl->b.x < 0 || fl->b.x >= f_w || fl->b.y < 0 || fl->b.y >= f_h)
    {
        fprintf(stderr, "fuck %d \r", fuck++);
        return;
    }

#define PUTDOT(xx, yy, cc) fb[(yy)*f_w + (xx)] = (cc)

    dx = fl->b.x - fl->a.x;
    ax = 2 * (dx < 0 ? -dx : dx);
    sx = dx < 0 ? -1 : 1;

    dy = fl->b.y - fl->a.y;
    ay = 2 * (dy < 0 ? -dy : dy);
    sy = dy < 0 ? -1 : 1;

    x = fl->a.x;
    y = fl->a.y;

    if (ax > ay)
    {
        d = ay - ax / 2;
        while (1)
        {
            PUTDOT(x, y, color);
            if (x == fl->b.x)
                return;
            if (d >= 0)
            {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    }
    else
    {
        d = ax - ay / 2;
        while (1)
        {
            PUTDOT(x, y, color);
            if (y == fl->b.y)
                return;
            if (d >= 0)
            {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
}

//
// Clip lines, draw visible part sof lines.
//
void AutoMap::drawMline(MLine *ml, int color)
{
    static FLine fl;

    if (clipMline(ml, &fl))
        drawFline(&fl, color); // draws it on frame buffer using fb coords
}

//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
void AutoMap::drawGrid(int color)
{
    fixed_t x, y;
    fixed_t start, end;
    MLine ml;

    // Figure out start of vertical gridlines
    start = m_x;
    if ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS))
        start += (MAPBLOCKUNITS << FRACBITS) - ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS));
    end = m_x + m_w;

    // draw vertical gridlines
    ml.a.y = m_y;
    ml.b.y = m_y + m_h;
    for (x = start; x < end; x += (MAPBLOCKUNITS << FRACBITS))
    {
        ml.a.x = x;
        ml.b.x = x;
        drawMline(&ml, color);
    }

    // Figure out start of horizontal gridlines
    start = m_y;
    if ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS))
        start += (MAPBLOCKUNITS << FRACBITS) - ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS));
    end = m_y + m_h;

    // draw horizontal gridlines
    ml.a.x = m_x;
    ml.b.x = m_x + m_w;
    for (y = start; y < end; y += (MAPBLOCKUNITS << FRACBITS))
    {
        ml.a.y = y;
        ml.b.y = y;
        drawMline(&ml, color);
    }
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
void AutoMap::drawWalls()
{
    int i;
    static MLine l;

    for (i = 0; i < numlines; i++)
    {
        l.a.x = lines[i].v1->x;
        l.a.y = lines[i].v1->y;
        l.b.x = lines[i].v2->x;
        l.b.y = lines[i].v2->y;
        if (cheating || (lines[i].flags & ML_MAPPED))
        {
            if ((lines[i].flags & LINE_NEVERSEE) && !cheating)
                continue;
            if (!lines[i].backsector)
            {
                drawMline(&l, WALLCOLORS + lightlev);
            }
            else
            {
                if (lines[i].special == 39)
                { // teleporters
                    drawMline(&l, WALLCOLORS + WALLRANGE / 2);
                }
                else if (lines[i].flags & ML_SECRET) // secret door
                {
                    if (cheating)
                        drawMline(&l, SECRETWALLCOLORS + lightlev);
                    else
                        drawMline(&l, WALLCOLORS + lightlev);
                }
                else if (lines[i].backsector->floorheight != lines[i].frontsector->floorheight)
                {
                    drawMline(&l, FDWALLCOLORS + lightlev); // floor level change
                }
                else if (lines[i].backsector->ceilingheight != lines[i].frontsector->ceilingheight)
                {
                    drawMline(&l, CDWALLCOLORS + lightlev); // ceiling level change
                }
                else if (cheating)
                {
                    drawMline(&l, TSWALLCOLORS + lightlev);
                }
            }
        }
        else if (plr->powers[INT(PowerType::pw_allmap)])
        {
            if (!(lines[i].flags & LINE_NEVERSEE))
                drawMline(&l, GRAYS + 3);
        }
    }
}

//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
void AutoMap::rotate(fixed_t *x,
               fixed_t *y,
               angle_t a)
{
    fixed_t tmpx;

    tmpx =
        FixedMul(*x, finecosine[a >> ANGLETOFINESHIFT]) - FixedMul(*y, finesine[a >> ANGLETOFINESHIFT]);

    *y =
        FixedMul(*x, finesine[a >> ANGLETOFINESHIFT]) + FixedMul(*y, finecosine[a >> ANGLETOFINESHIFT]);

    *x = tmpx;
}

void AutoMap::drawLineCharacter(MLine *lineguy,
                          int lineguylines,
                          fixed_t scale,
                          angle_t angle,
                          int color,
                          fixed_t x,
                          fixed_t y)
{
    int i;
    MLine l;

    for (i = 0; i < lineguylines; i++)
    {
        l.a.x = lineguy[i].a.x;
        l.a.y = lineguy[i].a.y;

        if (scale)
        {
            l.a.x = FixedMul(scale, l.a.x);
            l.a.y = FixedMul(scale, l.a.y);
        }

        if (angle)
            rotate(&l.a.x, &l.a.y, angle);

        l.a.x += x;
        l.a.y += y;

        l.b.x = lineguy[i].b.x;
        l.b.y = lineguy[i].b.y;

        if (scale)
        {
            l.b.x = FixedMul(scale, l.b.x);
            l.b.y = FixedMul(scale, l.b.y);
        }

        if (angle)
            rotate(&l.b.x, &l.b.y, angle);

        l.b.x += x;
        l.b.y += y;

        drawMline(&l, color);
    }
}

void AutoMap::drawPlayers()
{
    int i;
    player_t *p;
    static int their_colors[] = {GREENS, GRAYS, BROWNS, REDS};
    int their_color = -1;
    int color;

    if (!netgame)
    {
        if (cheating)
            drawLineCharacter(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
                                 plr->mo->angle, WHITE, plr->mo->x, plr->mo->y);
        else
            drawLineCharacter(player_arrow, NUMPLYRLINES, 0, plr->mo->angle,
                                 WHITE, plr->mo->x, plr->mo->y);
        return;
    }

    for (i = 0; i < MAXPLAYERS; i++)
    {
        their_color++;
        p = &players[i];

        if ((deathmatch && !singledemo) && p != plr)
            continue;

        if (!playeringame[i])
            continue;

        if (p->powers[INT(PowerType::pw_invisibility)])
            color = 246; // *close* to black
        else
            color = their_colors[their_color];

        drawLineCharacter(player_arrow, NUMPLYRLINES, 0, p->mo->angle,
                             color, p->mo->x, p->mo->y);
    }
}

void AutoMap::drawThings(int colors, int colorrange)
{
    int i;
    mobj_t *t;

    for (i = 0; i < numsectors; i++)
    {
        t = sectors[i].thinglist;
        while (t)
        {
            drawLineCharacter(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
                                 16 << FRACBITS, t->angle, colors + lightlev, t->x, t->y);
            t = t->snext;
        }
    }
}

void AutoMap::drawMarks()
{
    int i, fx, fy, w, h;

    for (i = 0; i < NUMMARKPOINTS; i++)
    {
        if (markpoints[i].x != -1)
        {
            //      w = SHORT(marknums[i]->width);
            //      h = SHORT(marknums[i]->height);
            w = 5; // because something's wrong with the wad, i guess
            h = 6; // because something's wrong with the wad, i guess
            fx = CXMTOF(markpoints[i].x);
            fy = CYMTOF(markpoints[i].y);
            if (fx >= f_x && fx <= f_w - w && fy >= f_y && fy <= f_h - h)
                V_DrawPatch(fx, fy, FB, marknums[i]);
        }
    }
}

void AutoMap::drawCrosshair(int color)
{
    fb[(f_w * (f_h + 1)) / 2] = color; // single point for now
}

void AutoMap::Drawer()
{
    if (!automapactive)
        return;

    clearFB(BACKGROUND);
    if (grid)
        drawGrid(GRIDCOLORS);
    drawWalls();
    drawPlayers();
    if (cheating == 2)
        drawThings(THINGCOLORS, THINGRANGE);
    drawCrosshair(XHAIRCOLORS);

    drawMarks();

    V_MarkRect(f_x, f_y, f_w, f_h);
}
