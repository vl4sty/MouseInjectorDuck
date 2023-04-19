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

#define TAU 6.2831853f // 0x40C90FDB

#define HAZE_CAMBASE 0x7FC0108
#define HAZE_CAMBASE_SANITY_1_VALUE 0x00001BB1
#define HAZE_CAMBASE_SANITY_2_VALUE 0x40933333
// offsets from camBase
#define HAZE_CAMBASE_SANITY_1 0x30
#define HAZE_CAMBASE_SANITY_2 0x80
#define HAZE_CAMY 0x104
#define HAZE_CAMX 0x108
#define HAZE_FOV 0xD0

// #define HAZE_CAMY 0x802F48C
// #define HAZE_CAMX 0x802F490
// #define HAZE_FOV 0x802F458

static uint8_t PS3_HAZE_Status(void);
static uint8_t PS3_HAZE_DetectCambase(void);
static void PS3_HAZE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"HAZE",
	PS3_HAZE_Status,
	PS3_HAZE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS3_HAZE = &GAMEDRIVER_INTERFACE;

uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS3_HAZE_Status(void)
{
	return (PS3_MEM_ReadUInt(0x7669068) == 0x48415A45U && 
			PS3_MEM_ReadUInt(0x766906C) == 0x00505245U);
	// return (PS3_MEM_ReadUInt(0x0) == 0x4U || PS3_MEM_ReadUInt(0x0) == 0x00000004U);
}

static uint8_t PS3_HAZE_DetectCambase(void)
{
	uint32_t tempCamBase = PS3_MEM_ReadPointer(HAZE_CAMBASE);
	if (tempCamBase &&
		PS3_MEM_ReadUInt(tempCamBase + HAZE_CAMBASE_SANITY_1) == HAZE_CAMBASE_SANITY_1_VALUE)// &&)
		// PS3_MEM_ReadUInt(tempCamBase + HAZE_CAMBASE_SANITY_2) == HAZE_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS3_HAZE_Inject(void)
{
	// TODO: determine when in different seats of buggy to clamp different values
	// TODO: buggy car
	// 			side seat camX
	//			turret
	// TODO: disable during
	//			pause
	//			lever pull/contextual action animation
	// TODO: shake mouse to put out flames
	//			need to use DS3/4 and check for value
	// FIXME: camBase broken on buggy level when on fire?

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (!PS3_HAZE_DetectCambase())
		return;

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 600.f;
	const float fov = PS3_MEM_ReadFloat(camBase + HAZE_FOV) / 0.6000000238f;

	float camY = PS3_MEM_ReadFloat(camBase + HAZE_CAMY);
	float ym = (float)(invertpitch ? -ymouse : ymouse);
	camY -= ym * looksensitivity / scale * fov;
	camY = ClampFloat(camY, -1.25, 1.149999976);

	float camX = PS3_MEM_ReadFloat(camBase + HAZE_CAMX);
	camX -= (float)xmouse * looksensitivity / scale * fov;

	// float camXSin = PS3_MEM_ReadFloat(HAZE_CAMX_SIN);
	// float camXCos = PS3_MEM_ReadFloat(HAZE_CAMX_COS);

	// // keep track of total rotation angle since it is not kept in-game
	// float angle = atan(camXSin / camXCos);
	// if (camXCos < 0)
	// 	angle += TAU / 2.f;

	// float angleChange = (float)xmouse * looksensitivity / scale;

	// // angle += angleChange;
	// angle += angleChange;
	// // TODO: while totalAngle > or < TAU, -TAU

	// camXSin = sin(angle);
	// camXCos = cos(angle);

	// PS3_MEM_WriteFloat(HAZE_CAMX_SIN, (float)camXSin);
	// PS3_MEM_WriteFloat(HAZE_CAMX_COS, (float)camXCos);

	PS3_MEM_WriteFloat(camBase + HAZE_CAMY, camY);
	PS3_MEM_WriteFloat(camBase + HAZE_CAMX, camX);
}