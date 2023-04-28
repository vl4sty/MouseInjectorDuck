//==========================================================================
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

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB

#define TE_IS_PAUSED 0x80310A8C
#define TE_IS_PAUSED_TRUE 0x00000002

#define TE_IS_IN_GAME_CUTSCENE 0x8070C834
#define TE_IS_IN_GAME_CUTSCENE_TRUE 0x00000001

#define TE_ONFOOT_CAMBASE 0x803113AC
#define TE_ONFOOT_CAMBASE_SANITY_1_VALUE 0xFFA284BF
// offsets from camBase
#define TE_ONFOOT_CAMBASE_SANITY_1 0x2C
#define TE_ONFOOT_CAMX 0xF0
#define TE_ONFOOT_CAMY 0x41C
#define TE_ONFOOT_FOV 0x8D8
#define TE_CLIMB_CAMX 0x420
#define TE_IS_NOT_CLIMBING 0x400
#define TE_IN_WATER 0x1888
#define TE_TURRET_BASE 0x19FC
// offsets from turret base
#define TE_TURRET_CAMY 0x448
#define TE_TURRET_CAMX 0x44C
#define TE_TURRET_CAMY_BOUND 0x498
#define TE_TURRET_CAMX_BOUND 0x49C

#define TE_IS_CLIMBING 0x807DDF28
#define TE_IS_CLIMBING_TRUE 0xBF000000

#define TE_FLYING_BASE 0x803113AC
#define TE_FLYING_BASE_SANITY_1_VALUE 0x3F7B1D3C
#define TE_FLYING_BASE_SANITY_2_VALUE 0xFFA284FF
// offsets from flying camBase
#define TE_FLYING_BASE_SANITY_1 0x40
#define TE_FLYING_BASE_SANITY_2 0x2C
#define TE_FLYING_ROLL_SPEED 0xB4C
#define TE_FLYING_PITCH_SPEED 0xB50

#define TE_CLIMBING_CAMY_REBOUND_OP 0x801CBEBC


static uint8_t GC_TE_Status(void);
static uint8_t GC_TE_DetectCamBase(void);
static uint8_t GC_TE_DetectFlyingBase(void);
static void GC_TE_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Turok: Evolution",
	GC_TE_Status,
	GC_TE_Inject,
	1, // if tickrate is any lower, mouse input will get sluggish
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_GC_TUROKEVOLUTION = &GAMEDRIVER_INTERFACE;

static uint32_t camBase = 0;
static uint32_t flyingCamBase = 0;
static float scale = 300.f;
static float lastRoll = 0.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t GC_TE_Status(void)
{
	// GTKE51
	return (MEM_ReadUInt(0x80000000) == 0x47544B45U && 
			MEM_ReadUInt(0x80000004) == 0x35310000U);
}

