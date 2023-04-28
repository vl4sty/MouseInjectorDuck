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

#define SOR_CAMY 0x4C80E0
#define SOR_CAMX 0x4C80E2

static uint8_t PS2_SOR_Status(void);
static void PS2_SOR_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Shadow of Rome",
	PS2_SOR_Status,
	PS2_SOR_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SHADOWOFROME = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.;
static float yAccumulator = 0.;
static float scale = 3.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SOR_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323039U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E30323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_SOR_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	uint16_t camX = PS2_MEM_ReadUInt16(SOR_CAMX);
	int16_t camY = PS2_MEM_ReadInt16(SOR_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);
	while (camXF >= 4096)
		camXF -= 4096.f;
	while (camXF < 0)
		camXF += 4096.f;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);
	camYF = ClampFloat(camYF, -700.f, 200.f);

	PS2_MEM_WriteUInt16(SOR_CAMX, (uint16_t)camXF);
	PS2_MEM_WriteInt16(SOR_CAMY, (int16_t)camYF);
}