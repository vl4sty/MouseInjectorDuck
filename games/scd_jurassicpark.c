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

#define JP_CURSORX 0xA213A
#define JP_CURSORY 0xA213C
#define JP_SCROLL_SPEED 0x12862

static uint8_t SCD_JP_Status(void);
static void SCD_JP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Jurassic Park CD",
	SCD_JP_Status,
	SCD_JP_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_SCD_JURASSICPARK = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SCD_JP_Status(void)
{
	return (PS1_MEM_ReadWord(0x5920) == 0x31674649U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SCD_JP_Inject(void)
{
	// TODO: disable cursor movement during
	//			loading
	//			inventory

	uint16_t cursorX = PS1_MEM_ReadHalfword(JP_CURSORX);
	uint16_t cursorY = PS1_MEM_ReadHalfword(JP_CURSORY);

	// move screen when cursor is at screen edge
	if (cursorX < 129)
	{
		// if (xmouse <= 0 && camX < 129)
		if (xmouse <= 0)
			PS1_MEM_WriteHalfword(JP_SCROLL_SPEED, 0xFFFC);
		else
			PS1_MEM_WriteHalfword(JP_SCROLL_SPEED, 0x0);
		// if (xmouse > 0 && camX < 129)
		if (xmouse > 0)
			PS1_MEM_WriteHalfword(JP_CURSORX, (uint16_t)129);
		
		if (cursorX < 128)
			PS1_MEM_WriteHalfword(JP_CURSORX, (uint16_t)128);
	}

	if (cursorX > 430)
	{
		// if (xmouse <= 0 && camX < 129)
		if (xmouse >= 0)
			PS1_MEM_WriteHalfword(JP_SCROLL_SPEED, 0x4);
		else
			PS1_MEM_WriteHalfword(JP_SCROLL_SPEED, 0x0);
		// if (xmouse > 0 && camX < 129)
		if (xmouse < 0)
			PS1_MEM_WriteHalfword(JP_CURSORX, (uint16_t)430);
		
		if (cursorX > 431)
			PS1_MEM_WriteHalfword(JP_CURSORX, (uint16_t)431);
	}


	if (xmouse >= 0 && cursorX > 430)
		PS1_MEM_WriteHalfword(JP_SCROLL_SPEED, 0x4);


	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	float cursorXF = (float)cursorX;
	float cursorYF = (float)cursorY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 4.f;

	float dx = (float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&cursorXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity / scale;
	AccumulateAddRemainder(&cursorYF, &yAccumulator, ym, dy);

	cursorYF = ClampFloat(cursorYF, 176, 288);
	cursorXF = ClampFloat(cursorXF, 128, 431);

	// if (cursorXF < 128)
	// 	camXF = 128;
	// if (camXF > 431)
	// 	camXF = 431;

	PS1_MEM_WriteHalfword(JP_CURSORX, (uint16_t)cursorXF);
	PS1_MEM_WriteHalfword(JP_CURSORY, (uint16_t)cursorYF);

}