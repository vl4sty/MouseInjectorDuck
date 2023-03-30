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

#define TAA_CAMBASE 0x33A038
// #define TAA_CAM_Y 0x1E4AE8C
// #define TAA_CAM_X 0x1E4AE90
#define TAA_CAM_Y 0xBFC
#define TAA_CAM_X 0xC00

static uint8_t PS2_TAA_Status(void);
static uint8_t PS2_TAA_DetectCam(void);
static void PS2_TAA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Tribes: Aerial Assault",
	PS2_TAA_Status,
	PS2_TAA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_TRIBESAA = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_TAA_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323031U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E34393BU);
}

static uint8_t PS2_TAA_DetectCam(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(TAA_CAMBASE);
	if (tempCamBase)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}

static void PS2_TAA_Inject(void)
{
	// TODO: find fov
	// TODO: disable auto-aim

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_TAA_DetectCam())
		return;

	// float fov = PS2_MEM_ReadFloat(cambase + RDR_camFov);
	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 300.f;

	float camX = PS2_MEM_ReadFloat(camBase + TAA_CAM_X);
	camX -= (float)xmouse * looksensitivity / scale;
	PS2_MEM_WriteFloat(camBase + TAA_CAM_X, (float)camX);

	float camY = PS2_MEM_ReadFloat(camBase + TAA_CAM_Y);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
	PS2_MEM_WriteFloat(camBase + TAA_CAM_Y, (float)camY);

}