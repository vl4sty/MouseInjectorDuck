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

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB

#define AVP_MAPX_MAX 640U
#define AVP_MAPY_MAX 454U

#define AVP_CURSORX 0x15FEA40
#define AVP_CURSORY 0x15FEA44
#define AVP_MAPX 0x1605EEC
#define AVP_MAPY 0x1605EF0

#define AVP_CURSOR_MODE 0x16EBA8C
#define AVP_CURSOR_MAPX 0x16EC400
#define AVP_CURSOR_MAPY 0x16EC404

static uint8_t PS2_AVP_Status(void);
static void PS2_AVP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Alien vs. Predator: Extinction",
	PS2_AVP_Status,
	PS2_AVP_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_AVPEXTINCTION = &GAMEDRIVER_INTERFACE;

static float camSpeed = 0.5f;
static float scrollBuffer = 40.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_AVP_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323031U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E34373BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_AVP_Inject(void)
{
	// TODO: fix slow selection expansion on left side of screen
	// TODO: find cursor base

	float cursorX = PS2_MEM_ReadFloat(AVP_CURSORX);
	float cursorY = PS2_MEM_ReadFloat(AVP_CURSORY);

	float mapX = PS2_MEM_ReadFloat(AVP_MAPX);
	float mapY = PS2_MEM_ReadFloat(AVP_MAPY);
	if (cursorX < scrollBuffer)
		PS2_MEM_WriteFloat(AVP_MAPX, mapX - camSpeed);
	if (cursorX > AVP_MAPX_MAX - scrollBuffer)
		PS2_MEM_WriteFloat(AVP_MAPX, mapX + camSpeed);
	if (cursorY < scrollBuffer)
		PS2_MEM_WriteFloat(AVP_MAPY, mapY - camSpeed);
	if (cursorY > AVP_MAPY_MAX - scrollBuffer)
		PS2_MEM_WriteFloat(AVP_MAPY, mapY + camSpeed);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 1.f;

	cursorX += (float)xmouse * looksensitivity / scale;
	cursorY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

	if (cursorX > AVP_MAPX_MAX)
		cursorX = AVP_MAPX_MAX;
	if (cursorX < 0)
		cursorX = 0;
	
	if (cursorY > AVP_MAPY_MAX)
		cursorY = AVP_MAPY_MAX;
	if (cursorY < 8)
		cursorY = 8;
	
	if (cursorX > 455 && cursorY > 310)
	{
		// float mapCursorX = PS2_MEM_ReadFloat(AVP_CURSOR_MAPX);
		// float mapCursorY = PS2_MEM_ReadFloat(AVP_CURSOR_MAPY);

		PS2_MEM_WriteUInt(AVP_CURSOR_MODE, 0x01000101);
		PS2_MEM_WriteFloat(AVP_CURSOR_MAPX, cursorX - 455);
		PS2_MEM_WriteFloat(AVP_CURSOR_MAPY, cursorY - 310);
	}
	else
	{

		PS2_MEM_WriteUInt(AVP_CURSOR_MODE, 0x01000001);
	}

	PS2_MEM_WriteFloat(AVP_CURSORX, (float)cursorX);
	PS2_MEM_WriteFloat(AVP_CURSORY, (float)cursorY);
}