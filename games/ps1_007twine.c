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

#define TWINE_CAMY 0xB60F8
#define TWINE_CAMY_AIM 0xB7148
#define TWINE_CAMX 0x16BB62
#define TWINE_CAMX_AIM 0xB7140

#define TWINE_IS_IN_GAME_CUTSCENE 0xA118C
#define TWINE_IS_INTERACTION 0xA0F34

static uint8_t PS1_TWINE_Status(void);
static void PS1_TWINE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"007 The World Is Not Enough",
	PS1_TWINE_Status,
	PS1_TWINE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_007THEWORLDISNOTENOUGH = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_TWINE_Status(void)
{
	// SLUS_012.72
	return (PS1_MEM_ReadWord(0x9394) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9398) == 0x5F303132U && 
			PS1_MEM_ReadWord(0x939C) == 0x2E37323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_TWINE_Inject(void)
{
	// TODO: disable during
	//			pause
	//			in-game cutscenes
	// TODO: camBase
	// TODO: cheats for each level?
	//			look for cheat base
	// FIXME: camY popping?
	// TODO: use menu (60 FPS) to find FPS cheat

	// disable camY rebound level 1?, only works on level reset
	// needs to be a cheat but keep here as reference for cheat base search
	// PS1_MEM_WriteWord(0x52120, 0x0);
	// PS1_MEM_WriteWord(0x5236C, 0x0);
	// PS1_MEM_WriteWord(0x527FC, 0x0);
	// PS1_MEM_WriteWord(0x52A58, 0x0);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadHalfword(TWINE_IS_IN_GAME_CUTSCENE))
		return;

	if (PS1_MEM_ReadHalfword(TWINE_IS_INTERACTION))
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(TWINE_CAMX);
	int16_t camY = PS1_MEM_ReadInt16(TWINE_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	while (camXF > 4096)
		camXF -= 4096;
	while (camXF < 0)
		camXF += 4096;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	// TODO: only clamp when in-game, not during in-game cutscenes
	camYF = ClampFloat(camYF, -640.f, 800.f);

	PS1_MEM_WriteHalfword(TWINE_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(TWINE_CAMX_AIM, (uint16_t)camXF);
	PS1_MEM_WriteInt16(TWINE_CAMY, (int16_t)camYF);
	PS1_MEM_WriteInt(TWINE_CAMY_AIM, (int32_t)camYF);
}