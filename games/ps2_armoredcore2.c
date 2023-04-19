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

#define AC2_ROTY 0x2BB920
#define AC2_ROTX 0x2BB954

#define AC2_IS_PAUSED 0x2B6900
#define AC2_IS_PAUSED_TRUE 0xFF010000
#define AC2_IS_IN_GAME_CUTSCENE 0x2B68EC
#define AC2_IS_MAP_DISPLAYED 0x1C7D624
#define AC2_IS_NOT_IN_MENU 0x2D4D00

// #define AC2_EMBLEM_EDIT_CURSORX 0xC9670C
// #define AC2_EMBLEM_EDIT_CURSORY 0xC96710

// #define AC2_CURRENT_MENU_SCREEN 0x2AD32C
// #define AC2_MENU_SCREEN_EMBLEM_EDITOR 0x00000EDA

static uint8_t PS2_AC2_Status(void);
static void PS2_AC2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Armored Core 2",
	PS2_AC2_Status,
	PS2_AC2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_ARMOREDCORE2 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_AC2_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323030U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E31343BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_AC2_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 400.f;

	// if (PS2_MEM_ReadUInt(AC2_CURRENT_MENU_SCREEN) == AC2_MENU_SCREEN_EMBLEM_EDITOR)
	// {
	// 	// TODO: activate tools, pallete, editor when hovered
	// 	// TODO: cursor position base

	// 	// move emblem editor cursor
	// 	float cursorScale = 2.f;
	// 	float cursorX = PS2_MEM_ReadFloat(AC2_EMBLEM_EDIT_CURSORX);
	// 	float cursorY = PS2_MEM_ReadFloat(AC2_EMBLEM_EDIT_CURSORY);
	// 	cursorX += (float)xmouse * looksensitivity / cursorScale;
	// 	cursorY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / cursorScale;
	// 	PS2_MEM_WriteFloat(AC2_EMBLEM_EDIT_CURSORX, (float)cursorX);
	// 	PS2_MEM_WriteFloat(AC2_EMBLEM_EDIT_CURSORY, (float)cursorY);
	// }

	if (!PS2_MEM_ReadUInt(AC2_IS_NOT_IN_MENU))
		return;
	
	// paused
	if (PS2_MEM_ReadWord(AC2_IS_PAUSED) == AC2_IS_PAUSED_TRUE)
		return;
	
	if (PS2_MEM_ReadWord(AC2_IS_IN_GAME_CUTSCENE))
		return;

	if (PS2_MEM_ReadUInt(AC2_IS_MAP_DISPLAYED))
		return;

	float rotX = PS2_MEM_ReadFloat(AC2_ROTX);
	float rotY = PS2_MEM_ReadFloat(AC2_ROTY);

	rotX += (float)xmouse * looksensitivity / scale;
	rotY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;

	rotY = ClampFloat(rotY, -1.17809999f, 1.17809999);

	while (rotX > PI)
		rotX -= TAU;
	while (rotX < -PI)
		rotX += TAU;

	PS2_MEM_WriteFloat(AC2_ROTX, (float)rotX);
	PS2_MEM_WriteFloat(AC2_ROTY, (float)rotY);
}