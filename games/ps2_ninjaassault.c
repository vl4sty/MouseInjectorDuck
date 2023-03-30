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

#define NA_cursorx 0x4E6E20
#define NA_cursory 0x4E6E24
// #define NA_targetLock1 0xE06598
// #define NA_targetLock2 0xE0659C
// #define NA_targetLock3 0xE065A4
// #define NA_targetLock4 0xE065A8

static uint8_t PS2_NA_Status(void);
static void PS2_NA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS2 Ninja Assault",
	PS2_NA_Status,
	PS2_NA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_NINJAASSAULT = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.;
static float yAccumulator = 0.;
static float scale = 1.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_NA_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323034U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E39323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_NA_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;


	uint16_t cursorX = PS2_MEM_ReadUInt16(NA_cursorx);
	uint16_t cursorY = PS2_MEM_ReadUInt16(NA_cursory);
	float cursorXF = (float)cursorX;
	float cursorYF = (float)cursorY;

	float dx = (float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&cursorXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity / scale;
	AccumulateAddRemainder(&cursorYF, &yAccumulator, ym, dy);

	cursorXF = ClampFloat(cursorXF, 0, 639);
	cursorYF = ClampFloat(cursorYF, 0, 479);

	PS2_MEM_WriteUInt16(NA_cursorx, (uint16_t)cursorXF);
	PS2_MEM_WriteUInt16(NA_cursory, (uint16_t)cursorYF);
}