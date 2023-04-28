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

#define NOLF_CAMY 0x7C3E50
#define NOLF_CAMX 0x7C3E54
#define NOLF_ZOOM 0x426100

#define NOLF_IS_IN_GAME_CUTSCENE 0x424064

static uint8_t PS2_NOLF_Status(void);
static void PS2_NOLF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"No One Lives Forever",
	PS2_NOLF_Status,
	PS2_NOLF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_NOONELIVESFOREVER = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_NOLF_Status(void)
{
	// SLUS_200.28
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323030U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E32383BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_NOLF_Inject(void)
{
	if (PS2_MEM_ReadUInt(NOLF_IS_IN_GAME_CUTSCENE))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 14000.f;
	float zoom = 1.f / PS2_MEM_ReadFloat(NOLF_ZOOM);

	float camX = PS2_MEM_ReadFloat(NOLF_CAMX);
	float camY = PS2_MEM_ReadFloat(NOLF_CAMY);

	camX += (float)xmouse * looksensitivity * zoom;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * zoom;

	camY = ClampFloat(camY, -1.470796347, 1.470796347);

	PS2_MEM_WriteFloat(NOLF_CAMX, (float)camX);
	PS2_MEM_WriteFloat(NOLF_CAMY, (float)camY);

}