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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

// TODO: check if machinegun sentry cam and hold L2 manual aim use same values, they feel similar and left stick only does up and down just like machinegun
//	 try freezing fov above 400, the camera moves independent of gun

#define TAU 6.2831853f // 0x40C90FDB

// OFFSET addresses, requires a base address to use
#define SIN_cursorx 0x8010BA88 - 0x8010BA60
#define SIN_cursory 0x8010BA8C - 0x8010BA60

// STATIC addresses
#define SIN_playerbase 0x800DA978

static uint8_t N64_SIN_Status(void);
static uint8_t N64_SIN_DetectPlayer(void);
static void N64_SIN_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"N64 Tsumi to Batsu - Hoshi no Keishousha",
	N64_SIN_Status,
	N64_SIN_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_SINPUNISHMENT = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t N64_SIN_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A8004 && N64_MEM_ReadUInt(0x80000004) == 0x275AD940); // unique header in RDRAM
}
//==========================================================================
// Purpose: determines if there is a player
//==========================================================================
static uint8_t N64_SIN_DetectPlayer(void)
{
	const uint32_t tempplayerbase = N64_MEM_ReadUInt(SIN_playerbase);

	if (N64WITHINMEMRANGE(tempplayerbase) && N64_MEM_ReadUInt(tempplayerbase + 0x30) == 0xC2FE0000) // arbitrary offset value that is stable
	{
		playerbase = tempplayerbase;
		return 1;
	}

	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void N64_SIN_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (!N64_SIN_DetectPlayer())
		return;

	const float looksensitivity = (float)sensitivity / 100.f;

	float cursorx = N64_MEM_ReadFloat(playerbase + SIN_cursorx); // bounds: 20-420
	float cursory = N64_MEM_ReadFloat(playerbase + SIN_cursory); // 20-310

	cursorx += (float)xmouse * looksensitivity;
	cursory -= (float)ymouse * looksensitivity;

	cursorx = ClampFloat(cursorx, -144.f, 144.f);
	cursory = ClampFloat(cursory, -104.f, 104.f);

	N64_MEM_WriteFloat(playerbase + SIN_cursorx, cursorx);
	N64_MEM_WriteFloat(playerbase + SIN_cursory, cursory);
}