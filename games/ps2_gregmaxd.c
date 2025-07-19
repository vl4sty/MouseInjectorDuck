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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f // 0x40C90FDB

#define GMAXD_CAMY 0x17DCA60
#define GMAXD_CAMXSIN 0x17DCA58
#define GMAXD_CAMXCOS 0x17DCA5C

#define GMAXD_IS_DEATH_CAM 0x1B82800

static uint8_t PS2_GMAXD_Status(void);
static void PS2_GMAXD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Greg Hastings' Tournament Paintball Max'd",
	PS2_GMAXD_Status,
	PS2_GMAXD_Inject,
	1,
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GREGMAXD = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_GMAXD_Status(void)
{
	return (PS2_MEM_ReadWord(0x00410CA8) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00410CAC) == 0x5F323135U &&
			PS2_MEM_ReadWord(0x00410CB0) == 0x2E33393BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_GMAXD_Inject(void)
{
	// FIXME: delta time/FPS dependent sensitivity?

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS2_MEM_ReadUInt(GMAXD_IS_DEATH_CAM) == 0x6)
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	float scale = 400.f;

	float camY = PS2_MEM_ReadFloat(GMAXD_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	camY = ClampFloat(camY, -0.707106, 0.422618);


	float camXSin = PS2_MEM_ReadFloat(GMAXD_CAMXSIN);
	float camXCos = PS2_MEM_ReadFloat(GMAXD_CAMXCOS);

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2;

	angle += (float)xmouse * looksensitivity / scale;

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(GMAXD_CAMXSIN, camXSin);
	PS2_MEM_WriteFloat(GMAXD_CAMXCOS, camXCos);
	PS2_MEM_WriteFloat(GMAXD_CAMY, camY);

}