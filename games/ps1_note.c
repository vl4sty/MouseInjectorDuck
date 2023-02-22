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

#define NOTE_CAMY 0x17D310
#define NOTE_CAMX 0x17D312
#define NOTE_IS_PAUSED_TRANSITION_CUTSCENE 0x1FB6AE

static uint8_t PS1_NOTE_Status(void);
static void PS1_NOTE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"The Note",
	PS1_NOTE_Status,
	PS1_NOTE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_THENOTE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_NOTE_Status(void)
{
	return (PS1_MEM_ReadWord(0x9364) == 0x534C4553U && PS1_MEM_ReadWord(0x9368) == 0x5F303037 && PS1_MEM_ReadWord(0x936C) == 0x2E34393BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_NOTE_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// don't move camera on pause menu, during screen transition, or during cutscene
	if (PS1_MEM_ReadHalfword(NOTE_IS_PAUSED_TRANSITION_CUTSCENE))
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(NOTE_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(NOTE_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;

	float dx = -(float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	// if (camYF > 800 && camYF < 32000)
	// 	camYF = 800;
	// if (camYF < 64735 && camYF > 32000)
	// 	camYF = 64735;

	PS1_MEM_WriteHalfword(NOTE_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(NOTE_CAMY, (uint16_t)camYF);
}