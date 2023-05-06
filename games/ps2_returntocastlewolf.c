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

#define RTCW_ACTUAL_CAMY_BASE_PTR 0x236974
#define RTCW_CAM_Y 0x5E4034
#define RTCW_CAM_X 0x5E4038
#define RTCW_FOV 0x5AFEC0
#define RTCW_SANITY 0x5E4050

#define RTCW_IS_PAUSED 0x236490

#define RTCW_CURRENT_WEAPON 0xD459E0

static uint8_t PS2_RTCW_Status(void);
static uint8_t PS2_RTCW_DetectCam(void);
static void PS2_RTCW_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Return to Castle Wolfenstein: Operation Resurrection",
	PS2_RTCW_Status,
	PS2_RTCW_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_RETURNTOCASTLEWOLF = &GAMEDRIVER_INTERFACE;

static uint32_t actualCamYBase = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_RTCW_Status(void)
{
	// SLUS_202.97
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x00093394) == 0x5F323032U &&
			PS2_MEM_ReadWord(0x00093398) == 0x2E39373BU);
}

static uint8_t PS2_RTCW_DetectCam(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(RTCW_ACTUAL_CAMY_BASE_PTR);
	if (tempCamBase)
	{
		actualCamYBase = tempCamBase;
		return 1;
	}
	return 0;
}

static void PS2_RTCW_Inject(void)
{
	// TODO: Disable on menu, as camera addresses are used by something else
	//			check for some kind of base, look for camera values as offsets with debugger
	// TODO: disable during mission end stats (optional)
	//			make sure it's not based on the graphic appearing as you can get close
	//			 to the exit without leaving to see stats

	// TODO: current weapon base
	// TODO: check if current weapon has ammo before switching
	// TODO: change weapon keys based on weapon type (pistols, SMG, sniper, etc)
	//			pressing same key will scroll through weapons of same type
	// TODO: attach silence button (switches to silenced variant of current weapon)
	// TODO: prevent weapon switch while reloading or stop reload when switching

	// uint32_t currentWeapon = PS2_MEM_ReadUInt(RTCW_CURRENT_WEAPON);
	// if (K_1)
	// 	PS2_MEM_WriteUInt(RTCW_CURRENT_WEAPON, 1);
	// if (K_2)
	// 	PS2_MEM_WriteUInt(RTCW_CURRENT_WEAPON, 2);
	// if (K_3)
	// 	PS2_MEM_WriteUInt(RTCW_CURRENT_WEAPON, 3);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (!PS2_RTCW_DetectCam())
	// 	return;

	if (PS2_MEM_ReadWord(RTCW_SANITY) != 0x48554E4BU)
		return;

	if (PS2_MEM_ReadUInt(RTCW_IS_PAUSED))
		return;

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 5.f;
	float fov = PS2_MEM_ReadFloat(RTCW_FOV) / 106.5f;

	float camX = PS2_MEM_ReadFloat(RTCW_CAM_X);
	camX -= (float)xmouse * looksensitivity / scale * fov;
	PS2_MEM_WriteFloat(RTCW_CAM_X, (float)camX);

	float camY = PS2_MEM_ReadFloat(RTCW_CAM_Y);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;
	// game clamps internally to actual camY
	PS2_MEM_WriteFloat(RTCW_CAM_Y, (float)camY);

}