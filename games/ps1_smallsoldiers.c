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

#define SS_CAMX 0xE124A // rotX
#define SS_CAMY 0xE1426
#define SS_CAMX2 0xE279A
#define SS_IS_FIRST_PERSON 0xE242C
#define SS_IS_HANGING 0xE1118

static uint8_t PS1_SS_Status(void);
static void PS1_SS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Small Soldiers",
	PS1_SS_Status,
	PS1_SS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_SMALLSOLDIERS = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_SS_Status(void)
{
	// SLUS_007.81;
	return (PS1_MEM_ReadWord(0x9304) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9308) == 0x5F303037U && 
			PS1_MEM_ReadWord(0x930C) == 0x2E38313BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_SS_Inject(void)
{
	// TODO: disabled during
	//			hang
	//			pause
	// TODO: instant camera update?
	// TODO: only change Y when in first person
	// TODO: camera base

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadByte(SS_IS_HANGING) == 0x1)
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(SS_CAMX);
	float camXF = (float)camX;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	// while (camXF > 4096)
	// 	camXF -= 4096;
	// while (camXF < 0)
	// 	camXF += 4096;

	PS1_MEM_WriteHalfword(SS_CAMX, (uint16_t)camXF);
	// PS1_MEM_WriteHalfword(SS_CAMX2, (uint16_t)camXF);

	if (PS1_MEM_ReadByte(SS_IS_FIRST_PERSON))
	{
		int16_t camY = PS1_MEM_ReadInt16(SS_CAMY);
		float camYF = (float)camY;

		float ym = -(float)(invertpitch ? -ymouse : ymouse);
		float dy = ym * looksensitivity * scale;
		AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

		camYF = ClampFloat(camYF, -512.f, 512.f);
		PS1_MEM_WriteInt16(SS_CAMY, (int16_t)camYF);
	}
}