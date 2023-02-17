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

#define CW_IS_PAUSED 0x7BD18C

#define CW_CAMBASE 0x715688
#define CW_CAMBASE_SANITY_1_VALUE 0x78166B00
#define CW_CAMBASE_SANITY_2_VALUE 0x8A6F443F
// offsets from cambase
#define CW_CAMBASE_SANITY_1 0xC
#define CW_CAMBASE_SANITY_2 0x70
#define CW_CAMY 0x9AC
#define CW_CAMX 0x9B0
#define CW_FOV 0xA5C

#define CW_FRAMERATE_BASE 0x720334
// offsets from framerate base
#define CW_FRAMERATE_FPS 0x8C // 1=60,2=30

static uint8_t PS2_CW_Status(void);
static uint8_t PS2_CW_DetectCambase(void);
static void PS2_CW_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Cold Winter",
	PS2_CW_Status,
	PS2_CW_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_COLDWINTER = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
// static uint32_t framerateBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_CW_Status(void)
{
	// BASLUS-20845
	return (PS2_MEM_ReadWord(0x005D2AC0) == 0x4241534CU && PS2_MEM_ReadWord(0x005D2AC4) == 0x55532D32U) &&
			PS2_MEM_ReadWord(0x005D2AC8) == 0x30383435U;
}

static uint8_t PS2_CW_DetectCambase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(CW_CAMBASE);
	if (tempCamBase != 0)
	{
		if (PS2_MEM_ReadWord(tempCamBase + CW_CAMBASE_SANITY_1) == CW_CAMBASE_SANITY_1_VALUE &&
			PS2_MEM_ReadWord(tempCamBase + CW_CAMBASE_SANITY_2) == CW_CAMBASE_SANITY_2_VALUE)
		{
			camBase = tempCamBase;
			return 1;
		}
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_CW_Inject(void)
{
	// TODO: gun sway
	// TODO: disable during in-game cutscenes, there are a few at end of level 2 during helicopter scene

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (PS2_MEM_ReadUInt(CW_IS_PAUSED) == 0x2) // return if paused | 2=true, 0=false
		return;
	
	if (!PS2_CW_DetectCambase())
		return;

	float fov = PS2_MEM_ReadFloat(camBase + CW_FOV);
	float looksensitivity = (float)sensitivity / 140.f;

	float camY = PS2_MEM_ReadFloat(camBase + CW_CAMY);
	float camX = PS2_MEM_ReadFloat(camBase + CW_CAMX);

	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov;
	camX -= (float)xmouse * looksensitivity * fov;

	PS2_MEM_WriteFloat(camBase + CW_CAMY, camY);
	PS2_MEM_WriteFloat(camBase + CW_CAMX, camX);

}