static uint8_t GC_TE_DetectCamBase(void)
{
	uint32_t tempCamBase = MEM_ReadUInt(TE_ONFOOT_CAMBASE);
	if (tempCamBase &&
		MEM_ReadUInt(tempCamBase + TE_ONFOOT_CAMBASE_SANITY_1) == TE_ONFOOT_CAMBASE_SANITY_1_VALUE) // &&
		// MEM_ReadUInt(tempCamBase + TE_ONFOOT_CAMBASE_SANITY_2) == TE_ONFOOT_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}

static uint8_t GC_TE_DetectFlyingBase(void)
{
	uint32_t tempCamBase = MEM_ReadUInt(TE_FLYING_BASE);
	if (tempCamBase &&
		MEM_ReadUInt(tempCamBase + TE_FLYING_BASE_SANITY_1) == TE_FLYING_BASE_SANITY_1_VALUE &&
		MEM_ReadUInt(tempCamBase + TE_FLYING_BASE_SANITY_2) == TE_FLYING_BASE_SANITY_2_VALUE)
	{
		flyingCamBase = tempCamBase;
		return 1;
	}
	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void GC_TE_Inject(void)
{
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (MEM_ReadUInt(TE_IS_PAUSED) == TE_IS_PAUSED_TRUE)
		return;

	if (MEM_ReadUInt(TE_IS_IN_GAME_CUTSCENE) == TE_IS_IN_GAME_CUTSCENE_TRUE)
		return;

	const float looksensitivity = (float)sensitivity / 40.f;

	if (GC_TE_DetectFlyingBase())
	{
		float flyingPitchSpeed = MEM_ReadFloat(flyingCamBase + TE_FLYING_PITCH_SPEED);
		flyingPitchSpeed += (float)ymouse * looksensitivity / 100.f;
		flyingPitchSpeed = ClampFloat(flyingPitchSpeed, -1.33f, 1.33f);
		MEM_WriteFloat(flyingCamBase + TE_FLYING_PITCH_SPEED, flyingPitchSpeed);

		float flyingRollSpeed = MEM_ReadFloat(flyingCamBase + TE_FLYING_ROLL_SPEED);
		flyingRollSpeed += (float)xmouse * looksensitivity / scale;
		// flyingRollSpeed = ClampFloat(flyingRollSpeed, -0.83f, 0.83f);
		flyingRollSpeed = ClampFloat(flyingRollSpeed, -1.2f, 1.2f);
		MEM_WriteFloat(flyingCamBase + TE_FLYING_ROLL_SPEED, flyingRollSpeed);
		lastRoll = flyingRollSpeed;

		return;
	}

	if (!GC_TE_DetectCamBase())
		return;
	
	uint32_t turretBase = MEM_ReadUInt(camBase + TE_TURRET_BASE);
	
	if (turretBase)
	{
		float camX = MEM_ReadFloat(turretBase + TE_TURRET_CAMX);
		camX -= (float)xmouse * looksensitivity / scale;
		float camXBound = MEM_ReadFloat(turretBase + TE_TURRET_CAMX_BOUND);
		camX = ClampFloat(camX, -camXBound, camXBound);

		float camY = MEM_ReadFloat(turretBase + TE_TURRET_CAMY);
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
		float camYBound = MEM_ReadFloat(turretBase + TE_TURRET_CAMY_BOUND);
		camY = ClampFloat(camY, -camYBound, camYBound);

		MEM_WriteFloat(turretBase + TE_TURRET_CAMX, camX);
		MEM_WriteFloat(turretBase + TE_TURRET_CAMY, camY);
	}
	else if (!MEM_ReadUInt(camBase + TE_IS_NOT_CLIMBING) && !MEM_ReadUInt(camBase + TE_IN_WATER))
	{
		// if climbing and not in water

		if (MEM_ReadUInt(TE_CLIMBING_CAMY_REBOUND_OP) == 0x60000000) // if patch is enabled
		{
			float camX = MEM_ReadFloat(camBase + TE_CLIMB_CAMX);
			camX -= (float)xmouse * looksensitivity * (360.f / TAU) / scale;
			camX = ClampFloat(camX, -110.f, 110.f);

			float camY = MEM_ReadFloat(camBase + TE_ONFOOT_CAMY);
			camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * (360.f / TAU) / scale;
			camY = ClampFloat(camY, -75.f, 225.f);

			MEM_WriteFloat(camBase + TE_CLIMB_CAMX, camX);
			MEM_WriteFloat(camBase + TE_ONFOOT_CAMY, camY);
		}

		return;
	}
	else
	{
		float fov = MEM_ReadFloat(camBase + TE_ONFOOT_FOV) / 90.f;
		
		float camX = MEM_ReadFloat(camBase + TE_ONFOOT_CAMX);
		camX -= (float)xmouse * looksensitivity / scale * fov;
		while (camX >= TAU)
			camX -= TAU;
		while (camX < 0)
			camX += TAU;

		float camY = MEM_ReadFloat(camBase + TE_ONFOOT_CAMY);
		camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * (360.f / TAU) / scale * fov;
		camY = ClampFloat(camY, -70.f, 70.f);

		MEM_WriteFloat(camBase + TE_ONFOOT_CAMX, camX);
		MEM_WriteFloat(camBase + TE_ONFOOT_CAMY, camY);
	}
}