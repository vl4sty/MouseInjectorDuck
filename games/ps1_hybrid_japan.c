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

#define HYB_CAMY 0x156E7C
#define HYB_CAMY2 0x9E7B8
#define HYB_CAMX 0xEE2EE

#define HYB_IS_NOT_PAUSED 0x9E770

static uint8_t PS1_HYB_Status(void);
static void PS1_HYB_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Hybrid",
	PS1_HYB_Status,
	PS1_HYB_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_HYBRID_JAPAN = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_HYB_Status(void)
{
	// SLPS_011.02
	return (PS1_MEM_ReadWord(0x93C4) == 0x534C5053U && 
			PS1_MEM_ReadWord(0x93C8) == 0x5F303131U && 
			PS1_MEM_ReadWord(0x93CC) == 0x2E30323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_HYB_Inject(void)
{
	// TODO: 25/50 FPS cheat

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_MEM_ReadByte(HYB_IS_NOT_PAUSED))
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(HYB_CAMX);
	int16_t camY = PS1_MEM_ReadInt16(HYB_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;

	float dx = (float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);
	while (camXF > 4096.f)
		camXF -= 4096.f;
	while (camXF < 0.f)
		camXF += 4096.f;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);
	camYF = ClampFloat(camYF, -340.f, 340.f);

	PS1_MEM_WriteHalfword(HYB_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteInt16(HYB_CAMY, (int16_t)camYF);
	// PS1_MEM_WriteInt16(HYB_CAMY2, (int16_t)camYF);
}