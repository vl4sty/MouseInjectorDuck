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

#define POED_CAMY 0x89F94
#define POED_CAMY_SIGN 0x89F96
#define POED_CAMY2 0x89FB0
#define POED_CAMY2_SIGN 0x89FB2
#define POED_CAMX 0x9A6C0

#define POED_ALLOW_MOUSE_MOVEMENT 0x89E62

static uint8_t PS1_POED_Status(void);
static void PS1_POED_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"PO'ed",
	PS1_POED_Status,
	PS1_POED_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_POED = &GAMEDRIVER_INTERFACE;

static uint16_t lastCamY = 0;
static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_POED_Status(void)
{
	return (PS1_MEM_ReadWord(0x946C) == 0x534C5553U && PS1_MEM_ReadWord(0x9470) == 0x5F303030 && PS1_MEM_ReadWord(0x9474) == 0x2E39373BU); // SLUS_000.97;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_POED_Inject(void)
{
	// TODO: prevent whatever opcode from rubberbanding camY
	//			should just be able to set it and have it remain the same

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	PS1_MEM_WriteHalfword(POED_ALLOW_MOUSE_MOVEMENT, 0xFFFF);
	
	uint16_t camX = PS1_MEM_ReadHalfword(POED_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(POED_CAMY2);
	float camXF = (float)camX;
	float camYF = (float)camY;
	if (!lastCamY)
		lastCamY = camY;

	const float looksensitivity = (float)sensitivity / 20.f;

	float dx = -(float)xmouse * looksensitivity * 100;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * 100;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	if (lastCamY < (uint16_t)32760 && camYF > (uint16_t)32760)
		camYF = (uint16_t)32760;

	uint16_t camYSign = (uint16_t)0x0000;
	if ((uint16_t)camYF >= (uint16_t)32766)
		camYSign = (uint16_t)0xFFFF;
	
	PS1_MEM_WriteHalfword(POED_CAMY_SIGN, (uint16_t)camYSign);
	PS1_MEM_WriteHalfword(POED_CAMY2_SIGN, (uint16_t)camYSign);

	// clamp y-axis
	// if (camYF > 800 && camYF < 32000)
	// 	camYF = 800;
	// if (camYF < 64735 && camYF > 32000)
	// 	camYF = 64735;

	// PS1_MEM_WriteHalfword(POED_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(POED_CAMY, (uint16_t)camYF);
	PS1_MEM_WriteHalfword(POED_CAMY2, (uint16_t)camYF);
}