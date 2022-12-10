//===========================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f // 0x40C90FDB
// G ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
// 0x7E517020 - playerbase in 1-1
// 0x7E6054A0 - playerbase in level 2, science facility
#define G_camy 0x7E517050 - 0x7E517020
#define G_camy_up_trigger 0x7E51716C - 0x7E517020 // 0x00000000 - off | 0x00000002 - on
#define G_cstick_y 0x7E51707C - 0x7E517020
#define G_camx 0x7E5174F0 - 0x7E517020
#define G_auto_center_timer 0x7E5170AC - 0x7E517020
#define G_player_not_in_conversation 0x7E6067E8 - 0x7E6054A0
// STATIC ADDRESSES BELOW
#define G_playerbase 0x8045C868 // random stack address, commonly holds player pointer - requires sanity checks before using!
#define G_fovbase 0x8045C7A4

static uint8_t G_Status(void);
static uint8_t G_DetectPlayer(void);
static void G_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Geist",
	G_Status,
	G_Inject,
	1, // 1000 Hz tickrate
	1 // crosshair sway supported for driver
};

// const GAMEDRIVER *GAME_TRIGGERMAN = &GAMEDRIVER_INTERFACE;
const GAMEDRIVER *GAME_GEIST = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t G_Status(void)
{
	return (MEM_ReadUInt(0x80000000) == 0x47495445U && MEM_ReadUInt(0x80000004) == 0x30310000U); // check game header to see if it matches Geist (GITE01)
}
//==========================================================================
// Purpose: detects player pointer from stack address
// Changed Globals: fovbase, playerbase
//==========================================================================
static uint8_t G_DetectPlayer(void)
{
	const uint32_t tempplayerbase = MEM_ReadUInt(G_playerbase);
	// if (WITHINMEMRANGE(tempplayerbase))
	if (WITHINARAMRANGE(tempplayerbase))
	{
		playerbase = tempplayerbase;
		return 1;
	}
	return WITHINARAMRANGE(playerbase);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void G_Inject(void)
{
	// TODO: turn off aim assist
	// fix camx on elevators/platforms

	if(!G_DetectPlayer()) // if player pointer was not found
		return;

	ARAM_WriteFloat(playerbase + G_auto_center_timer, 0.0f); // disable auto y centering
	const uint32_t playerNotInConversation = ARAM_ReadUInt(playerbase + G_player_not_in_conversation);
	if(!playerNotInConversation) // don't move camera if player is talking to NPC
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float camx = ARAM_ReadFloat(playerbase + G_camx);
	camx = (camx / 90.0f) * (TAU / 4);

	float camy = ARAM_ReadFloat(playerbase + G_camy);
	camy = (camy / 90.0f) * (TAU / 4); // convert to -PI/2 to PI/2 scale

	const float fov = MEM_ReadFloat(G_fovbase) / 60.0f; // just an arbitrary division, 60 FOV is average for humans
	const float looksensitivity = (float)sensitivity / 40.f;

	camx += -(float)xmouse / 10.f * looksensitivity / (360.f / TAU) / (1.2f / fov); // normal calculation method for X
	while(camx >= TAU)
		camx -= TAU;
	camx = (camx / (TAU / 4)) * 90.0f;

	camy += (float)(invertpitch ? ymouse : -ymouse) / 10.f * looksensitivity / (360.f / TAU) / (1.2f / fov); // normal calculation method for Y
	camy = (camy / (TAU / 4)) * 90.0f; // convert back to -90 to 90 scale

	ARAM_WriteFloat(playerbase + G_camx, camx);
	ARAM_WriteFloat(playerbase + G_camy, camy);
}