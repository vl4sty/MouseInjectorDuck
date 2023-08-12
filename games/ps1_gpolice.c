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

#define GP_CAMX 0xF36C4
#define GP_CAMY 0xF36C6
#define GP_CAMX_VEL 0xF36C8

#define GP_IS_PAUSED 0x7CE24
#define GP_IS_BUSY 0x8DBE0
#define GP_IS_MISSION_ACCOMPLISED 0x8F568

#define GP_MISSION_ACCOMPLISHED_TRUE 0x0600

#define GP_CONTROL_TYPE_DIRECT 1
#define GP_CONTROL_TYPE_INDIRECT 2

static uint8_t PS1_GP_Status(void);
static void PS1_GP_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"G-Police",
	PS1_GP_Status,
	PS1_GP_Inject,
	1, // 1000 Hz tickrate
	0, // crosshair sway supported for driver
	"Control Type: Direct",  // control type option
	"Control Type: Indirect",  // control type option
};

const GAMEDRIVER *GAME_PS1_GPOLICE = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint8_t controlType = GP_CONTROL_TYPE_DIRECT;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_GP_Status(void)
{
	return (PS1_MEM_ReadWord(0x9424) == 0x534C5553U &&  // SLUS_005.44 (Disc 1)
			PS1_MEM_ReadWord(0x9428) == 0x5F303035U && 
			PS1_MEM_ReadWord(0x942C) == 0x2E34343BU) ||
			(PS1_MEM_ReadWord(0x940C) == 0x534C5553U &&  // SLUS_005.56 (Disc 2)
			PS1_MEM_ReadWord(0x9410) == 0x5F303035U && 
			PS1_MEM_ReadWord(0x9414) == 0x2E35363BU);
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_GP_Inject(void)
{
	// TODO: isBusy prevents movement during mission accomplished but
	//			you should actually be able to still pilot during that

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	if (PS1_MEM_ReadHalfword(GP_IS_PAUSED))
		return;
	if (PS1_MEM_ReadHalfword(GP_IS_BUSY))
		return;

	if (optionToggle)
		controlType = GP_CONTROL_TYPE_INDIRECT;
	else
		controlType = GP_CONTROL_TYPE_DIRECT;
	
	const float fov = 1.f;
	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 15.f;

	if (controlType == GP_CONTROL_TYPE_DIRECT)
	{
		// camX direct
		// feels more like flying a drone than a helicopter
		// air-brake has no effect
		int16_t camX = PS1_MEM_ReadInt16(GP_CAMX);
		float camXF = (float)camX;
		float dx = (float)xmouse * looksensitivity * scale * fov;
		AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);
		PS1_MEM_WriteInt16(GP_CAMX, (int16_t)camXF);
	}
	else
	{
		// camXVel indirect
		// closer to how game usually works
		// air-brake still works
		int16_t camX = PS1_MEM_ReadInt16(GP_CAMX_VEL);
		float camXF = (float)camX;
		float dx = -(float)xmouse * looksensitivity * scale * fov;
		AccumulateAddRemainder(&camXF, &xAccumulator, -xmouse, dx);
		PS1_MEM_WriteInt16(GP_CAMX_VEL, (int16_t)camXF);
	}

	int16_t camY = PS1_MEM_ReadInt16(GP_CAMY);
	float camYF = (float)camY;
	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale * fov;
	AccumulateAddRemainder(&camYF, &yAccumulator, ym, dy);
	camYF = ClampFloat(camYF, -16000.f, 16000.f);
	PS1_MEM_WriteInt16(GP_CAMY, (int16_t)camYF);
}