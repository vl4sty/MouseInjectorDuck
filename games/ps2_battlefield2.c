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

// #define BF2_CAM_BASE_PTR 0x782B98
#define BF2_CAM_BASE_PTR 0xB295F0
// offsets from camBase
#define BF2_CAMX 0x4
#define BF2_CAMY 0x8

static uint8_t PS2_BF2_Status(void);
static uint8_t PS2_BF2_DetectCamBase(void);
static void PS2_BF2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Battlefield 2: Modern Combat",
	PS2_BF2_Status,
	PS2_BF2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_BATTLEFIELD2 = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_BF2_Status(void)
{
	// SLUS_210.26
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323130U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E32363BU);
}

static uint8_t PS2_BF2_DetectCamBase(void)
{
	uint32_t p4 = PS2_MEM_ReadUInt(BF2_CAM_BASE_PTR) + 0x1C8; // 1AD86B0 + 1C8 = 1AD8878
	uint32_t p3 = PS2_MEM_ReadUInt(p4); // 1DDA760
	uint32_t p2 = PS2_MEM_ReadUInt(p3); // 7F8EF0
	uint32_t p1 = PS2_MEM_ReadUInt(p2) + 0x1C; // 1B11420 + 1C = 1B1143C
	uint32_t tempCamBase = PS2_MEM_ReadUInt(p1);

	// TODO: sanity check
	if (tempCamBase)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

static void PS2_BF2_Inject(void)
{
	// TODO: test camBase tree
	// TODO: playerBase? and look for current weapon camera since
	//			each weapon uses different camera values???
	// TODO: tank barrel
	// TODO: turrets
	//			static
	//			car mounted

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_BF2_DetectCamBase())
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 250.f;
	// float fov = PS2_MEM_ReadFloat(RTCW_FOV) / 106.5f;
	float fov = 1.f;

	float camX = PS2_MEM_ReadFloat(camBase + BF2_CAMX);
	camX += (float)xmouse * looksensitivity / scale * fov;
	PS2_MEM_WriteFloat(camBase + BF2_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + BF2_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;

	PS2_MEM_WriteFloat(camBase + BF2_CAMY, (float)camY);

}