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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define TAU 6.2831853f // 0x40C90FDB

#define SOC1_CAM_BASE_PTR 0x48D920
#define SOC1_CAM_BASE_SANITY_1_VALUE 0x004899F0
// offsets from camBase
#define SOC1_CAM_BASE_SANITY_1 0x0
#define SOC1_CAMX_SIN 0x54
#define SOC1_CAMX_COS 0x5C
#define SOC1_ZOOM 0x164
#define SOC1_CAMY_BASE_PTR 0xC0
#define SOC1_IS_CLIMBING_BASE 0x26C
// offset from camYBase
#define SOC1_CAMY 0x134
// offset from isClimbingBase
#define SOC1_IS_CLIMBING 0x2C

#define SOC1_IS_CLIMBING_TRUE 0xCAC0

#define SOC1_IS_PAUSED 0x4D49A4
#define SOC1_IS_PAUSED_TRUE 0x00000901

#define SOC1_IS_GAME_TIP 0x48EB58

#define SOC1_IS_COMMAND_MENU 0x48E750

#define SOC1_IS_MAP_DISPLAYED 0xDDEFF0

static uint8_t PS2_SOC1_Status(void);
static uint8_t PS2_SOC1_DetectCamBase(void);
static void PS2_SOC1_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"SOCOM U.S. Navy SEALs",
	PS2_SOC1_Status,
	PS2_SOC1_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_SOCOM1 = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t isClimbingBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_SOC1_Status(void)
{
	// SCUS_971.34
	return (PS2_MEM_ReadWord(0x00093390) == 0x53435553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F393731U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E33343BU);
}

static uint8_t PS2_SOC1_DetectCamBase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(SOC1_CAM_BASE_PTR);

	if (tempCamBase &&
		PS2_MEM_ReadUInt(tempCamBase + SOC1_CAM_BASE_SANITY_1) == SOC1_CAM_BASE_SANITY_1_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}

	return 0;
}

static void PS2_SOC1_Inject(void)
{
	// TODO: disable camX while climbing (ladder, ledge)
	// TODO: disable during orders
	// TODO: turning animation

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	if (PS2_MEM_ReadUInt(SOC1_IS_PAUSED) == SOC1_IS_PAUSED_TRUE)
		return;
	
	if (PS2_MEM_ReadUInt(SOC1_IS_GAME_TIP))
		return;

	if (PS2_MEM_ReadUInt(SOC1_IS_COMMAND_MENU))
		return;

	if (!PS2_SOC1_DetectCamBase())
		return;

	uint32_t camYBase = PS2_MEM_ReadUInt(camBase + SOC1_CAMY_BASE_PTR);

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 450.f;
	float zoom = 1.f / PS2_MEM_ReadFloat(camBase + SOC1_ZOOM);
	// float fov = 1.f;

	isClimbingBase = PS2_MEM_ReadUInt(camBase + SOC1_IS_CLIMBING_BASE);
	if (PS2_MEM_ReadUInt16(isClimbingBase + SOC1_IS_CLIMBING) != SOC1_IS_CLIMBING_TRUE)
	{
		float camXSin = PS2_MEM_ReadFloat(camBase + SOC1_CAMX_SIN);
		float camXCos = PS2_MEM_ReadFloat(camBase + SOC1_CAMX_COS);
		float angle = atan(camXSin / camXCos);
		if (camXCos < 0)
			angle += TAU;
		
		angle -= (float)xmouse * looksensitivity / scale * zoom;

		camXSin = sin(angle);
		camXCos = cos(angle);

		PS2_MEM_WriteFloat(camBase + SOC1_CAMX_SIN, (float)camXSin);
		PS2_MEM_WriteFloat(camBase + SOC1_CAMX_COS, (float)camXCos);
	}

	float camY = PS2_MEM_ReadFloat(camYBase + SOC1_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * zoom;
	camY = ClampFloat(camY, -1.221730351f, 1.047197461f);

	PS2_MEM_WriteFloat(camYBase + SOC1_CAMY, (float)camY);

}