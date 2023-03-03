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

#define MX_CAMY 0x1E3634
// #define MX_CAMY 0x47634
// #define MX_CAMX 0x413F94

static uint8_t SD_MX_Status(void);
static void SD_MX_Inject(void);


static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Maken X",
	SD_MX_Status,
	SD_MX_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SD_MAKENX = &GAMEDRIVER_INTERFACE;


//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SD_MX_Status(void)
{
	return (SD_MEM_ReadWord(0x8040) == 0x4D4B2D35U && SD_MEM_ReadWord(0x8044) == 0x31303530U);
	// return (SD_MEM_ReadWord(0x0) == 0x28220389U && SD_MEM_ReadWord(0x4) == 0x60D3B365U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SD_MX_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;

	// float camX = SD_MEM_ReadFloat(MX_CAMX);
	float camY = SD_MEM_ReadFloat(MX_CAMY);

	// camX += (float)xmouse * looksensitivity;
	camY -= (float)ymouse * looksensitivity;

	int i;
	// bunch of redundant values that are all the same but need to all be set?
	for (i = 0; i < 30; ++i)
	{
		SD_MEM_WriteFloat(MX_CAMY + (i * 0x4), (float)camY);
	}
		// SD_MEM_WriteFloat(MX_CAMY + (i * 0x4), (float)camY);

}