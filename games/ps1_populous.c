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

#define PTB_CURSORX 0x164726
#define PTB_CURSORY 0x164728

static uint8_t PS1_PTB_Status(void);
static void PS1_PTB_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Populous: The Beginning",
	PS1_PTB_Status,
	PS1_PTB_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_POPULOUSTHEBEGINNING = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_PTB_Status(void)
{
	// SLUS_002.77
	return (PS1_MEM_ReadWord(0x9334) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9338) == 0x5F303032U && 
			PS1_MEM_ReadWord(0x933C) == 0x2E37373BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_PTB_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	uint16_t cursorX = PS1_MEM_ReadHalfword(PTB_CURSORX);
	uint16_t cursorY = PS1_MEM_ReadHalfword(PTB_CURSORY);
	float cursorXF = (float)cursorX;
	float cursorYF = (float)cursorY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 0.6f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&cursorXF, &xAccumulator, xmouse, dx);
	cursorXF = ClampFloat(cursorXF, 8.f, 312.f);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&cursorYF, &yAccumulator, ym, dy);
	cursorYF = ClampFloat(cursorYF, 16.f, 224.f);

	PS1_MEM_WriteHalfword(PTB_CURSORX, (uint16_t)cursorXF);
	PS1_MEM_WriteHalfword(PTB_CURSORY, (uint16_t)cursorYF);
}