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

#define BAR_CURRENT_FLOOR 0xBF698
#define BAR_CURRENT_FLOOR_MODE 0xBF699 // 0 is outer world, 1 is normal dungeon, 2 is Coffin Man tutorial
#define BAR_CAMY 0xC0670
#define BAR_CAMX 0xC0672
#define BAR_IS_BUSY 0xC6AAC
#define BAR_IS_PAUSED 0xB6584

#define BAR_FLOOR_MODE_TUTORIAL 0x2

static uint8_t PS1_BAR_Status(void);
static void PS1_BAR_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Baroque",
	PS1_BAR_Status,
	PS1_BAR_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_BAROQUE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_BAR_Status(void)
{
	return ((PS1_MEM_ReadWord(0x937C) == 0x534C504DU && PS1_MEM_ReadWord(0x9380) == 0x5F383633U && PS1_MEM_ReadWord(0x9384) == 0x2E32383BU) || // 1.02 English patch
			(PS1_MEM_ReadWord(0x9364) == 0x534C504DU && PS1_MEM_ReadWord(0x9368) == 0x5F383633U && PS1_MEM_ReadWord(0x936C) == 0x2E32383BU));  // original Japanese
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_BAR_Inject(void)
{
	if (PS1_MEM_ReadWord(BAR_IS_BUSY))
		return;
	
	if (PS1_MEM_ReadHalfword(BAR_IS_PAUSED))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(BAR_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(BAR_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	if (camYF > 400 && camYF < 32000)
		camYF = 400;
	if (camYF < 65000 && camYF > 32000)
		camYF = 65000;

	// prevent camY movement on final floor of tutorial as any value other than 0 when leaving will softlock
	uint8_t skipCamY = 0;
	if (PS1_MEM_ReadByte(BAR_CURRENT_FLOOR_MODE) == BAR_FLOOR_MODE_TUTORIAL && PS1_MEM_ReadByte(BAR_CURRENT_FLOOR) == 0x7)
		skipCamY = 1;
	
	PS1_MEM_WriteHalfword(BAR_CAMX, (uint16_t)camXF);
	if (!skipCamY)
		PS1_MEM_WriteHalfword(BAR_CAMY, (uint16_t)camYF);
}