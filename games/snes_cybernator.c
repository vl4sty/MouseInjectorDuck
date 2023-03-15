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

// STATIC addresses
#define CYB_GUN_ANGLE 0x0014F0
#define CYB_AIM_LOCK 0x14F7

static uint8_t SNES_CYB_Status(void);
static void SNES_CYB_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Cybernator",
	SNES_CYB_Status,
	SNES_CYB_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_CYBERNATOR = &GAMEDRIVER_INTERFACE;

static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_CYB_Status(void)
{
	return (SNES_MEM_ReadWord(0xE0) == 0x614A && SNES_MEM_ReadWord(0xE2) == 0x656B);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_CYB_Inject(void)
{
	// TODO: Don't adjust when Punch weapon selected
	// TODO: Make patch file
	// TODO: Add cursor
	// TODO: Lock aim so you can walk backwards and control direction with mouse

	if(ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 5.f;

	uint16_t gunAngle = SNES_MEM_ReadWord(CYB_GUN_ANGLE);
	float gunAngleF = (float)gunAngle;

	float dy = (float)ymouse * looksensitivity * scale;
	AccumulateAddRemainder(&gunAngleF, &yAccumulator, ymouse, dy);

	if (gunAngleF < 200)
		gunAngleF = 200;
	if (gunAngleF > 4096)
		gunAngleF = 4096;

	SNES_MEM_WriteWord(CYB_GUN_ANGLE, (uint16_t)gunAngleF);
}