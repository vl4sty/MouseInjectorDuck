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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define T3_CAMBASE 0x8012D8E0
#define T3_CAMBASE_SANITY_1_VALUE 0x00DD8054
#define T3_CAMBASE_SANITY_2_VALUE 0x00640102
// -- offsets from cambase --
#define T3_CAMBASE_SANITY_1 0xC8
#define T3_CAMBASE_SANITY_2 0xF8
#define T3_CAMX 0x6C
#define T3_CAMY 0xD2C
#define T3_CAMY_ANGLE 0xDD0

static uint8_t N64_T3_Status(void);
static uint8_t N64_T3_DetectCam(void);
static void N64_T3_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Turok 3: Shadow of Oblivion",
	N64_T3_Status,
	N64_T3_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_N64_TUROK3 = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

static uint8_t N64_T3_Status(void)
{
	return (N64_MEM_ReadUInt(0x80000000) == 0x3C1A002E && N64_MEM_ReadUInt(0x80000004) == 0x275A5ED0); // unique header in RDRAM
}

static uint8_t N64_T3_DetectCam(void)
{
	uint32_t tempCamBase = N64_MEM_ReadUInt(T3_CAMBASE);
	if (tempCamBase &&
		N64_MEM_ReadUInt(tempCamBase + T3_CAMBASE_SANITY_1) == T3_CAMBASE_SANITY_1_VALUE &&
		N64_MEM_ReadUInt(tempCamBase + T3_CAMBASE_SANITY_2) == T3_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}

static void N64_T3_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!N64_T3_DetectCam())
		return;

	const float looksensitivity = (float)sensitivity;
	const float scale = 6400.f;

	float camX = N64_MEM_ReadFloat(camBase + T3_CAMX);
	float camY = N64_MEM_ReadFloat(camBase + T3_CAMY);

	camX += (float)xmouse * looksensitivity / scale;
	camY -= (float)ymouse * looksensitivity / scale;

	if (camY > 1)
		camY = 1;
	if (camY < -1)
		camY = -1;

	// cursorx = ClampFloat(cursorx, -144.f, 144.f);
	// cursory = ClampFloat(cursory, -104.f, 104.f);

	N64_MEM_WriteFloat(camBase + T3_CAMX, camX);
	N64_MEM_WriteFloat(camBase + T3_CAMY, camY);
	N64_MEM_WriteFloat(camBase + T3_CAMY_ANGLE, camY * 1.570796);
}