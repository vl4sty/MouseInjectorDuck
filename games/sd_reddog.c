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

#define RD_CURSORX 0x28D358
#define RD_CURSORY 0x28D35C

static uint8_t SD_RD_Status(void);
static void SD_RD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Red Dog: Superior Firepower",
	SD_RD_Status,
	SD_RD_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SD_REDDOG = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SD_RD_Status(void)
{
	return (SD_MEM_ReadWord(0x8040) == 0x54343032U && SD_MEM_ReadWord(0x8044) == 0x31354E20U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SD_RD_Inject(void)
{
	// TODO: rotate camera when cursor is at edge

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 0.01f;

	float cursorX = SD_MEM_ReadFloat(RD_CURSORX);
	float cursorY = SD_MEM_ReadFloat(RD_CURSORY);

	cursorX += (float)xmouse * looksensitivity * scale;
	cursorY -= (float)ymouse * looksensitivity * scale;

	SD_MEM_WriteFloat(RD_CURSORX, (float)cursorX);
	SD_MEM_WriteFloat(RD_CURSORY, (float)cursorY);

}