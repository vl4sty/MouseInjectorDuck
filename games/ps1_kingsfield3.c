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

#define KF3_CAMY 0x1B2610
#define KF3_CAMX 0x1B2612
#define KF3_IS_BUSY 0x18FAA8

static uint8_t PS1_KF3_Status(void);
static void PS1_KF3_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"King's Field II (III)",
	PS1_KF3_Status,
	PS1_KF3_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_KINGSFIELD3 = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_KF3_Status(void)
{
	return ((PS1_MEM_ReadWord(0x9304) == 0x534C5553U && PS1_MEM_ReadWord(0x9308) == 0x5F303032U && PS1_MEM_ReadWord(0x930C) == 0x2E35353BU));
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_KF3_Inject(void)
{
	// TODO: play a bit to see if IS_BUSY messes with camera

	// don't move camera if reading message, in conversation, paused, loading, etc...
	if (PS1_MEM_ReadHalfword(KF3_IS_BUSY))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(KF3_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(KF3_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	while (camYF > 4096)
		camYF -= 4096;

	// clamp y-axis
	if (camYF > 700 && camYF < 2000)
		camYF = 700;
	if (camYF < 3396 && camYF > 2000)
		camYF = 3396;

	PS1_MEM_WriteHalfword(KF3_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(KF3_CAMY, (uint16_t)camYF);
}