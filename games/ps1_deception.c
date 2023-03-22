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

#define TD_CAMX 0xA387A
#define TD_CAMY 0xA3880

static uint8_t PS1_TD_Status(void);
static void PS1_TD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Tecmo's Deception - Invitation to Darkness",
	PS1_TD_Status,
	PS1_TD_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_DECEPTION = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static float scale = 20.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_TD_Status(void)
{
	return (PS1_MEM_ReadWord(0x928C) == 0x534C5553U && PS1_MEM_ReadWord(0x9290) == 0x5F303033U && PS1_MEM_ReadWord(0x9294) == 0x2E34303BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_TD_Inject(void)
{
	// TODO: bird's-eye map cam
	// TODO: disable auto center
	// TODO: disable during
	//			pause
	//			conversation
	//			examine
	//			map
	// TODO: determine if crooked camera is caused by cheats?

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(TD_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(TD_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity;

	float dx = (float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float dy = -(float)ymouse * looksensitivity / scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ymouse, dy);

	while (camXF >= 4096)
		camXF -= 4096;

	if (camYF > 768 && camYF < 32000)
		camYF = 768;
	if (camYF < 65024 && camYF > 32000)
		camYF = 65024;

	PS1_MEM_WriteHalfword(TD_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(TD_CAMY, (uint16_t)camYF);
}