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

#define LOTB_CAMX 0xE9D1C
#define LOTB_CAMY_1 0x10A70C
#define LOTB_CAMY_2 0x10A720

static uint8_t PS1_DNLOTB_Status(void);
static void PS1_DNLOTB_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Duke Nukem: Land of the Babes",
	PS1_DNLOTB_Status,
	PS1_DNLOTB_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_DNLANDOFTHEBABES = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_DNLOTB_Status(void)
{
	return (PS1_MEM_ReadWord(0x92EC) == 0x534C5553U && PS1_MEM_ReadWord(0x92F0) == 0x5F303130U && PS1_MEM_ReadWord(0x92F4) == 0x2E30323BU); // SLUS_010.02;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_DNLOTB_Inject(void)
{

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(LOTB_CAMX);
	// uint16_t camY = PS1_MEM_ReadHalfword(LOTB_CAMY_2);
	float camXF = (float)camX;
	// float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 30.f;

	float dx = (float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	// float ym = (float)(invertpitch ? -ymouse : ymouse);
	// float dy = ym * looksensitivity;
	// AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	PS1_MEM_WriteHalfword(LOTB_CAMX, (uint16_t)camXF);
	// PS1_MEM_WriteHalfword(LOTB_CAMY_1, (uint16_t)camYF);
	// PS1_MEM_WriteHalfword(LOTB_CAMY_2, (uint16_t)camYF);
}