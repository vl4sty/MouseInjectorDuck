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

#define PD_CAMERA_VIEW 0x4D770 // front is zero, side is non-zero
#define PD_CURSORY 0x4D77E
#define PD_CURSORX 0x4D780
#define PD_FRONT_CURSORY 0x4D77A
#define PD_FRONT_CURSORX 0x4D77C
#define PD_FRONT_CURSORY_ACTUAL 0x4D8CA
#define PD_FRONT_CURSORX_ACTUAL 0x4D8CC

#define PD_CAMX_TARGET 0x4D7EC

static uint8_t SS_PD_Status(void);
static void SS_PD_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Panzer Dragoon",
	SS_PD_Status,
	SS_PD_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_SS_PANZERDRAGOON = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SS_PD_Status(void)
{
	return (PS1_MEM_ReadWord(0xC60) == 0x41505A4EU && 
			PS1_MEM_ReadWord(0xC64) == 0x52454420U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SS_PD_Inject(void)
{
	// TODO: camera rotates at extremes of cursor movement
	//			buffer zone around extremes that will move camera
	//			around 800-1020, any cursor movement towards edge will move camera
	// TODO: forward camera cursor is different than sides and rear

	int16_t cursorX = PS1_MEM_ReadInt16(PD_CURSORX);
	float cursorXF = (float)cursorX;
	int16_t camX = PS1_MEM_ReadInt16(PD_CAMX_TARGET);
	if (cursorXF > 1000)
		camX += 1;
	if (cursorXF < -1000)
		camX -= 1;
	PS1_MEM_WriteInt16(PD_CAMX_TARGET, (float)camX);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	float scale = 10.f;
	// float scale = 1.f;

	int16_t cursorY;
	uint32_t cursorYAddress, cursorXAddress;
	uint32_t camView = PS1_MEM_ReadUInt(PD_CAMERA_VIEW);

	if (camView)	
	{
		cursorYAddress = PD_CURSORY;
		cursorXAddress = PD_CURSORX;
	}
	else
	{
		cursorYAddress = PD_FRONT_CURSORY;
		cursorXAddress = PD_FRONT_CURSORX;
	}

	cursorY = PS1_MEM_ReadInt16(cursorYAddress);
	cursorX = PS1_MEM_ReadInt16(cursorXAddress);

	// int16_t cursorY = PS1_MEM_ReadInt16(PD_CURSORY);
	float cursorYF = (float)cursorY;
	int ym = (invertpitch ? ymouse : -ymouse);
	float dy = (float)ym * looksensitivity * scale;
	AccumulateAddRemainder(&cursorYF, &yAccumulator, ym, dy);
	cursorYF = ClampFloat(cursorYF, -1020, 1020);
	// PS1_MEM_WriteInt16(PD_CURSORY, (float)cursorYF);
	PS1_MEM_WriteInt16(cursorYAddress, (float)cursorYF);

	// cursorX = PS1_MEM_ReadInt16(PD_CURSORX);
	cursorXF = (float)cursorX;
	// int16_t cursorX = PS1_MEM_ReadInt16(PD_CURSORX);
	// float cursorXF = (float)cursorX;
	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&cursorXF, &xAccumulator, -xmouse, dx);
	cursorXF = ClampFloat(cursorXF, -1020, 1020);
	// PS1_MEM_WriteInt16(PD_CURSORX, (float)cursorXF);
	PS1_MEM_WriteInt16(cursorXAddress, (float)cursorXF);

	if (!camView)
	{
		PS1_MEM_WriteInt16(PD_FRONT_CURSORY_ACTUAL, (float)cursorYF);
		PS1_MEM_WriteInt16(PD_FRONT_CURSORX_ACTUAL, (float)cursorXF);
	}

	// int16_t camX = PS1_MEM_ReadInt16(PD_CAMX_TARGET);
	// if (cursorXF > 800)
	// 	camX += 1;
	// if (cursorXF < -800)
	// 	camX -= 1;
	// PS1_MEM_WriteInt16(PD_CAMX_TARGET, (float)camX);
}