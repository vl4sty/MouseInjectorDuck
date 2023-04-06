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

#define GITS_CAMYBASE_PTR 0xAE8630
#define GITS_SANITY_1_VALUE 0x378D2740
#define GITS_SANITY_2_VALUE 0x1E36913F
// offsets from camYBase
#define GITS_SANITY_1 0x8
#define GITS_SANITY_2 0x10
#define GITS_CAMY 0x20
#define GITS_CAMXBASE 0x74
// offset from camXBase
#define GITS_CAMX 0x54

#define GITS_FOV 0xAEA29C

static uint8_t PSP_GITS_Status(void);
static uint8_t PSP_GITS_DetectCambase(void);
static void PSP_GITS_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Ghost in the Shell: Stand Alone Complex",
	PSP_GITS_Status,
	PSP_GITS_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PSP_GHOSTINTHESHELL = &GAMEDRIVER_INTERFACE;

static uint32_t camYBase = 0;
static uint32_t camXBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PSP_GITS_Status(void)
{
	// ULUS10020
	return (PSP_MEM_ReadWord(0xA98AE4) == 0x554C5553U &&
			PSP_MEM_ReadWord(0xA98AE8) == 0x31303032U && 
			PSP_MEM_ReadWord(0xA98AEC) == 0x30000000U);
}

static uint8_t PSP_GITS_DetectCambase(void)
{
	uint32_t camBasePtr = PSP_MEM_ReadPointer(GITS_CAMYBASE_PTR); // 01566C40
	if (!camBasePtr)
		return 0;

	uint32_t tempCamBase = PSP_MEM_ReadPointer(camBasePtr - 0x46C);

	if (tempCamBase &&
		PSP_MEM_ReadWord(tempCamBase + GITS_SANITY_1) == GITS_SANITY_1_VALUE &&
		PSP_MEM_ReadWord(tempCamBase + GITS_SANITY_2) == GITS_SANITY_2_VALUE)
	{
		camYBase = tempCamBase;
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PSP_GITS_Inject(void)
{
	// TODO: disable during
	//			in-game cutscene
	//			pause

	if (xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (!PSP_GITS_DetectCambase())
		return;

	camXBase = PSP_MEM_ReadPointer(camYBase + GITS_CAMXBASE);

	float camX = PSP_MEM_ReadFloat(camXBase + GITS_CAMX);
	float camY = PSP_MEM_ReadFloat(camYBase + GITS_CAMY);

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 600.f;
	const float fov = PSP_MEM_ReadFloat(GITS_FOV) / 47.f;

	camX -= (float)xmouse * looksensitivity / scale * fov;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;

	while (camX > TAU / 2)
		camX -= TAU;
	while (camX < -TAU / 2)
		camX += TAU;
	camY = ClampFloat(camY, -1.134464025f, 1.134464025f);

	PSP_MEM_WriteFloat(camXBase + GITS_CAMX, (float)camX);
	PSP_MEM_WriteFloat(camYBase + GITS_CAMY, (float)camY);
}