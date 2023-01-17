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

#define TAP_slingshootercursorx 0x0029A2 // 0-255
#define TAP_slingshootercursory 0x0029A6 // 0-223
#define TAP_hubcursorx 0x002A6E // 0-243
#define TAP_hubcursory 0x002A72 // 0-211
#define TAP_screen 0x000096 // 0=hub, 3=slingshooter
#define TAP_screen_hub 0x0
#define TAP_screen_slingshooter 0x3

static uint8_t SNES_TAP_Status(void);
static void SNES_TAP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Timon & Pumbaa's Jungle Games",
	SNES_TAP_Status,
	SNES_TAP_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_SNES_TIMONANDPUMBAA = &GAMEDRIVER_INTERFACE;

static uint32_t playerbase = 0;
static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SNES_TAP_Status(void)
{
	return (SNES_MEM_ReadWord(0x50A) == 0x55DA); // 1 random static address
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SNES_TAP_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	if (SNES_MEM_ReadWord(TAP_screen) == TAP_screen_hub) // 4 is in-game, any other value is pause, screen transition, etc..
	{
		uint16_t cursorXInt = SNES_MEM_ReadWord(TAP_hubcursorx);
		uint16_t cursorYInt = SNES_MEM_ReadWord(TAP_hubcursory);
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

		cursorX = ClampFloat(cursorX, 1.f, 243.f);
		cursorY = ClampFloat(cursorY, 1.f, 211.f);

		SNES_MEM_WriteWord(TAP_hubcursorx, (uint16_t)cursorX);
		SNES_MEM_WriteWord(TAP_hubcursory, (uint16_t)cursorY);
	} else if (SNES_MEM_ReadWord(TAP_screen) == TAP_screen_slingshooter)
	{
		uint16_t cursorXInt = SNES_MEM_ReadWord(TAP_slingshootercursorx);
		uint16_t cursorYInt = SNES_MEM_ReadWord(TAP_slingshootercursory);
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

		cursorX = ClampFloat(cursorX, 1.f, 255.f);
		cursorY = ClampFloat(cursorY, 1.f, 223.f);

		SNES_MEM_WriteWord(TAP_slingshootercursorx, (uint16_t)cursorX);
		SNES_MEM_WriteWord(TAP_slingshootercursory, (uint16_t)cursorY);
	}
}