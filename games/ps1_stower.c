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

#define STOW_CAMY 0x1991A0
#define STOW_CAMX 0x1991A2
#define STOW_IS_PAUSED 0x18B6DA
#define STOW_IS_NOT_GRABBING_ITEM 0x1FFF68
#define STOW_IS_SCREEN_TRANSITION 0x199146
#define STOW_IS_DIALOGUE_OR_MESSAGE 0x17BEA6 // 1 byte

static uint8_t PS1_STOW_Status(void);
static void PS1_STOW_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Shadow Tower",
	PS1_STOW_Status,
	PS1_STOW_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_SHADOWTOWER = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_STOW_Status(void)
{
	return (PS1_MEM_ReadWord(0x92A4) == 0x534C5553U && PS1_MEM_ReadWord(0x92A8) == 0x5F303038U && PS1_MEM_ReadWord(0x92AC) == 0x2E36333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_STOW_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// pause and menu
	if (PS1_MEM_ReadHalfword(STOW_IS_PAUSED))
		return;

	// picking up item
	if (!PS1_MEM_ReadHalfword(STOW_IS_NOT_GRABBING_ITEM))
		return;

	// conversation or reading wall message
	if (PS1_MEM_ReadByte(STOW_IS_DIALOGUE_OR_MESSAGE))
		return;

	// area change
	if (PS1_MEM_ReadHalfword(STOW_IS_SCREEN_TRANSITION))
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(STOW_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(STOW_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity;
	const float scale = 30.f;

	float dx = -(float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity / scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	if (camYF > 700 && camYF < 2000)
		camYF = 700;
	if (camYF < 3396 && camYF > 2000)
		camYF = 3396;

	while (camYF > 4096)
		camYF -= 4096;

	PS1_MEM_WriteHalfword(STOW_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(STOW_CAMY, (uint16_t)camYF);
}