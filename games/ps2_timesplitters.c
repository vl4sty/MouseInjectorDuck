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

#define TS_CAM_BASE_PTR 0x3AFA20
#define TS_CAM_BASE_PTR_V110 0x3A40A0
#define TS_CABASE_SANITY_1_VALUE 0x6666E63F
// offsets from camBase
#define TS_CABASE_SANITY_1 0x68
#define TS_FOV 0x104
#define TS_CAMY 0x11C
#define TS_CAMX 0x114

#define TS_IS_BUSY_V200 0x352DB4
#define TS_IS_BUSY_V110 0x347BAC
#define TS_IS_BUSY_SANITY_VALUE 0x686F7374
// offset from isBusy
#define TS_IS_BUSY_SANITY 0xAC

static uint8_t PS2_TS_Status(void);
static uint8_t PS2_TS_DetectCamBase(void);
static uint8_t PS2_TS_IsBusy(void);
static void PS2_TS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"TimeSplitters v2.00",
	PS2_TS_Status,
	PS2_TS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_TIMESPLITTERS = &GAMEDRIVER_INTERFACE;

static uint32_t CAM_BASE_PTR_ADDRESSES[] = {
	TS_CAM_BASE_PTR,
	TS_CAM_BASE_PTR_V110
};
static uint32_t IS_BUSY_ADDRESSES[] = {
	TS_IS_BUSY_V110,
	TS_IS_BUSY_V200
};
static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_TS_Status(void)
{
	// SLUS_200.90
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323030U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E39303BU);
}

static uint8_t PS2_TS_DetectCamBase(void)
{
	int i;
	for (i = 0; i < 2; ++i)
	{
		uint32_t tempCamBase = PS2_MEM_ReadUInt(CAM_BASE_PTR_ADDRESSES[i]);
		if (tempCamBase &&
			PS2_MEM_ReadWord(tempCamBase + TS_CABASE_SANITY_1) == TS_CABASE_SANITY_1_VALUE)
		{
			camBase = tempCamBase;
			return 1;
		}
	}

	return 0;
}

static uint8_t PS2_TS_IsBusy(void)
{
	int i;
	for (i = 0; i < 2; ++i)
	{
		if (PS2_MEM_ReadWord(IS_BUSY_ADDRESSES[i] + TS_IS_BUSY_SANITY) == TS_IS_BUSY_SANITY_VALUE)
		{
			return PS2_MEM_ReadUInt(IS_BUSY_ADDRESSES[i]);
		}
	}
	return 0;
}

static void PS2_TS_Inject(void)
{
	// TODO: angle turret on camY extremes
	//			currently some enemies may be impossible to hit without using analog stick

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (PS2_MEM_ReadUInt(TS_IS_BUSY))
	// 	return;

	if (PS2_TS_IsBusy())
		return;
	
	if (!PS2_TS_DetectCamBase())
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 5.f;
	float fov = PS2_MEM_ReadFloat(camBase + TS_FOV) / 60.f;
	// float fov = 1.f;

	float camX = PS2_MEM_ReadFloat(camBase + TS_CAMX);
	camX -= (float)xmouse * looksensitivity * fov / scale;
	PS2_MEM_WriteFloat(camBase + TS_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + TS_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov / scale;
	camY = ClampFloat(camY, -50.f, 50.f);
	PS2_MEM_WriteFloat(camBase + TS_CAMY, (float)camY);

}