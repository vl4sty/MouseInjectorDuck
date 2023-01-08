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

#define TAU 6.2831853f // 0x40C90FDB

// STATIC addresses
#define SRUN_cursorx 0x0001F0 // range: 768-61184
#define SRUN_cursory 0x0001F2 // range: 768-52736
#define SRUN_aimmode 0x0001F4 // 0x0=off, 0xFFFF=on

#define SRUN_aimmode_on 0xFFFF

static uint8_t SNES_SRUN_Status(void);
static void SNES_SRUN_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Shadowrun",
	SNES_SRUN_Status,
	SNES_SRUN_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_SHADOWRUN = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_SRUN_Status(void)
{
	return (SNES_MEM_ReadWord(0x1CC5) == 0xF25F && SNES_MEM_ReadWord(0x1CCB) == 0xC10A);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_SRUN_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	SNES_MEM_WriteWord(0x1FA, 0xFFFF);

	if (SNES_MEM_ReadWord(SRUN_aimmode) != SRUN_aimmode_on)
		return;

	const float looksensitivity = (float)sensitivity;

	uint16_t cursorx = SNES_MEM_ReadWord(SRUN_cursorx);
	uint16_t cursory = SNES_MEM_ReadWord(SRUN_cursory);
	uint16_t lastX = cursorx;
	uint16_t lastY = cursory;

	cursorx += ((float)xmouse) * looksensitivity * 5.f;
	cursory += ((float)ymouse) * looksensitivity * 5.f;

	// prevent wrapping
	// if (lastX > 0 && lastX < 100 && cursorx > 200)
	// 	cursorx = 0.f;
	// if (lastY > 0 && lastY < 80 && cursory > 140)
	// 	cursory = 0.f;

	cursorx = ClampFloat(cursorx, 17.f, 61184.f);
	cursory = ClampFloat(cursory, 17.f, 52736.f);

	SNES_MEM_WriteWord(SRUN_cursorx, (uint16_t)cursorx);
	SNES_MEM_WriteWord(SRUN_cursory, (uint16_t)cursory);
}