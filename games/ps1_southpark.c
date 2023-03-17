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

// #define SP_CAMX 0xA5324
// #define SP_CAMY 0xA5328
#define SP_CAMX 0xA5326
#define SP_CAMY 0xA532A
#define SP_FOV 0x98984
#define SP_IS_UNPAUSED 0xA7648
// #define SP_CAMY_SLOPE_ADJUST 0x186B4
// #define SP_CAMY_REBOUND 0x18B6C

static uint8_t PS1_SP_Status(void);
static void PS1_SP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"South Park",
	PS1_SP_Status,
	PS1_SP_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_SOUTHPARK = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_SP_Status(void)
{
	return (PS1_MEM_ReadWord(0x925C) == 0x534C5553U && PS1_MEM_ReadWord(0x9260) == 0x5F303039U && PS1_MEM_ReadWord(0x9264) == 0x2E33363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_SP_Inject(void)
{
	// TODO: fix chicken sniper unzooming on diagonal movement
	//			seems to be a feature of the game

	// disable annoying camY rebound at camY min
	// PS1_MEM_WriteWord(SP_CAMY_REBOUND, 0x0);
	// // disable camY adjust when move to a slope of a different angle
	// PS1_MEM_WriteWord(SP_CAMY_SLOPE_ADJUST, 0x0);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS1_MEM_ReadByte(SP_IS_UNPAUSED))
		return;
	
	uint16_t camX = PS1_MEM_ReadHalfword(SP_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(SP_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float fov = (300.f / (float)PS1_MEM_ReadHalfword(SP_FOV));
	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 10.f;

	float dx = -(float)xmouse * looksensitivity * scale * fov;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale * fov;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);


	// clamp y-axis
	// if (camYF > 24320)
	// 	camYF = 24320;

	PS1_MEM_WriteHalfword(SP_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(SP_CAMY, (uint16_t)camYF);
}