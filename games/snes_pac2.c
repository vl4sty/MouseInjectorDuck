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
#define PAC_cursorx 0x001062 // range: 17-239
#define PAC_cursory 0x001066 // range: 17-182
#define PAC_canMoveCursor 0x000124
#define PAC_onMainMenu 0x0000F4

static uint8_t SNES_PAC_Status(void);
static void SNES_PAC_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Pac-Man 2: The New Adventures",
	SNES_PAC_Status,
	SNES_PAC_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_PACMAN2 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_PAC_Status(void)
{
	return (SNES_MEM_ReadWord(0xE3C) == 0x4150 && SNES_MEM_ReadWord(0xE3E) == 0x0043); // PAC.
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_PAC_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (SNES_MEM_ReadWord(PAC_canMoveCursor) != 0x4 && SNES_MEM_ReadWord(PAC_onMainMenu) != 0) // 4 is in-game, any other value is pause, screen transition, etc..
		return;

	const float looksensitivity = (float)sensitivity / 80.f;

	uint16_t cursorx = SNES_MEM_ReadWord(PAC_cursorx);
	uint16_t cursory = SNES_MEM_ReadWord(PAC_cursory);
	uint16_t lastX = cursorx;
	uint16_t lastY = cursory;

	cursorx += ((float)xmouse + 1) * looksensitivity * 1.6;
	cursory += ((float)ymouse + 1) * looksensitivity * 1.6;

	// prevent wrapping
	if (lastX > 0 && lastX < 100 && cursorx > 200)
		cursorx = 0.f;
	if (lastY > 0 && lastY < 80 && cursory > 140)
		cursory = 0.f;

	cursorx = ClampFloat(cursorx, 17.f, 239.f);
	cursory = ClampFloat(cursory, 17.f, 182.f);

	SNES_MEM_WriteWord(PAC_cursorx, (uint16_t)cursorx);
	SNES_MEM_WriteWord(PAC_cursory, (uint16_t)cursory);
}