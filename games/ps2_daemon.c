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

#define DAE_CAM_Y 0x2B05B0
#define DAE_CAMXBASE 0x2A6744
#define DAE_CAM_X 0x98

static uint8_t PS2_DAE_Status(void);
static uint8_t PS2_DAE_DetectCam(void);
static void PS2_DAE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Daemon Summoner",
	PS2_DAE_Status,
	PS2_DAE_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_DAEMONSUMMONER = &GAMEDRIVER_INTERFACE;

static uint32_t camXBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_DAE_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C4553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F353336U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E35323BU);
}

static uint8_t PS2_DAE_DetectCam(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(DAE_CAMXBASE);
	if (tempCamBase)
	{
		camXBase = tempCamBase;
		return 1;
	}
	return 0;
}

static void PS2_DAE_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (!PS2_DAE_DetectCam())
	// 	return;

	camXBase = PS2_MEM_ReadUInt(DAE_CAMXBASE);

	// float zoom = PS2_MEM_ReadFloat(TAA_ZOOM) / 1.221730351f;
	float zoom = 1.f;
	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 300.f;

	float camX = PS2_MEM_ReadFloat(camXBase + DAE_CAM_X);
	camX -= (float)xmouse * looksensitivity / scale * zoom;
	PS2_MEM_WriteFloat(camXBase + DAE_CAM_X, (float)camX);

	float camY = PS2_MEM_ReadFloat(DAE_CAM_Y);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * zoom;
	camY = ClampFloat(camY, -0.8000000119f, 0.8000000119f);
	PS2_MEM_WriteFloat(DAE_CAM_Y, (float)camY);

}