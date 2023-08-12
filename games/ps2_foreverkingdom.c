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

#define FK_CAMY 0x245F50
#define FK_CAMX 0x245F54

static uint8_t PS2_FK_Status(void);
static void PS2_FK_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Forever Kingdom",
	PS2_FK_Status,
	PS2_FK_Inject,
	1,
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_FOREVERKINGDOM = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_FK_Status(void)
{
	// SLUS_203.43
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323033U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E34333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_FK_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 500.f;

	float camX = PS2_MEM_ReadFloat(FK_CAMX);
	float camY = PS2_MEM_ReadFloat(FK_CAMY);

	camX += (float)xmouse * looksensitivity / scale;
	while (camX > TAU / 2)
		camX -= TAU;
	while (camX < -TAU / 2)
		camX += TAU;

	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	camY = ClampFloat(camY, -1.3f, 1.3f);

	PS2_MEM_WriteFloat(FK_CAMX, camX);
	PS2_MEM_WriteFloat(FK_CAMY, camY);

}