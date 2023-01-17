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

#define TAU 6.2831853f // 0x40C90FDB

#define UT_MISSION1CAMYLOW 0x18
#define UT_MISSION1CAMYHIGH 0x8C
// #define UT_MISSION3CAMYLOW 0x18
#define UT_MISSION3CAMYHIGH 0xC0
#define UT_MISSION5CAMYHIGH 0xC8

// STATIC addresses
#define UT_cursorx 0x001AD1
#define UT_cursory 0x001AD3
#define UT_screenleft 0x000065
#define UT_currentlevel 0x00184B // 0=lvl1, 1=lvl2, 2=lvl3, 3=lvl4, 4=lvl5(?), 5=mainmenu
#define UT_wallaction 0x0000F0
#define UT_currentsection 0x0000C4 // 0=section 1, 1 = section 2, ...

static uint8_t SNES_UT_Status(void);
static void SNES_UT_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"The Untouchables",
	SNES_UT_Status,
	SNES_UT_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_UNTOUCHABLES = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.;
static float yAccumulator = 0.;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_UT_Status(void)
{
	return (SNES_MEM_ReadWord(0xD0) == 0xBB00 && SNES_MEM_ReadWord(0xD2) == 0x1882);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_UT_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	uint16_t currentlevel = SNES_MEM_ReadWord(UT_currentlevel);
	uint8_t currentsection = SNES_MEM_ReadByte(UT_currentsection);

	// if ((currentlevel != 0) && (currentlevel != 1) && (currentlevel != 2) && (currentlevel != 4))
	if (currentlevel == 3) // return if in mission 4, no shooting section
		return;

	if ((currentlevel == 1) && (currentsection != 6)) // return if not at the shooting section of mission 2
		return;

	if ((currentlevel == 4) && (SNES_MEM_ReadWord(UT_wallaction) != 0xE000)) // return if not peaking out from wall on mission 5
		return;

	uint16_t camylow = UT_MISSION1CAMYLOW;
	uint16_t camyhigh = UT_MISSION1CAMYHIGH;
	if (currentlevel == 0x2)
		camyhigh = UT_MISSION3CAMYHIGH;
	else if (currentlevel == 0x4)
		camyhigh = UT_MISSION5CAMYHIGH;

	const float looksensitivity = (float)sensitivity / 40.f;

	uint16_t cursorXInt = SNES_MEM_ReadWord(UT_cursorx);
	uint16_t cursorYInt = SNES_MEM_ReadWord(UT_cursory);
	float cursorX = (float)cursorXInt;
	float cursorY = (float)cursorYInt;

	if (xmouse != 0)
	{
		float dx = (float)xmouse * looksensitivity;
		if (xmouse < 0)
			cursorX += ceil(dx);
		else
			cursorX += (uint16_t)dx;

		float r = fmod(dx, 1.f);

		if (abs(r + xAccumulator) >= 1)
		{
			if (xmouse > 0)
				cursorX += 1;
			else
				cursorX -= 1;
		}
	
		xAccumulator = fmod(r + xAccumulator, 1.f);
	}

	if (ymouse != 0)
	{
		int ym = (invertpitch ? -ymouse : ymouse);
		float dy = (float)ym * looksensitivity;
		// if (ymouse < 0)
		if (ym < 0)
			cursorY += ceil(dy);
		else
			cursorY += (uint16_t)dy;

		float r = fmod(dy, 1.f);

		if (abs(r + yAccumulator) >= 1)
		{
			if (ym > 0)
				cursorY += 1;
			else
				cursorY -= 1;
		}
		
		yAccumulator = fmod(r + yAccumulator, 1.f);
	}

	uint16_t screenleft = SNES_MEM_ReadWord(UT_screenleft);

	cursorX = ClampFloat(cursorX, (float)screenleft, (float)screenleft + 255.f);
	cursorY = ClampFloat(cursorY, camylow, camyhigh);

	SNES_MEM_WriteWord(UT_cursorx, (uint16_t)cursorX);
	SNES_MEM_WriteWord(UT_cursory, (uint16_t)cursorY);
}