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

#define GITS_CAMYBASE 0x11670C0
#define GITS_CAMYBASE2 0x11671F0
// #define GITS_CAMYBASE2 0x14F0780
#define GITS_SANITY_1_VALUE 0x5044AB08
#define GITS_SANITY_2_VALUE 0x77777777
// offsets from camYBase
#define GITS_SANITY_1 0x30
#define GITS_SANITY_2 0x28
#define GITS_CAMY -0x39C
#define GITS_CAMXBASE -0x348
// offset from camXBase
// #define GITS_CAMX 0x1A54
#define GITS_CAMX 0x54
// #define GITS_CAMY 0x1504CF4
// #define GITS_CAMX 0x1506AE4

static uint8_t PSP_GITS_Status(void);
static uint8_t PSP_GITS_DetectCambase(void);
static void PSP_GITS_Inject(void);
static uint32_t camBases[] = {GITS_CAMYBASE2, GITS_CAMYBASE};


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
	int i;
	for (i = 0; i < 2; ++i)
	{
		uint32_t tempCamBase = PSP_MEM_ReadPointer(camBases[i]);

		if (tempCamBase &&
			PSP_MEM_ReadWord(tempCamBase + GITS_SANITY_1) == GITS_SANITY_1_VALUE &&
			PSP_MEM_ReadWord(tempCamBase + GITS_SANITY_2) == GITS_SANITY_2_VALUE)
		{
			camYBase = tempCamBase;
			return 1;
		}
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PSP_GITS_Inject(void)
{
	// TODO: sanity check that checks one camYBase
	//			find all the possible cambases
	//			or better find a pointer to cambase pointer
	//			breaks when picking up different weapons? 

	if (xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// camYBase = PSP_MEM_ReadPointer(GITS_CAMYBASE);
	// camYBase = PSP_MEM_ReadPointer(GITS_CAMYBASE2);

	if (!PSP_GITS_DetectCambase())
		return;

	camXBase = PSP_MEM_ReadPointer(camYBase + GITS_CAMXBASE);

	float camX = PSP_MEM_ReadFloat(camXBase + GITS_CAMX);
	float camY = PSP_MEM_ReadFloat(camYBase + GITS_CAMY);

	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 600.f;
	const float zoom = 1.f;

	camX -= (float)xmouse * looksensitivity / scale / zoom;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / zoom;

	while (camX > TAU / 2)
		camX -= TAU;
	while (camX < -TAU / 2)
		camX += TAU;
	camY = ClampFloat(camY, -1.134464025f, 1.134464025f);

	PSP_MEM_WriteFloat(camXBase + GITS_CAMX, (float)camX);
	PSP_MEM_WriteFloat(camYBase + GITS_CAMY, (float)camY);
}