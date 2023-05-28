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

#define NPPL_CAM_BASE_PTR 0xD897A8
// #define NPPL_CAMBASE_SANITY_1_VALUE 0x6666E63F
// offsets from camBase
// #define NPPL_CAMBASE_SANITY_1 0x68
#define NPPL_CAMY 0x334
#define NPPL_CAMX 0x338

static uint8_t PS2_NPPL_Status(void);
static uint8_t PS2_NPPL_DetectCamBase(void);
static void PS2_NPPL_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"NPPL Championship Paintball 2009",
	PS2_NPPL_Status,
	PS2_NPPL_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_NPPLPAINTBALL = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_NPPL_Status(void)
{
	// SLUS_218.55
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323138U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E35353BU);
}

// static uint8_t PS2_NPPL_DetectCamBase(void)
// {
// 	uint32_t tempCamBase = PS2_MEM_ReadUInt(NPPL_CAM_BASE_PTR);
// 	if (tempCamBase &&
// 		PS2_MEM_ReadWord(tempCamBase + NPPL_CAMBASE_SANITY_1) == NPPL_CAMBASE_SANITY_1_VALUE)
// 	{
// 		camBase = tempCamBase;
// 		return 1;
// 	}

// 	return 0;
// }

static void PS2_NPPL_Inject(void)
{
	// TODO: camBase sanity
	// TODO: gun sway
	//			disable game writing sway and calculate externally
	//			- or - 
	//			disable sway to avoid jittering gun
	// TODO: disabled during
	//			pause

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (!PS2_NPPL_DetectCamBase())
	// 	return;

	camBase = PS2_MEM_ReadUInt(NPPL_CAM_BASE_PTR);

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 5.f;
	// float fov = PS2_MEM_ReadFloat(camBase + TS_FOV) / 60.f;
	float fov = 1.f;

	float camX = PS2_MEM_ReadFloat(camBase + NPPL_CAMX);
	camX += (float)xmouse * looksensitivity * fov / scale;
	PS2_MEM_WriteFloat(camBase + NPPL_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + NPPL_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * fov / scale;
	camY = ClampFloat(camY, -55.f, 55.f);
	PS2_MEM_WriteFloat(camBase + NPPL_CAMY, (float)camY);

}