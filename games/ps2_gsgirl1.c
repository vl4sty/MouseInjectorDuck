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
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define GSG_cursorx 0x303D80 		// range: 0 (left) - 512 (right)
#define GSG_last_cursorx 0x303D90
#define GSG_cursory 0x303D84		// range: 0 (top) - 448 (bottom)
#define GSG_last_cursory 0x303D94

static uint8_t PS2_GSG_Status(void);
static void PS2_GSG_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS2 Gunslinger Girl Vol. 1",
	PS2_GSG_Status,
	PS2_GSG_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GUNSLINGERGIRL1 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_GSG_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5053U && PS2_MEM_ReadWord(0x00093394) == 0x5F323533U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E34333BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_GSG_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	float cursorX = PS2_MEM_ReadFloat(GSG_cursorx);
	float cursorY = PS2_MEM_ReadFloat(GSG_cursory);

	cursorX += (float)xmouse * looksensitivity;
	cursorY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * 1.2;

	PS2_MEM_WriteFloat(GSG_cursorx, (float)cursorX);
	PS2_MEM_WriteFloat(GSG_last_cursorx, (float)cursorX);
	PS2_MEM_WriteFloat(GSG_cursory, (float)cursorY);
	PS2_MEM_WriteFloat(GSG_last_cursory, (float)cursorY);
}