#pragma once

#include "event.h"
#include "doomdef.h"
#include "w_wad.h"
#include "g_game.h"
#include "m_menu.h"
//
// EventManager singletone object
//

class EventManager
{
public:
    static EventManager &instance()
    {
        static EventManager *instance = new EventManager();
        return *instance;
    }

    //
    // PostEvent
    // Called by the I/O functions when input is detected
    //
    void PostEvent(Event *ev)
    {
        events[eventhead] = *ev;
        eventhead = (++eventhead) & (MAXEVENTS - 1);
    }

    //
    // ProcessEvents
    // Send all the events of the given timestamp down the responder chain
    //
    void ProcessEvents()
    {
        Event *ev;

        // IF STORE DEMO, DO NOT ACCEPT INPUT
        if ((gamemode == GameMode::commercial) && (W_CheckNumForName("map01") < 0))
        {
            return;
        }
        for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1))
        {
            ev = &events[eventtail];
            if (M_Responder(ev))
                continue; // menu ate the event
            G_Responder(ev);
        }
    }

    //
    // CheckAbort
    //
    void CheckAbort()
    {
        const int stoptic = I_GetTime() + 2;
        while (I_GetTime() < stoptic)
        {
            I_StartTic();
        }
        I_StartTic();

        for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1))
        {
            const Event* ev = &events[eventtail];
            if (ev->type == EventType::KeyDown && ev->data1 == KEY_ESCAPE)
                I_Error("Network game synchronization aborted.");
        }
    }

private:
    static constexpr const int MAXEVENTS = 64;

    EventManager() {}
    //
    // EVENT HANDLING
    //
    // Events are asynchronous inputs generally generated by the game user.
    // Events can be discarded if no responder claims them
    //
    Event events[MAXEVENTS];
    int eventhead;
    int eventtail;
};
