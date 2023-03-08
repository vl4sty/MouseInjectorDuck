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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

// STATIC addresses
#define WG_CURSOR_X 0x001609
#define WG_CURSOR_Y 0x001709
#define WG_CURSOR_SPEED 0x1C09
#define WG_CURSOR_SPRITE 0x1308
#define WG_PAUSED 0xFEF
#define WG_CONTINUE_SCREEN 0xFD2
#define WG_NOT_IN_MENU 0x1073E
#define WG_SCREEN_X 0x24

static uint8_t SNES_WG_Status(void);
static void SNES_WG_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Wild Guns",
	SNES_WG_Status,
	SNES_WG_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_WILDGUNS = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint16_t lastCursorX = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_WG_Status(void)
{
	return (SNES_MEM_ReadWord(0xFFFC) == 0x728F && SNES_MEM_ReadWord(0xFFFE) == 0xEA88);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_WG_Inject(void)
{
	// breaks gold mine sub-boss
	// if (!SNES_MEM_ReadByte(WG_NOT_IN_MENU))
	// 	return;

	// don't move cursor when paused
	if (SNES_MEM_ReadByte(WG_PAUSED) == 0x3)
		return;

	// sprite not visible during level start and end
	if (SNES_MEM_ReadWord(WG_CURSOR_SPRITE) == 0x0)
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	uint16_t cursorXInt = SNES_MEM_ReadWord(WG_CURSOR_X);
	uint16_t cursorYInt = SNES_MEM_ReadWord(WG_CURSOR_Y);
	float cursorX = (float)cursorXInt;
	float cursorY = (float)cursorYInt;

	float dx = (float)xmouse * looksensitivity;
	AccumulateAddRemainder(&cursorX, &xAccumulator, xmouse, dx);

	int ym = (invertpitch ? -ymouse : ymouse);
	float dy = (float)ym * looksensitivity;
	AccumulateAddRemainder(&cursorY, &yAccumulator, ym, dy);

	// uint16_t screenX = SNES_MEM_ReadWord(WG_SCREEN_X);
	// float screenXF = (float)screenX;
	// x range: screenX + 4 (left) to screenX + 251 (right)
	// cursorX = ClampFloat(cursorX, screenXF + 4.f, screenXF + 251.f);
	// cursorY = ClampFloat(cursorY, 8.f, 132.f);

	SNES_MEM_WriteWord(WG_CURSOR_X, (uint16_t)cursorX);
	SNES_MEM_WriteWord(WG_CURSOR_Y, (uint16_t)cursorY);
}