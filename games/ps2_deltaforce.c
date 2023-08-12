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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define DF_CAMY 0x1D917D0
#define DF_CAMX 0x1D917D4

static uint8_t PS2_DF_Status(void);
static void PS2_DF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Delta Force: Black Hawk Down",
	PS2_DF_Status,
	PS2_DF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_DELTAFORCE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.;
static float yAccumulator = 0.;
static float scale = 0.005f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_DF_Status(void)
{
	// SLUS_211.24
	return (PS2_MEM_ReadWord(0x003FA550) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x003FA554) == 0x5F323131U &&
			PS2_MEM_ReadWord(0x003FA558) == 0x2E323400U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_DF_Inject(void)
{
	// TODO: camX clamp when in helicopter gun

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	float camX = PS2_MEM_ReadFloat(DF_CAMX);
	float camY = PS2_MEM_ReadFloat(DF_CAMY);

	camX += (float)xmouse * looksensitivity * scale;
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * scale;
	camY = ClampFloat(camY, -1.299999952f, 1.299999952f);

	PS2_MEM_WriteFloat(DF_CAMX, camX);
	PS2_MEM_WriteFloat(DF_CAMY, camY);
}