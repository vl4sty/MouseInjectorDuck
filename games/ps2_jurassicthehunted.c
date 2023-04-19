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

#define JTH_CAMBASE 0x6BE670 // needs cheat
#define JTH_CAMBASE_SANITY_1_VALUE 0xC0F35E00
#define JTH_CAMBASE_SANITY_2_VALUE 0x06000000
// offsets from camBase
#define JTH_CAMBASE_SANITY_1 0x0
#define JTH_CAMBASE_SANITY_2 0x18
#define JTH_IS_INGAME_CUTSCENE 0x4
#define JTH_CAMY 0x13C
#define JTH_CAMX 0x140
#define JTH_BALANCE_SPEED 0x364
#define JTH_BALANCE_POS 0x368

#define JTH_FOV_BASE 0xE0B448
#define JTH_FOV 0x98

#define JTH_IS_PAUSED 0x5DB6FC

static uint8_t PS2_JTH_Status(void);
static uint8_t PS2_JTH_CambaseValid(void);
static uint8_t PS2_JTH_DetectCambase(void);
static void PS2_JTH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Jurassic: The Hunted",
	PS2_JTH_Status,
	PS2_JTH_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_JURASSICTHEHUNTED = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static float scale = 700.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_JTH_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323139U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E30373BU);
}

static uint8_t PS2_JTH_CambaseValid(void)
{
	if (camBase &&
		PS2_MEM_ReadWord(camBase + JTH_CAMBASE_SANITY_1) == JTH_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(camBase + JTH_CAMBASE_SANITY_2) == JTH_CAMBASE_SANITY_2_VALUE)
	{
		return 1;
	}
	return 0;
}

static uint8_t PS2_JTH_DetectCambase(void)
{
	// check if last camBase is still valid
	if (PS2_JTH_CambaseValid())
		return 1;

	uint32_t tempCamBase = PS2_MEM_ReadUInt(JTH_CAMBASE);
	if (tempCamBase &&
		PS2_MEM_ReadWord(tempCamBase + JTH_CAMBASE_SANITY_1) == JTH_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + JTH_CAMBASE_SANITY_2) == JTH_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_JTH_Inject(void)
{
	// TODO: try looking for playerBase then a camBase offset from that
	// TODO: look for different free nop cheat to write camBase
	//			preferably one that only writes camBase
	// TODO: clamp x on turret

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS2_MEM_ReadUInt(JTH_IS_PAUSED))
		return;

	if (!PS2_JTH_DetectCambase())
		return;
	
	// in-game cutscenes and quick-time button mash event
	if (PS2_MEM_ReadUInt(camBase + JTH_IS_INGAME_CUTSCENE))
		return;

	float looksensitivity = (float)sensitivity / 20.f;
	
	if (PS2_MEM_ReadFloat(camBase + JTH_BALANCE_POS) != 0.f)
	{
		float leanSpeed = PS2_MEM_ReadFloat(camBase + JTH_BALANCE_SPEED);
		float leanScale = 800.f;
		leanSpeed -= (float)xmouse * looksensitivity / leanScale;
		leanSpeed = ClampFloat(leanSpeed, -0.993, 0.993);
		PS2_MEM_WriteFloat(camBase + JTH_BALANCE_SPEED, leanSpeed);

		return;
	}

	uint32_t fovBase = PS2_MEM_ReadUInt(JTH_FOV_BASE);
	float fov = PS2_MEM_ReadFloat(fovBase + JTH_FOV) / 1.22173059f;

	float camX = PS2_MEM_ReadFloat(camBase + JTH_CAMX);
	float camY = PS2_MEM_ReadFloat(camBase + JTH_CAMY);

	camX += (float)xmouse * looksensitivity / scale * fov;
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;

	// clamp Y
	if (camY > 3.f && camY < 4.886921883f)
		camY = 4.886921883f;
	if (camY < 3.f && camY > 1.39626348f)
		camY = 1.39626348f;

	PS2_MEM_WriteFloat(camBase + JTH_CAMX, (float)camX);
	PS2_MEM_WriteFloat(camBase + JTH_CAMY, (float)camY);
}