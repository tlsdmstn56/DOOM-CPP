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
//   Duh.
// 
//-----------------------------------------------------------------------------


#pragma once

#include "doomdef.h"
#include "event.h"



//
// GAME
//
void G_DeathMatchSpawnPlayer (int playernum);

void G_InitNew (Skill skill, int episode, int map);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (Skill skill, int episode, int map);

void G_DeferedPlayDemo (char* demo);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame (char* name);

void G_DoLoadGame ();

// Called by M_Responder.
void G_SaveGame (int slot, char* description);

// Only called by startup code.
void G_RecordDemo (char* name);

void G_BeginRecording ();

void G_PlayDemo (char* name);
void G_TimeDemo (char* name);
bool G_CheckDemoStatus ();

void G_ExitLevel ();
void G_SecretExitLevel ();

void G_WorldDone ();

void G_Ticker ();
bool G_Responder (Event*	ev);

void G_ScreenShot ();


//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
