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

#define G2_CAMY 0x19B9CA
#define G2_CAMX 0x19B9CC

#define G2_FACING 0x19B968

static uint8_t PS1_G2_Status(void);
static void PS1_G2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Gamera 2000",
	PS1_G2_Status,
	PS1_G2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_GAMERA2000 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_G2_Status(void)
{
	// SLPS_008.33
	return (PS1_MEM_ReadWord(0x92D4) == 0x534C5053U && 
			PS1_MEM_ReadWord(0x92D8) == 0x5F303038U && 
			PS1_MEM_ReadWord(0x92DC) == 0x2E33333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_G2_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// int16_t camX = PS1_MEM_ReadInt16(G2_CAMX);
	uint16_t camX = PS1_MEM_ReadHalfword(G2_CAMX);
	int16_t camY = PS1_MEM_ReadInt16(G2_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	// while (camXF > 4096)
	// 	camXF -= 4096;
	// while (camXF < 0)
	// 	camXF += 4096;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	uint16_t facing = PS1_MEM_ReadHalfword(G2_FACING);

	if (facing)
	{
		// camXF = ClampFloat(camXF, 460.f, 3600.f);
		camYF = ClampFloat(camYF, -540.f, 550.f);
	}
	else
	{
		// camXF = ClampFloat(camXF, -527.f, 530.f);
		camYF = ClampFloat(camYF, -406.f, 411.f);
	}

	while (camXF > 65535)
		camXF -= 65535;
	while (camXF < 0)
		camXF += 65535;

	// if (camXF > 460.f && camXF < 3600.f)
	// 	PS1_MEM_WriteHalfword(1, G2_FACING);
	// else
	// 	PS1_MEM_WriteHalfword(0, G2_FACING);

	// PS1_MEM_WriteInt16(G2_CAMX, (int16_t)camXF);
	PS1_MEM_WriteHalfword(G2_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteInt16(G2_CAMY, (int16_t)camYF);
}