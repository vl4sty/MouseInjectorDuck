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

#define PS_CAMY 0x84BBE
#define PS_CAMX 0x8E3C0

#define PS_IS_FOCUSED 0x7ADB0
#define PS_IS_PAUSED 0x7AFA4
#define PS_IS_INVENTORY 0x7AB88
#define PS_IS_NOT_LEVEL_END 0x1FFF50

static uint8_t PS1_PS_Status(void);
static void PS1_PS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Powerslave",
	PS1_PS_Status,
	PS1_PS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_POWERSLAVE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_PS_Status(void)
{
	// SLUS_001.02
	return (PS1_MEM_ReadWord(0x934C) == 0x534C5553U && 
			PS1_MEM_ReadWord(0x9350) == 0x5F303031U && 
			PS1_MEM_ReadWord(0x9354) == 0x2E30323BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_PS_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadByte(PS_IS_FOCUSED))
		return;
	
	if (PS1_MEM_ReadByte(PS_IS_PAUSED))
		return;

	if (PS1_MEM_ReadByte(PS_IS_INVENTORY))
		return;

	if (!PS1_MEM_ReadWord(PS_IS_NOT_LEVEL_END))
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(PS_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(PS_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;

	float dx = -(float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);
	while (camXF > 4096.f)
		camXF -= 4096.f;
	while (camXF < 0.f)
		camXF += 4096.f;

	float ym = -(float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);
	if (camYF > 1024 && camYF < 2200)
		camYF = 1024;
	if (camYF < 3072 && camYF > 2200)
		camYF = 3072;

	PS1_MEM_WriteHalfword(PS_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(PS_CAMY, (uint16_t)camYF);
}