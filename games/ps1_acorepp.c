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

#define ACPP_CAMY 0x42708
#define ACPP_CAMX 0x1E2DF2
#define ACPP_IS_NOT_BUSY 0x1A7FAC
#define ACPP_IS_NOT_PAUSED 0x3BA14
#define ACPP_IS_MAP_OPEN 0x1555EB
#define ACPP_IS_ABORT_PROMPT 0x1FBF5A


static uint8_t PS1_ACPP_Status(void);
static void PS1_ACPP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Armored Core: Project Phantasma",
	PS1_ACPP_Status,
	PS1_ACPP_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_ARMOREDCOREPP = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_ACPP_Status(void)
{
	// SLUS_006.70
	return (PS1_MEM_ReadWord(0x9274) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9278) == 0x5F303036U && 
			PS1_MEM_ReadWord(0x927C) == 0x2E37303BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_ACPP_Inject(void)
{
	if (!PS1_MEM_ReadByte(ACPP_IS_NOT_BUSY))
		return;
	
	if (!PS1_MEM_ReadByte(ACPP_IS_NOT_PAUSED))
		return;
	
	if (PS1_MEM_ReadByte(ACPP_IS_MAP_OPEN))
		return;

	if (PS1_MEM_ReadByte(ACPP_IS_ABORT_PROMPT) == 0x1A)
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(ACPP_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(ACPP_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	PS1_MEM_WriteHalfword(ACPP_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(ACPP_CAMY, (uint16_t)camYF);
}