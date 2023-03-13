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

#define AT_CAMY 0x99FDC
#define AT_CAMX 0x99FDE

#define AT_SLOPE_UP_ADJUST 0x39D78
#define AT_SLOPE_DOWN_ADJUST 0x39E10

#define AT_IS_NOT_BUSY 0xA21BD

static uint8_t PS1_AT_Status(void);
static void PS1_AT_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Alien Trilogy",
	PS1_AT_Status,
	PS1_AT_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_ALIENTRILOGY = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_AT_Status(void)
{
	return (PS1_MEM_ReadWord(0x9334) == 0x534C5553U && PS1_MEM_ReadWord(0x9338) == 0x5F303030U && PS1_MEM_ReadWord(0x933C) == 0x2E30373BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_AT_Inject(void)
{
	if (!PS1_MEM_ReadByte(AT_IS_NOT_BUSY))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(AT_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(AT_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	while (camXF > 4096)
		camXF -= 4096;
	while (camXF < 0)
		camXF += 4096;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// game clamps camY to 48 (default straight ahead value) and it will stick there with small mouse movements
	// but it works with a larger value for the same angle so just use the wrapped value
	while (camYF < 7000)
		camYF += 4096;

	// clamp y-axis
	if (camYF < 7400)
		camYF = 7400;
	if (camYF > 8900)
		camYF = 8900;

	PS1_MEM_WriteHalfword(AT_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(AT_CAMY, (uint16_t)camYF);
}