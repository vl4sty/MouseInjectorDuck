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

#define DIS_CAMX 0x77624
#define DIS_CAMX_SPEED 0x77604
#define DIS_CAMX_VEL 0x77618
#define DIS_DIRX 0x77671

#define DIS_IS_LOADING 0x5B900
#define DIS_IS_PAUSED 0x7145C

static uint8_t PS1_DIS_Status(void);
static void PS1_DIS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Disruptor",
	PS1_DIS_Status,
	PS1_DIS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_DISRUPTOR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_DIS_Status(void)
{
	return (PS1_MEM_ReadWord(0x93AC) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x93B0) == 0x5F303032U && 
			PS1_MEM_ReadWord(0x93B4) == 0x2E32343BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_DIS_Inject(void)
{
	if(xmouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadWord(DIS_IS_LOADING))
		return;
	
	if (PS1_MEM_ReadHalfword(DIS_IS_PAUSED))
		return;
	
	uint8_t camX = PS1_MEM_ReadByte(DIS_CAMX);
	float camXF = (float)camX;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 12.f;

	float dx = -(float)xmouse * looksensitivity / scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	PS1_MEM_WriteByte(DIS_CAMX, (uint8_t)camXF);
}