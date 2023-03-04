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

#define AC_CAMY 0x411C0
#define AC_CAMX 0x1A26CA
#define AC_IS_NOT_BUSY 0x1AC80C
#define AC_IS_NOT_PAUSED 0x39AD4
#define AC_IS_MAP_OPEN 0x14C82B
// #define AC_IS_ABORT_PROMPT 0x1ED848
#define AC_IS_ABORT_PROMPT 0x1FE06E


static uint8_t PS1_AC_Status(void);
static void PS1_AC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Armored Core",
	PS1_AC_Status,
	PS1_AC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_ARMOREDCORE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_AC_Status(void)
{
	return ((PS1_MEM_ReadWord(0x928C) == 0x534C5553U && PS1_MEM_ReadWord(0x9290) == 0x5F303133U && PS1_MEM_ReadWord(0x9294) == 0x2E32333BU) || // SLUS_013.23 
			(PS1_MEM_ReadWord(0x928C) == 0x53435553U && PS1_MEM_ReadWord(0x9290) == 0x5F393431U && PS1_MEM_ReadWord(0x9294) == 0x2E38323BU)); // SCUS_941.82
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_AC_Inject(void)
{
	// TODO: set idle rotating animation with xmouse input

	if (!PS1_MEM_ReadHalfword(AC_IS_NOT_BUSY))
		return;
	
	if (!PS1_MEM_ReadHalfword(AC_IS_NOT_PAUSED))
		return;
	
	if (PS1_MEM_ReadByte(AC_IS_MAP_OPEN))
		return;

	// if (PS1_MEM_ReadByte(AC_IS_ABORT_PROMPT))
	if (PS1_MEM_ReadByte(AC_IS_ABORT_PROMPT) == 0x1A)
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(AC_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(AC_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	// range is larger than game usually allows but feels better
	// if (camYF > 600 && camYF < 32000)
	// 	camYF = 600;
	// if (camYF < 65000 && camYF > 32000)
	// 	camYF = 65000;

	PS1_MEM_WriteHalfword(AC_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(AC_CAMY, (uint16_t)camYF);
}