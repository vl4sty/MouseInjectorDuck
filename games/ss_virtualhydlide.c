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

#define VH_CAMX 0x58D28
#define VH_ROTX 0x599BC

static uint8_t SS_VH_Status(void);
static void SS_VH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Virtual Hydlide",
	SS_VH_Status,
	SS_VH_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_SS_VIRTUALHYDLIDE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SS_VH_Status(void)
{
	return (PS1_MEM_ReadWord(0xC20) == 0x2D543431U && 
			PS1_MEM_ReadWord(0xC24) == 0x30344831U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SS_VH_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	float scale = 10.f;
	// float scale = 1.f;
	
	uint8_t camX = PS1_MEM_ReadByte(VH_CAMX);
	float camXF = (float)camX;

	float dx = -(float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	while (camXF >= 256)
		camXF -= 256;
	while (camXF < 0)
		camXF += 256;

	PS1_MEM_WriteByte(VH_CAMX, (uint8_t)camXF);

	camXF -= 256;

	while (camXF >= 256)
		camXF -= 256;
	while (camXF < 0)
		camXF += 256;

	PS1_MEM_WriteByte(VH_ROTX, (uint8_t)camXF);
}