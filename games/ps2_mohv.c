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

#define MOHV_ONFOOT_BASE 0x620D48
#define MOHV_ONFOOT_SANITY_1_VALUE 0xA8C55200
#define MOHV_ONFOOT_SANITY_2_VALUE 0x30C95200
// offsets
#define MOHV_ONFOOT_CAMY 0x110
#define MOHV_ONFOOT_CAMX 0x12C
#define MOHV_ONFOOT_FOV 0x170
#define MOHV_ONFOOT_AIM_ASSIST 0x1D0
#define MOHV_ONFOOT_SANITY_1 0x20
#define MOHV_ONFOOT_SANITY_2 0x30
#define MOHV_ONFOOT_SANITY_3 0x70

#define MOHV_INGAME_CUTSCENE 0x5B4198

static uint8_t PS2_MOHV_Status(void);
static uint8_t PS2_MOHV_DetectCamera(void);
static void PS2_MOHV_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Medal of Honor: Vanguard",
	PS2_MOHV_Status,
	PS2_MOHV_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_MOHVANGUARD = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_MOHV_Status(void)
{
	// SLUS_215.97
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && PS2_MEM_ReadWord(0x00093394) == 0x5F323135U) &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E39373BU;
}

static uint8_t PS2_MOHV_DetectCamera(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadPointer(MOHV_ONFOOT_BASE);
	if (PS2_MEM_ReadWord(tempCamBase + MOHV_ONFOOT_SANITY_1) == MOHV_ONFOOT_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + MOHV_ONFOOT_SANITY_2) == MOHV_ONFOOT_SANITY_2_VALUE &&
		tempCamBase != 0)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_MOHV_Inject(void)
{
	if (!PS2_MOHV_DetectCamera())
		return;

	// return if an in-game cutscene is happening
	if (PS2_MEM_ReadUInt(MOHV_INGAME_CUTSCENE) == 0x0)
		return;
	
	// disable aim-assist
	PS2_MEM_WriteUInt(camBase + MOHV_ONFOOT_AIM_ASSIST, 0x0);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	float fov = PS2_MEM_ReadFloat(camBase + MOHV_ONFOOT_FOV);
	float looksensitivity = (float)sensitivity / 40.f;

	float camX = PS2_MEM_ReadFloat(camBase + MOHV_ONFOOT_CAMX);
	float camY = PS2_MEM_ReadFloat(camBase + MOHV_ONFOOT_CAMY);

	camX -= (float)xmouse * looksensitivity / 300.f * (fov / 35.f);
	camY -= (float)ymouse * looksensitivity / 300.f * (fov / 35.f);

	PS2_MEM_WriteFloat(camBase + MOHV_ONFOOT_CAMX, (float)camX);
	PS2_MEM_WriteFloat(camBase + MOHV_ONFOOT_CAMY, (float)camY);

}