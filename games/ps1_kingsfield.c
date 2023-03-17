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

#define KF_CAMY 0xA0838
#define KF_CAMX 0xA083A
// #define KF_IS_NOT_READING_MESSAGE 0x64B78
#define KF_CAN_MOVE_CAMERA 0x1FFF44

static uint8_t PS1_KF_Status(void);
static void PS1_KF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"King's Field",
	PS1_KF_Status,
	PS1_KF_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS1_KINGSFIELD = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_KF_Status(void)
{
	return (PS1_MEM_ReadWord(0xBA94) == 0x534C5053U && PS1_MEM_ReadWord(0xBA98) == 0x2D303030U && PS1_MEM_ReadWord(0xBA9C) == 0x31374B46U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_KF_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (!PS1_MEM_ReadHalfword(KF_IS_NOT_READING_MESSAGE))
	// 	return;

	if (!PS1_MEM_ReadHalfword(KF_CAN_MOVE_CAMERA))
		return;

	uint16_t camX = PS1_MEM_ReadHalfword(KF_CAMX);
	uint16_t camY = PS1_MEM_ReadHalfword(KF_CAMY);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 1.f;

	float dx = -(float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);

	// clamp y-axis
	// range is larger than game usually allows but feels better
	if (camYF > 600 && camYF < 32000)
		camYF = 600;
	if (camYF < 65000 && camYF > 32000)
		camYF = 65000;

	PS1_MEM_WriteHalfword(KF_CAMX, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(KF_CAMY, (uint16_t)camYF);
}