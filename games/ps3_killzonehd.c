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

#define KZ_CAMY 0x88606CC
#define KZ_CAMX_COS 0x6549DE8
#define KZ_CAMX_SIN 0x6549DEC

#define KZ_ON_FOOT_ANGLE_UNSET -99

static uint8_t PS3_KZ_Status(void);
static void PS3_KZ_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Killzone HD",
	PS3_KZ_Status,
	PS3_KZ_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS3_KILLZONEHD = &GAMEDRIVER_INTERFACE;

static float onFootAngle = KZ_ON_FOOT_ANGLE_UNSET;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS3_KZ_Status(void)
{
	return (PS3_MEM_ReadUInt(0x107EA0) == 0x50554138U && PS3_MEM_ReadUInt(0x107EA4) == 0x30383536U);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS3_KZ_Inject(void)
{
	// TODO: fix flickering for camX, maybe find the angle in memory and add to and set based on that
	// TODO: clamp camY
	// TODO: scale camY to match camX
	//			camY is on a -1 to 1 scale
	//			camX is a sin/cos angle
	// TODO: find FOV
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	const float looksensitivity = (float)sensitivity / 20.f;

	float camY = PS3_MEM_ReadFloat(KZ_CAMY);
	float ym = (float)(invertpitch ? -ymouse : ymouse);
	camY -= ym * looksensitivity / 200.f;

	float camXSin = PS3_MEM_ReadFloat(KZ_CAMX_SIN);
	float camXCos = PS3_MEM_ReadFloat(KZ_CAMX_COS);

	// keep track of total rotation angle since it is not kept in-game
	float angle = atan(camXSin / camXCos);
	if (onFootAngle == KZ_ON_FOOT_ANGLE_UNSET) {
		onFootAngle = angle;
	}

	float angleChange = (float)xmouse * looksensitivity / 200.f;

	// angle += angleChange;
	onFootAngle += angleChange;
	// TODO: while totalAngle > or < TAU, -TAU

	camXSin = sin(onFootAngle);
	camXCos = cos(onFootAngle);

	PS3_MEM_WriteFloat(KZ_CAMX_SIN, (float)camXSin);
	PS3_MEM_WriteFloat(KZ_CAMX_COS, (float)camXCos);

	PS3_MEM_WriteFloat(KZ_CAMY, camY);
}