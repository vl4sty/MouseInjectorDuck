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

#define REDA_cursorx 0x4B0E54
#define REDA_snapx 0x4B21A4
#define REDA_cursory 0x2D092C

static uint8_t PS2_REDA_Status(void);
static void PS2_REDA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PS2 Resident Evil Dead Aim",
	PS2_REDA_Status,
	PS2_REDA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_REDEADAIM = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_REDA_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323036U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E36393BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_REDA_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	float cursorX = PS2_MEM_ReadFloat(REDA_cursorx);
	float cursorY = PS2_MEM_ReadUInt(REDA_cursory);

	cursorX += (float)xmouse * looksensitivity / 600.f;
	// cursorX += (float)xmouse / 100.f;
	cursorY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

	PS2_MEM_WriteFloat(REDA_cursorx, (float)cursorX);
	PS2_MEM_WriteFloat(REDA_snapx, (float)cursorX);
	PS2_MEM_WriteUInt(REDA_cursory, (uint32_t)cursorY);
}