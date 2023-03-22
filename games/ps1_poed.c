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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define PI 3.14159265f // 0x40490FDB

#define POED_CAMY 0x89F94
#define POED_CAMY_SIGN 0x89F96
#define POED_CAMY2 0x89FB0
#define POED_CAMY2_SIGN 0x89FB2
#define POED_CAMX 0x9A6C0

#define POED_CAMX_SIN 0x9A6C0
#define POED_CAMX_SIN_SIGN 0x9A6C2
#define POED_CAMX_COS 0x9A6C8
#define POED_CAMX_COS_SIGN 0x9A6CA

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
static float totalAngle = -99.f;

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
	
	const float looksensitivity = (float)sensitivity / 200.f;

	if (xmouse)
	{
		uint16_t camXSin = PS1_MEM_ReadHalfword(POED_CAMX_SIN);
		uint16_t camXCos = PS1_MEM_ReadHalfword(POED_CAMX_COS);
		float camXSinF = (float)camXSin / 65536.f;
		float camXCosF = (float)camXCos / 65536.f;

		if (totalAngle == -99.f)
			totalAngle = atan(camXSinF / camXCosF);
		// TODO: add PI if cos/sin are negative

		totalAngle += (float)xmouse * looksensitivity / 20.f;

		while (totalAngle > (2 * PI))
			totalAngle -= (2 * PI);
		while (totalAngle < 0)
			totalAngle += (2 * PI);

		camXSinF = sin(totalAngle) * 65536.f;
		camXCosF = cos(totalAngle) * 65536.f;

		if (totalAngle > PI / 2 && totalAngle < (3 * PI) / 2)
			PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0xFFFF);
		else
			PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0x0);

		if (totalAngle > PI && totalAngle < (2 * PI))
			PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0xFFFF);
		else
			PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0x0);
		

		// if (angle > PI / 2)
		// {
		// 	PS1_MEM_WriteHalfword(POED_CAMX_SIN_SIGN, 0xFFFF);
		// 	PS1_MEM_WriteHalfword(POED_CAMX_COS_SIGN, 0xFFFF);
		// }

		PS1_MEM_WriteHalfword(POED_CAMX_SIN, (uint16_t)camXSinF);
		PS1_MEM_WriteHalfword(POED_CAMX_COS, (uint16_t)camXCosF);
	}

	if (ymouse)
	{
		uint16_t camY = PS1_MEM_ReadHalfword(POED_CAMY2);
		float camYF = (float)camY;
		// if (!lastCamY)
		// 	lastCamY = camY;

		float ym = (float)(invertpitch ? -ymouse : ymouse);
		float dy = -ym * looksensitivity * 5000;
		AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

		while (camYF < 0)
			camYF += 65536;

		uint16_t camYSign = (uint16_t)0x0000;
		if ((uint16_t)camYF >= (uint16_t)32766)
			camYSign = (uint16_t)0xFFFF;

		if (camYSign == 0xFFFF && camYF < 36300)
			camYF = 36300;
		if (camYSign == 0x0 && camYF > 29000)
			camYF = 29000;

		PS1_MEM_WriteHalfword(POED_CAMY_SIGN, (uint16_t)camYSign);
		PS1_MEM_WriteHalfword(POED_CAMY2_SIGN, (uint16_t)camYSign);

		

		PS1_MEM_WriteHalfword(POED_CAMY, (uint16_t)camYF);
		PS1_MEM_WriteHalfword(POED_CAMY2, (uint16_t)camYF);
	}
}