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

#define VCS_CAMXSIN 0x6F4760
#define VCS_CAMXCOS 0x6F4764
#define VCS_CAMY 0x6F4768
#define VCS_CAMY2 0x6F4778

#define VCS_CAMXSIN_MID 0x6F46E0
#define VCS_CAMXCOS_MID 0x6F46E4
#define VCS_CAMY_MID 0x6F46E8

static uint8_t PS2_VCS_Status(void);
static void PS2_VCS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Grand Theft Auto: Vice City Stories",
	PS2_VCS_Status,
	PS2_VCS_Inject,
	1,
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GTAVICECITYSTORIES = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_VCS_Status(void)
{
	// SLUS_215.90
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323135U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E39303BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_VCS_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	float scale = 400.f;

	float camYMid = PS2_MEM_ReadFloat(VCS_CAMY_MID);
	float camXSinMid = PS2_MEM_ReadFloat(VCS_CAMXSIN_MID);
	float camXCosMid = PS2_MEM_ReadFloat(VCS_CAMXCOS_MID);

	float camY = (PS2_MEM_ReadFloat(VCS_CAMY) - camYMid) / 10.f;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	// camY = ClampFloat(camY, -0.707106, 0.422618);
	camY = (camY * 10.f) + camYMid;


	float camXSin = (PS2_MEM_ReadFloat(VCS_CAMXSIN) - camXSinMid) / 10.f;
	float camXCos = (PS2_MEM_ReadFloat(VCS_CAMXCOS) - camXCosMid) / 10.f;

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2;

	angle += (float)xmouse * looksensitivity / scale;

	camXSin = (sin(angle) * 10.f) + camXSinMid;
	camXCos = (cos(angle) * 10.f) + camXCosMid;

	PS2_MEM_WriteFloat(VCS_CAMXSIN, camXSin);
	PS2_MEM_WriteFloat(VCS_CAMXCOS, camXCos);
	PS2_MEM_WriteFloat(VCS_CAMY, camY);
	PS2_MEM_WriteFloat(VCS_CAMY2, camY);

}