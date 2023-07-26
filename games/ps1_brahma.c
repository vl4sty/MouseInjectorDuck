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

#define BF_CAMY 0xE0D90
#define BF_CAMX 0xE0D92

#define BF_IS_BUSY 0xE2970
#define BF_IS_NOT_FMV 0xC4BD4

static uint8_t PS1_BF_Status(void);
static void PS1_BF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Brahma Force: The Assault on Beltlogger 9",
	PS1_BF_Status,
	PS1_BF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_BRAHMAFORCE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_BF_Status(void)
{
	return (PS1_MEM_ReadWord(0x9304) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9308) == 0x5F303034U && 
			PS1_MEM_ReadWord(0x930C) == 0x2E34343BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_BF_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadUInt(BF_IS_BUSY))
		return;

	if (!PS1_MEM_ReadUInt(BF_IS_NOT_FMV))
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(BF_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(BF_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	// while (camXF > 4096)
	// 	camXF -= 4096;
	// while (camXF < 0)
	// 	camXF += 4096;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	// clamp y-axis
	if (camYF < 32000 && camYF > 662)
		camYF = 662;
	if (camYF > 32000 && camYF < 64874)
		camYF = 64874;

	PS1_MEM_WriteHalfword(BF_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(BF_CAMY, (uint16_t)camYF);
}