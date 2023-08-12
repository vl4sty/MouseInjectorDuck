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
#include <math.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB

#define AUF_ONFOOT_CAMY 0x8036165C
#define AUF_ONFOOT_CAMX 0x80361660
#define AUF_ONFOOT_FOV 0x802F0BB8

#define AUF_AIMLOCK_BASE_PTR 0x803E74F4
#define AUF_AIMLOCK_SANITY_VALUE 0x80254D50
// offset from aimLockBase
#define AUF_AIMLOCK_SANITY 0x4
#define AUF_AIMLOCK_TOGGLE 0x68C

#define AUF_ONRAILS_CAMY 0x811EE964
#define AUF_ONRAILS_CAMX_SIN 0x811EE804
#define AUF_ONRAILS_CAMX_COS 0x811EE80C

#define AUF_IS_NOT_BUSY 0x80361674

static uint8_t GC_AUF_Status(void);
static void GC_AUF_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"007: Agent Under Fire",
	GC_AUF_Status,
	GC_AUF_Inject,
	1, // if tickrate is any lower, mouse input will get sluggish
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_GC_007AGENTUNDERFIRE = &GAMEDRIVER_INTERFACE;

static uint32_t aimLockBase = 0;
static float scale = 5.f;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t GC_AUF_Status(void)
{
	// GW7E69
	return (MEM_ReadUInt(0x80000000) == 0x47573745U && 
			MEM_ReadUInt(0x80000004) == 0x36390000U);
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void GC_AUF_Inject(void)
{
	// TODO: find different isBusy
	// FIXME: camX movement gets more jittery the farther it is from 0
	//			you can see the popping by going frame by frame and moving mouse

	// if (!MEM_ReadUInt(AUF_IS_NOT_BUSY))
	// 	return;

	if (aimLockBase == 0)
		aimLockBase = MEM_ReadUInt(AUF_AIMLOCK_BASE_PTR);
	
	if (MEM_ReadUInt(aimLockBase + AUF_AIMLOCK_SANITY) == AUF_AIMLOCK_SANITY_VALUE)
	{
		// disable aimlock
		MEM_WriteUInt(aimLockBase + AUF_AIMLOCK_TOGGLE, 0x0);
	}
	else {
		aimLockBase = MEM_ReadUInt(AUF_AIMLOCK_BASE_PTR);
	}

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;

	const float looksensitivity = (float)sensitivity / 40.f;
	float fov = MEM_ReadFloat(AUF_ONFOOT_FOV);


	if (!MEM_ReadUInt(AUF_ONFOOT_CAMY) && !MEM_ReadUInt(AUF_ONFOOT_CAMX))
	{
		// onRails (probably not a good sanity check for tank levels)
		float scale = 1400;
		float camY = MEM_ReadFloat(AUF_ONRAILS_CAMY);
		camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale;
		MEM_WriteFloat(AUF_ONRAILS_CAMY, (float)camY);

		float camXSin = MEM_ReadFloat(AUF_ONRAILS_CAMX_SIN);
		float camXCos = MEM_ReadFloat(AUF_ONRAILS_CAMX_COS);
		float angle = atan(camXSin / camXCos);
		if (camXCos < 0)
			angle -= PI;

		angle += (float)xmouse * looksensitivity / scale * 6;

		camXSin = sin(angle);
		camXCos = cos(angle);

		MEM_WriteFloat(AUF_ONRAILS_CAMX_SIN, (float)camXSin);
		MEM_WriteFloat(AUF_ONRAILS_CAMX_COS, (float)camXCos);
	}
	else
	{
		// onFoot	
		float camY = MEM_ReadFloat(AUF_ONFOOT_CAMY);
		float camX = MEM_ReadFloat(AUF_ONFOOT_CAMX);

		camX -= (float)xmouse * looksensitivity * (fov / 60.f) / scale;
		camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity * (fov / 60.f) / scale;

		MEM_WriteFloat(AUF_ONFOOT_CAMX, camX);
		MEM_WriteFloat(AUF_ONFOOT_CAMY, camY);
	}
}