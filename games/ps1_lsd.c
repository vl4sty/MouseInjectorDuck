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

#define LSD_CAMX 0x91EC2
#define LSD_CAMY 0x9B5E0
#define LSD_CAMY_SIGN 0x9B5E2

static uint8_t PS1_LSD_Status(void);
static void PS1_LSD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"LSD Dream Emulator",
	PS1_LSD_Status,
	PS1_LSD_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_LSDDREAMEMULATOR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float scale = 20.f;
static uint16_t sign = 0xFFFF;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_LSD_Status(void)
{
	return (PS1_MEM_ReadWord(0x9244) == 0x534C5053U && PS1_MEM_ReadWord(0x9248) == 0x5F303135U && PS1_MEM_ReadWord(0x924C) == 0x2E35363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_LSD_Inject(void)
{
	// TODO: disable on stairs
	// TODO: clampY
	// TODO: camBase

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(LSD_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(LSD_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity;

	float dx = (float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	// float ym = (float)(invertpitch ? -ymouse : ymouse);
	// float dy = ym * looksensitivity / scale;
	float dy = ((float)ymouse * looksensitivity / scale) * 8.f;
	AccumulateAddRemainder(&camYF, &yAccumulator, ymouse, dy);
	// AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	sign = 0xFFFF;
	if (camYF < 32000)
		sign = 0x0;
	PS1_MEM_WriteHalfword(LSD_CAMY_SIGN, sign);

	PS1_MEM_WriteHalfword(LSD_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(LSD_CAMY, (uint16_t)camYF);
}