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

#define PS_CAMY 0x632AC
#define PS_CAMX 0x632A8

static uint8_t SS_PS_Status(void);
static void SS_PS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PowerSlave",
	SS_PS_Status,
	SS_PS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_SS_POWERSLAVE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t SS_PS_Status(void)
{
	return (PS1_MEM_ReadWord(0x0) == 0x00064A09U && PS1_MEM_ReadWord(0x4) == 0x00064A09U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void SS_PS_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// uint32_t camXFixedPoint = PS1_MEM_ReadWord(PS_CAMX);
	// 00FF 0000
	
	uint16_t camX = PS1_MEM_ReadHalfword(PS_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(PS_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 10000.f;

	float dx = -(float)xmouse * looksensitivity;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	// if (camYF > 800 && camYF < 32000)
	// 	camYF = 800;
	// if (camYF < 64735 && camYF > 32000)
	// 	camYF = 64735;

	// PS1_MEM_WriteHalfword(PS_CAMX, (uint16_t)camXF);
	// PS1_MEM_WriteHalfword(PS_CAMY, (uint16_t)camYF);

	uint8_t camXInteger = PS1_MEM_ReadByte(PS_CAMX);
	uint16_t camXFraction = PS1_MEM_ReadHalfword(PS_CAMX + 0x2);
	uint32_t camX24 = ((uint32_t)camXInteger << 16) + (uint32_t)camXFraction;

	// uint16_t preCalcFraction = camXFraction;
	float dFx = -(float)xmouse * looksensitivity * scale; // change in fraction part

	camX24 += dFx;
	uint32_t camXInt = (camX24 & 0xFF0000) >> 16;
	uint32_t camXFrac = camX24 & 0xFFFF;


	// while (dFx > 0xFFFF)
	// {
	// }

	// camXFraction += -(float)xmouse * looksensitivity * scale;
	// PS1_MEM_WriteHalfword(PS_CAMX + 0x2, camXFraction);
	PS1_MEM_WriteByte(PS_CAMX, (uint8_t)camXInt);
	PS1_MEM_WriteHalfword(PS_CAMX + 0x2, (uint16_t)camXFrac);


	uint8_t camYInteger = PS1_MEM_ReadByte(PS_CAMY);
	uint8_t camYSign = PS1_MEM_ReadByte(PS_CAMY + 0x1);
	uint16_t camYFraction = PS1_MEM_ReadHalfword(PS_CAMY + 0x2);
	uint32_t camY24 = ((uint32_t)camYInteger << 16) + (uint32_t)camYFraction;

	float dFy = -(float)ymouse * looksensitivity * scale; // change in fraction part

	camY24 += dFy;
	if (camY24 > 0xFFFFFF)
	{
		if (camYSign == 0xFF)
			camYSign = 0x0;
		else
			camYSign = 0x0;
	}
	uint32_t camYInt = (camY24 & 0xFF0000) >> 16;
	uint32_t camYFrac = camY24 & 0xFFFF;

	PS1_MEM_WriteByte(PS_CAMY, (uint8_t)camYInt);
	PS1_MEM_WriteHalfword(PS_CAMY + 0x2, (uint16_t)camYFrac);

}