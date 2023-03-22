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

#define PI 3.14159265f // 0x40490FDB
#define TAU 6.2831853f // 0x40C90FDB

#define GERA_CAMBASE 0x711068
#define GERA_CAMBASE_SANITY_1_VALUE 0xF01E5F00
#define GERA_CAMBASE_SANITY_2_VALUE 0x901B5F00
// offsets from cambase
#define GERA_CAMBASE_SANITY_1 0x8
#define GERA_CAMBASE_SANITY_2 0x10
#define GERA_CAMX_COS 0x50
#define GERA_CAMX_SIN 0x54
#define GERA_CAMY 0x188
#define GERA_FOV 0x204
#define GERA_CAM_TYPE 0x250
#define GERA_GUN_SWAY 0x4E0
// #define GERA_CAMY 0x7D3558
// #define GERA_CAMX_COS 0x7D3420
// #define GERA_CAMX_SIN 0x7D3424

#define GERA_CAM_ANGLE_UNSET -99.f

static uint8_t PS2_GERA_Status(void);
static uint8_t PS2_GERA_DetectCambase(void);
static void PS2_GERA_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"GoldenEye: Rogue Agent",
	PS2_GERA_Status,
	PS2_GERA_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_GEROGUEAGENT = &GAMEDRIVER_INTERFACE;

uint32_t camBase = 0;
float angle = GERA_CAM_ANGLE_UNSET;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_GERA_Status(void)
{
	return (PS2_MEM_ReadWord(0x0078B334) == 0x534C5553U && 
			PS2_MEM_ReadWord(0x0078B338) == 0x5F323130U &&
			PS2_MEM_ReadWord(0x0078B33C) == 0x2E36343BU);
}

static uint8_t PS2_GERA_DetectCambase(void)
{
	uint32_t tempCamBase = PS2_MEM_ReadUInt(GERA_CAMBASE);
	if (tempCamBase &&
		PS2_MEM_ReadWord(tempCamBase + GERA_CAMBASE_SANITY_1) == GERA_CAMBASE_SANITY_1_VALUE &&
		PS2_MEM_ReadWord(tempCamBase + GERA_CAMBASE_SANITY_2) == GERA_CAMBASE_SANITY_2_VALUE)
	{
		camBase = tempCamBase;
		return 1;
	}
	return 0;
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_GERA_Inject(void)
{
	// TODO: Find camBase
	// TODO: Disable aim-assist
	// TODO: determine if a cheat is needed to prevent other things from changing camX values
	//			or do a check against the last camX to determine if it was changed by something else
	//			and just UNSET it again so the new angle is used
	// TODO: Disable or fix gun sway
	//			also change gun sway direction? (sin/cos?)
	//			rotational, lateral, and forward/backward sway
	// TODO: Fix Melee warping camera when it snaps to a guy, maybe prevent camY moving during
	// 			Also during hostage taking animation
	// TODO: Prevent cam during
	//			pause

	// PS2_MEM_WriteFloat(camBase + GERA_CAM_TYPE, 0.f);

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (!PS2_GERA_DetectCambase())
		return;

	
	// prevent cam type from changing during melee strike and warping
	// PS2_MEM_WriteUInt16(camBase + GERA_CAM_TYPE, 0x0304);
	
	float fov = PS2_MEM_ReadFloat(camBase + GERA_FOV);
	float looksensitivity = (float)sensitivity / 10000.f * (fov / 35.f);

	float camY = PS2_MEM_ReadFloat(camBase + GERA_CAMY);
	float camXSin = PS2_MEM_ReadFloat(camBase + GERA_CAMX_SIN);
	float camXCos = PS2_MEM_ReadFloat(camBase + GERA_CAMX_COS);

	// TODO: just always set angle like this??
	// if (angle == GERA_CAM_ANGLE_UNSET)
	// {
	angle = atan(camXSin / camXCos);
	if (camXCos < 0)
		angle += PI;
	// }

	// angle = atan(camXSin / camXCos);

	angle -= (float)xmouse * looksensitivity;

	camXSin = sin(angle);
	camXCos = cos(angle);

	camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity;

	camY = ClampFloat(camY, -0.9, 0.9);

	PS2_MEM_WriteFloat(camBase + GERA_CAMY, (float)camY);
	PS2_MEM_WriteFloat(camBase + GERA_CAMX_SIN, camXSin);
	PS2_MEM_WriteFloat(camBase + GERA_CAMX_COS, camXCos);

}