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

#define RTI_IS_PAUSED 0x4CF37C
#define RTI_NOT_ON_LADDER 0x4CE774

#define RTI_FOV 0x4CF488
#define RTI_CAMBASE 0x1FB58A0
#define RTI_CAMBASE_SANITY_1_VALUE 0x28CA4800
#define RTI_CAMBASE_SANITY_2_VALUE 0x70CFA500
// offsets from cambase
#define RTI_CAMBASE_SANITY_1 0x4
#define RTI_CAMBASE_SANITY_2 0xC
#define RTI_CAMX_SIN 0x30
#define RTI_CAMX_COS 0x34
#define RTI_CAMY_BASE 0xD0
#define RTI_NOT_ON_BIKE 0xF4
// offset from camY base
#define RTI_CAMY 0x98
#define RTI_FREE_CAMX 0x10C

// #define RTI_FOV 0x4CF488
// #define RTI_CAMBASE 0x461060
// #define RTI_CAMBASE_SANITY_1_VALUE 0x085FA600
// #define RTI_CAMBASE_SANITY_2_VALUE 0x502F4800
// // offsets from cambase
// #define RTI_CAMBASE_SANITY_1 0x0
// #define RTI_CAMBASE_SANITY_2 0x4
// #define RTI_CAMX_SIN 0xB8
// #define RTI_CAMX_COS 0xBC
// #define RTI_CAMY_BASE 0x158
// #define RTI_NOT_ON_BIKE 0x17C
// // offset from camY base
// #define RTI_CAMY 0x98
// #define RTI_FREE_CAMX 0x10C

static uint8_t PS2_RTI_Status(void);
static uint8_t PS2_RTI_DetectCamBase(void);
static uint8_t PS2_RTI_CamBaseValid(void);
static void PS2_RTI_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Robotech: Invasion",
	PS2_RTI_Status,
	PS2_RTI_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_ROBOTECHINVASION = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t camXBase = 0;
static uint32_t camYBase = 0;
static float scale = 600.f;
static uint8_t levelStartRightStickBudge = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_RTI_Status(void)
{
	return (PS2_MEM_ReadWord(0x004B50CC) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x004B50D0) == 0x5F323038U &&
			PS2_MEM_ReadWord(0x004B50D4) == 0x2E32333BU);
}

// Determine if last cambase is still valid
static uint8_t PS2_RTI_CamBaseValid(void)
{
	if (camBase &&
		PS2_MEM_ReadWord(camBase + RTI_CAMBASE_SANITY_1) == RTI_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(camBase + RTI_CAMBASE_SANITY_2) == RTI_CAMBASE_SANITY_2_VALUE)
	{
		return 1;
	}

	camBase = 0;
	return 0;
}

static uint8_t PS2_RTI_DetectCamBase(void)
{
	if (PS2_RTI_CamBaseValid())
		return 1;

	uint32_t tempCamBase = PS2_MEM_ReadUInt(RTI_CAMBASE);
	if (tempCamBase &&
		PS2_MEM_ReadWord(tempCamBase + RTI_CAMBASE_SANITY_1) == RTI_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + RTI_CAMBASE_SANITY_2) == RTI_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}


	return 0;
}

static void PS2_RTI_Inject(void)
{
	// TODO: disable while
	//			stun from big purple enemy slam?

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS2_MEM_ReadUInt(RTI_IS_PAUSED))
		return;

	if (!PS2_MEM_ReadUInt(RTI_NOT_ON_LADDER))
		return;
	
	if (!PS2_RTI_DetectCamBase())
		return;
	
	if (!PS2_MEM_ReadUInt(camBase + RTI_NOT_ON_BIKE))
		return;
	
	// camBase = PS2_MEM_ReadUInt(RTI_CAMBASE);
	camYBase = PS2_MEM_ReadUInt(camBase + RTI_CAMY_BASE);

	// when reset, camX will rebound
	if (!PS2_MEM_ReadUInt(camYBase + RTI_FREE_CAMX))
	{
		// set to a non-zero value once to allow camX movement
		PS2_MEM_WriteUInt(camYBase + RTI_FREE_CAMX, 0x11111111);
	}

	float fov = PS2_MEM_ReadFloat(RTI_FOV) / 60.f;
	// fov = 1.f;
	float looksensitivity = (float)sensitivity / 20.f;

	float camXSin = PS2_MEM_ReadFloat(camBase + RTI_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camBase + RTI_CAMX_COS);

	float angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += TAU / 2;
	
	angle += (float)xmouse * looksensitivity / scale * fov;

	camXSin = sin(angle);
	camXCos = cos(angle);

	PS2_MEM_WriteFloat(camBase + RTI_CAMX_SIN, (float)camXSin);
	PS2_MEM_WriteFloat(camBase + RTI_CAMX_COS, (float)camXCos);

	float camY = PS2_MEM_ReadFloat(camYBase + RTI_CAMY);
	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / (scale / 60.f) * fov;
	PS2_MEM_WriteFloat(camYBase + RTI_CAMY, (float)camY);

}